#include <platform/platform.h>

#include <core/memory_arena.h>
#include <engine/engine.h>
#include <engine/profiling.hpp>

auto inline thread_id_to_idx(u32 thread_id) -> u32 {
    for (u32 i = 0; i < TOTAL_THREAD_COUNT; i++) {
        if (Platform->thread_ids[i] == thread_id) {
            return i;
        }
    }
    printf("Did not find thread id: %d\n", thread_id);
    InvalidCodePath;
    return 0;
}

auto inline thread_idx_to_id(u32 thread_idx) -> u32 {
    Assert(thread_idx >= 0 && thread_idx < TOTAL_THREAD_COUNT);
    return Platform->thread_ids[thread_idx];
}

DEBUG_FRAME_END(debug_frame_end) {
    Assert(sizeof(DebugState) <= engine_memory->debug.size);
    DebugState* state = (DebugState*)engine_memory->debug.data;

    if (!state->is_initialized) {
        state->is_initialized = true;
        // Index 0 is the NIL node
        state->current_inspecting_frame = u64_max;
    }

    global_debug_table->current_debug_table = !global_debug_table->current_debug_table;
    u64 new_event_index = (u64)global_debug_table->current_debug_table << 32;
    u64 array_idx_and_event_idx = atomic_exchange_u64(&global_debug_table->event_index, new_event_index);
    u32 array_idx = array_idx_and_event_idx >> 32;
    u32 event_count = array_idx_and_event_idx & 0xFFFFFFFF;

    if (state->current_inspecting_frame != u64_max) {
        return;
    }

    u64 clock_start = __rdtsc();

    u32 frame_node_idx = add_kid(&state->node_forest);
    PrintEventNode* frame_node = &state->node_forest.nodes[frame_node_idx];
    frame_node->kind = PrintDebugEventType_Frame;
    frame_node->clock_start = ticks_at_start_of_frame;
    frame_node->clock_end = __rdtsc();
    frame_node->value_v2 = vec2(frame_duration_ms, frame_duration_before_sleep_ms);

    u32 current_thread_node_indexes[TOTAL_THREAD_COUNT] = {};
    for (u32 i = 0; i < TOTAL_THREAD_COUNT; i++) {
        current_thread_node_indexes[i] = add_kid(&state->node_forest, frame_node_idx);
        PrintEventNode* thread_node = &state->node_forest.nodes[current_thread_node_indexes[i]];
        thread_node->value_u32 = thread_idx_to_id(i);
        thread_node->kind = PrintDebugEventType_Thread;
    }
    frame_node->first_kid_idx = current_thread_node_indexes[0];

    for (u32 event_idx = 0; event_idx < event_count; event_idx++) {
        DebugEvent* event = global_debug_table->events[array_idx] + event_idx;
        u32 thread_idx = thread_id_to_idx(event->thread_id);
        // Each thread is on a seperate branch. So get the current node idx from that branch.
        u32* current_node_idx = &current_thread_node_indexes[thread_idx];
        switch (event->event_type) {

        case DebugEventType_BeginBlock: {
            u32 new_node_idx = add_kid(&state->node_forest, *current_node_idx);
            PrintEventNode* new_node = &state->node_forest.nodes[new_node_idx];
            new_node->GUID = event->GUID;
            new_node->clock_start = event->clock;
            new_node->kind = PrintDebugEventType_TimeBlock;

            *current_node_idx = new_node_idx;
        } break;
        case DebugEventType_EndBlock: {
            PrintEventNode* current_node = &state->node_forest.nodes[*current_node_idx];
            current_node->clock_end = event->clock;
            *current_node_idx = current_node->parent_idx;
        } break;
        case DebugEventType_Count:
        case DebugEventType_Unknown: {
            InvalidCodePath;
            break;
        }
        }
    }

    u64 clock_end = __rdtsc();
    u32 debug_index = add_kid(&state->node_forest, frame_node_idx);
    PrintEventNode* debug_processing = &state->node_forest.nodes[debug_index];
    debug_processing->GUID = DEBUG_NAME("process_debug_events");
    debug_processing->clock_start = clock_start;
    debug_processing->clock_end = clock_end;

    debug_processing->parent_idx = frame_node_idx;
    state->node_forest.nodes[frame_node->last_kid_idx].next_sib_idx = debug_index;
    frame_node->last_kid_idx = debug_index;

    u64 curr_frame_idx = state->processed_frame_count++;
    state->historic_frame_indices[curr_frame_idx % Historic_Frame_Count] = frame_node_idx;

    {
        u32 next_frame_entry_node_idx = state->historic_frame_indices[(curr_frame_idx + 1) % Historic_Frame_Count];
        u32 distance = (next_frame_entry_node_idx + PrintEventNode_Count - frame_node_idx) % PrintEventNode_Count;
        Assert(state->node_forest.curr_tree_node_count < distance);
    }
}

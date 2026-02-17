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
    DebugState* state = (DebugState*)engine_memory->debug;
    if (!state->is_initialized) {
        state->per_frame_arena.init((u8*)engine_memory->debug + sizeof(DebugState), Debug_Memory_Block_Size - sizeof(DebugState));
        state->nodes.init(&state->per_frame_arena, 1024);
        state->is_initialized = true;
    }

    global_debug_table->current_debug_table = !global_debug_table->current_debug_table;
    u64 new_event_index = (u64)global_debug_table->current_debug_table << 32;
    u64 array_idx_and_event_idx = atomic_exchange_u64(&global_debug_table->event_index, new_event_index);
    u32 array_idx = array_idx_and_event_idx >> 32;
    u32 event_count = array_idx_and_event_idx & 0xFFFFFFFF;

    u64 clock_start = __rdtsc();

    state->nodes.clear();
    List<PrintEventNode>& nodes = state->nodes;
    nodes.push(); // NIL node

    const u32 NIL_INDEX = 0;
    i32 frame_node_idx;
    PrintEventNode* frame_node = nodes.pushi(&frame_node_idx);
    frame_node->kind = PrintDebugEventType_Frame;
    frame_node->clock_start = ticks_at_start_of_frame;
    frame_node->clock_end = __rdtsc();
    frame_node->value_f32 = frame_duration_s;

    i32 current_thread_node_indexes[TOTAL_THREAD_COUNT] = {};
    for (u32 i = 0; i < TOTAL_THREAD_COUNT; i++) {

        PrintEventNode* thread_node = nodes.pushi(&current_thread_node_indexes[i]);
        thread_node->parent_idx = frame_node_idx;
        thread_node->value_u32 = thread_idx_to_id(i);
        thread_node->kind = PrintDebugEventType_Thread;
        if (i < TOTAL_THREAD_COUNT - 1) {
            thread_node->next_sib_idx = current_thread_node_indexes[i] + 1;
        }
    }
    frame_node->first_kid_idx = current_thread_node_indexes[0];

    for (u32 event_idx = 0; event_idx < event_count; event_idx++) {
        DebugEvent* event = global_debug_table->events[array_idx] + event_idx;
        u32 thread_idx = thread_id_to_idx(event->thread_id);
        // Each thread is on a seperate branch. So get the current node idx from that branch.
        i32* current_node_idx = &current_thread_node_indexes[thread_idx];
        PrintEventNode* current_node = &nodes[*current_node_idx];
        switch (event->event_type) {

        case DebugEventType_BeginBlock: {
            i32 new_node_idx;
            PrintEventNode* new_node = nodes.pushi(&new_node_idx);
            new_node->GUID = event->GUID;
            new_node->clock_start = event->clock;
            new_node->kind = PrintDebugEventType_TimeBlock;

            new_node->parent_idx = *current_node_idx;
            if (current_node->first_kid_idx == NIL_INDEX) {
                current_node->first_kid_idx = new_node_idx;
                new_node->parent_idx = *current_node_idx;
            }
            else {
                PrintEventNode* left_sib = &nodes[current_node->last_kid_idx];
                left_sib->next_sib_idx = new_node_idx;
            }
            current_node->last_kid_idx = new_node_idx;
            *current_node_idx = new_node_idx;

        } break;
        case DebugEventType_EndBlock: {
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
    i32 debug_index;
    PrintEventNode* debug_processing = nodes.pushi(&debug_index);
    debug_processing->GUID = DEBUG_NAME("process_debug_events");
    debug_processing->clock_start = clock_start;
    debug_processing->clock_end = clock_end;

    debug_processing->parent_idx = frame_node_idx;
    nodes[frame_node->last_kid_idx].next_sib_idx = debug_index;
    frame_node->last_kid_idx = debug_index;
}

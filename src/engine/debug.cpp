#include <platform/platform.h>

#include <engine/engine.h>
#include <engine/profiling.hpp>

auto inline thread_id_to_idx(u32 thread_id) -> u32 {
    for (u32 i = 0; i < TOTAL_THREAD_COUNT; i++) {
        if (Platform->thread_ids[i] == thread_id) {
            return i;
        }
    }
    InvalidCodePath;
    return 0;
}

DEBUG_FRAME_END(debug_frame_end) {
    u64 clock_start = __rdtsc();
    EngineState* state = (EngineState*)engine_memory->permanent;

    Assert(state->is_initialized);

    for (auto& thread_print_events : engine_memory->debug_print_events) {
        thread_print_events.clear();
    }

    auto& print_events = engine_memory->debug_print_events;
    i32 unclosed_print_events[TOTAL_THREAD_COUNT][1024] = {};
    i32 unclosed_print_event_count[TOTAL_THREAD_COUNT] = {};
    for (u32 i = 0; i < global_debug_table->event_index; i++) {
        DebugEvent* event = &global_debug_table->events[i];
        u32 thread_idx = thread_id_to_idx(event->thread_id);
        switch (event->event_type) {

        case DebugEventType_BeginFrame: {

            i32 print_event_index;
            PrintDebugEvent* print_event = print_events[thread_idx].pushi(&print_event_index);
            print_event->event_type = PrintDebugEventType_Frame;
            print_event->clock_start = event->clock;
            print_event->GUID = event->GUID;
            print_event->thread_idx = thread_id_to_idx(event->thread_id);

            unclosed_print_events[thread_idx][unclosed_print_event_count[thread_idx]++] = print_event_index;
        } break;
        case DebugEventType_EndFrame: {
            i32 print_event_index = unclosed_print_events[thread_idx][unclosed_print_event_count[thread_idx] - 1];
            unclosed_print_event_count[thread_idx]--;
            Assert(print_event_index == 0);
            Assert(unclosed_print_event_count[thread_idx] == 0);
            Assert(print_events[thread_idx][print_event_index].event_type == PrintDebugEventType_Frame);
            PrintDebugEvent* print_event = &print_events[thread_idx][print_event_index];
            print_event->clock_end = event->clock;
            Platform->frame_duration_ms = event->value_f32;
            Platform->frame_duration_clock_cycles = print_event->clock_end - print_event->clock_start;

        } break;
        case DebugEventType_BeginBlock: {
            i32 print_event_index;
            PrintDebugEvent* print_event = print_events[thread_idx].pushi(&print_event_index);
            print_event->event_type = PrintDebugEventType_Block;
            print_event->clock_start = event->clock;
            print_event->thread_idx = thread_id_to_idx(event->thread_id);
            print_event->GUID = event->GUID;
            if (unclosed_print_event_count[thread_idx] > 0) {
                print_event->parent_index = unclosed_print_events[thread_idx][unclosed_print_event_count[thread_idx] - 1];
                print_event->depth = print_events[thread_idx][print_event->parent_index].depth + 1;
            }
            unclosed_print_events[thread_idx][unclosed_print_event_count[thread_idx]++] = print_event_index;

        } break;
        case DebugEventType_EndBlock: {
            i32 print_event_index = unclosed_print_events[thread_idx][unclosed_print_event_count[thread_idx] - 1];
            unclosed_print_event_count[thread_idx]--;
            // Assert(print_event_index > 0);
            // Assert(unclosed_print_event_count[thread_idx] > 0);
            // Assert(print_events[thread_idx][print_event_index].event_type == PrintDebugEventType_Block);
            print_events[thread_idx][print_event_index].clock_end = event->clock;
        } break;
        case DebugEventType_Count:
        case DebugEventType_Unknown: {
            InvalidCodePath;
            break;
        }
        }
    }

    u64 clock_end = __rdtsc();
    PrintDebugEvent* debug_processing = print_events[0].push();
    debug_processing->GUID = DEBUG_NAME("process_debug_events");
    debug_processing->clock_start = clock_start;
    debug_processing->clock_end = clock_end;
    debug_processing->depth = 1;
}

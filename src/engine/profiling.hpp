#pragma once

#include <platform/platform.h>
#include <platform/types.h>

#include <core/list.hpp>
#include <core/memory_arena.h>

#include <math/vec2.h>

enum PrintDebugEventType {
    PrintDebugEventType_Nil = 0,
    PrintDebugEventType_Frame,
    PrintDebugEventType_TimeBlock,
    PrintDebugEventType_Thread,
};

struct PrintEventNode {
    PrintDebugEventType kind;
    u64 clock_start;
    u64 clock_end;
    const char* GUID;
    union {
        f32 value_f32;
        u32 value_u32;
        vec2 value_v2;
    };

    u32 parent_idx;
    u32 first_kid_idx;
    u32 last_kid_idx;
    u32 next_sib_idx;
};

struct DebugState {
    MemoryArena per_frame_arena;

    List<PrintEventNode> nodes;
    bool is_initialized;
};

enum DebugEventType {
    DebugEventType_Unknown = 0,
    DebugEventType_BeginBlock,
    DebugEventType_EndBlock,
    DebugEventType_Count,
};

struct DebugEvent {
    DebugEventType event_type;
    u64 clock;
    u32 thread_id;
    const char* GUID;
    union {
        f32 value_f32;
    };
};

constexpr i32 Debug_Max_Event_Count = 1 << 16;
struct DebugTable {
    // We store which table to use in the upmost 32 bits, to make the switch proper atomic, even though we "waste" 31 bits.
    volatile u64 event_index;
    u32 current_debug_table;
    DebugEvent events[2][Debug_Max_Event_Count];
};

extern DebugTable* global_debug_table;

inline auto record_debug_event(DebugTable* debug_table, const char* GUID, DebugEventType type) -> DebugEvent* {
    u64 index = atomic_add_u64(&global_debug_table->event_index, 1);
    u32 event_index = index & 0xFFFFFFFF;
    u32 array_index = index >> 32;
    Assert(index < Debug_Max_Event_Count);
    DebugEvent* event = &global_debug_table->events[array_index][event_index];
    event->GUID = GUID;
    event->event_type = type;
    event->clock = __rdtsc();
    event->thread_id = get_thread_id();
    return event;
}

#define UniqueFileCounterString__(A, B, C, D) A "|" #B "|" #C "|" D
#define UniqueFileCounterString_(A, B, C, D) UniqueFileCounterString__(A, B, C, D)
#define DEBUG_NAME(Name) UniqueFileCounterString_(__FILE__, __LINE__, __COUNTER__, Name)

#define TIMED_BLOCK__(GUID, Counter) TimedBlock TimedBlock_##Counter(GUID)
#define TIMED_BLOCK_(GUID) TIMED_BLOCK__(GUID, __COUNTER__)
#define TIMED_BLOCK(Name) TIMED_BLOCK_(DEBUG_NAME(Name))

#define RECORD_DEBUG_EVENT_(GUID, EventType) record_debug_event(global_debug_table, GUID, EventType);

#define BEGIN_BLOCK_(GUID) { RECORD_DEBUG_EVENT_(GUID, DebugEventType_BeginBlock) }
#define END_BLOCK_(GUID) { RECORD_DEBUG_EVENT_(GUID, DebugEventType_EndBlock) }

#define BEGIN_BLOCK(Name) BEGIN_BLOCK_(DEBUG_NAME(Name))
#define END_BLOCK() END_BLOCK_("END_BLOCK_")

struct TimedBlock {
    TimedBlock(const char* GUID) {
        BEGIN_BLOCK_(GUID);
    }

    ~TimedBlock() {
        END_BLOCK();
    }
};

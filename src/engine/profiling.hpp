#pragma once

#include <platform/platform.h>
#include <platform/types.h>

#include <core/list.hpp>
#include <core/memory_arena.h>
#include <engine/array.h>

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

constexpr u32 Nil_Index = 0;
constexpr u32 Historic_Frame_Count = 120;
constexpr u32 PrintEventNode_Count = Historic_Frame_Count * 1000;
struct PrintEventNodeForest {
    PrintEventNode nodes[Historic_Frame_Count * 1000];
    u32 current_idx;
    u32 curr_tree_node_count;
};

auto inline add_kid(PrintEventNodeForest* forest, u32 parent_idx = Nil_Index) -> u32 {
    forest->current_idx = (forest->current_idx + 1) % (PrintEventNode_Count);
    // Skip index 0, as that is the NIL node
    if (forest->current_idx == Nil_Index) {
        forest->current_idx++;
    }

    PrintEventNode* new_node = &forest->nodes[forest->current_idx];
    *new_node = {};
    new_node->parent_idx = parent_idx;
    // I.e. a new tree
    if (parent_idx == Nil_Index) {
        forest->curr_tree_node_count = 1;
    }
    else {
        PrintEventNode* parent_node = &forest->nodes[parent_idx];
        if (parent_node->first_kid_idx == Nil_Index) {
            parent_node->first_kid_idx = forest->current_idx;
        }
        else {
            PrintEventNode* left_sib = &forest->nodes[parent_node->last_kid_idx];
            left_sib->next_sib_idx = forest->current_idx;
        }
        parent_node->last_kid_idx = forest->current_idx;
    }
    forest->curr_tree_node_count++;
    return forest->current_idx;
}

struct DebugState {
    // MemoryArena permanent;
    u64 processed_frame_count;
    u64 current_inspecting_frame;
    PrintEventNodeForest node_forest;
    u32 historic_frame_indices[Historic_Frame_Count];

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

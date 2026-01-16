#include <platform/types.h>

#ifndef PROFILING_H_HUGIN
#define PROFILING_H_HUGIN

enum DebugEventType {
    DebugEventType_Unknown = 0,
    DebugEventType_BeginBlock,
    DebugEventType_EndBlock,
    DebugEventType_Count,
};

struct DebugEvent {
    DebugEventType event_type;
    u64 clock;
    const char* GUID;
};

struct DebugTable {
    u64 event_index;
    DebugEvent events[1 << 16];
};

extern DebugTable* global_debug_table;

#define UniqueFileCounterString__(A, B, C, D) A "|" #B "|" #C "|" D
#define UniqueFileCounterString_(A, B, C, D) UniqueFileCounterString__(A, B, C, D)
#define DEBUG_NAME(Name) UniqueFileCounterString_(__FILE__, __LINE__, __COUNTER__, Name)

/*#define TIMED_BLOCK__(GUID, Number, ...) TimedBlock TimedBlock_##Number(GUID, ##__VA_ARGS__)*/
/*#define TIMED_BLOCK_(GUID, Number, ...) TIMED_BLOCK__(GUID, Number, ##__VA_ARGS__)*/
/*#define TIMED_BLOCK(Name, ...) TIMED_BLOCK_(DEBUG_NAME(Name), __COUNTER__, ##__VA_ARGS__)*/
/*#define TIMED_FUNCTION(...) TIMED_BLOCK_(DEBUG_NAME(__FUNCTION__), ##__VA_ARGS__)*/

#define RECORD_DEBUG_EVENT_(GUIDInit, EventType)            \
    u64 index = ++global_debug_table->event_index;          \
    DebugEvent* event = &global_debug_table->events[index]; \
    event->GUID = GUIDInit;                                 \
    event->event_type = EventType;

#define BEGIN_BLOCK_(GUID) { RECORD_DEBUG_EVENT_(GUID, DebugEventType_BeginBlock) }
#define END_BLOCK_(GUID) { RECORD_DEBUG_EVENT_(GUID, DebugEventType_EndBlock) }

#define END_BLOCK() END_BLOCK_("END_BLOCK_")

struct TimedBlock {
    TimedBlock(const char* GUID) {
        BEGIN_BLOCK_(GUID);
    }

    ~TimedBlock() {
        END_BLOCK();
    }
};

#endif

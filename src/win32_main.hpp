#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef NOMINMAX

#include <platform/platform.hpp>

struct platform_work_queue_entry {
    platform_work_queue_callback* Callback;
    void* Data;
};

struct PlatformWorkQueue {
    u32 volatile completion_goal;
    u32 volatile completion_count;

    u32 volatile NextEntryToWrite;
    u32 volatile NextEntryToRead;

    // Worker threads can safely sleep until this tick.
    i64 target_sleep_tick;
    HANDLE SemaphoreHandle;

    platform_work_queue_entry entries[256];
};

struct Win32ThreadContext {
    i32 thread_id;
    i32 thread_idx;
    PlatformWorkQueue* queue;
    MemoryArena arena;
};

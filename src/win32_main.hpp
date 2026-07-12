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

struct win32_loaded_dll {
    LPCWSTR filename;
    LPCWSTR pdb_filename;
    LPCWSTR path;
    LPCWSTR copy_path;
    LPCWSTR pdb_path;

    i32 function_count;
    const char** function_names;
    void** functions;

    HMODULE handle;
    bool is_valid;
    FILETIME last_loaded_dll_write_time = { 0, 0 };
};

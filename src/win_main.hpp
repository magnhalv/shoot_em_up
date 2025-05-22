#define NOMINMAX
#include <Windows.h>

#include <platform/platform.h>

struct platform_work_queue_entry {
    platform_work_queue_callback* Callback;
    void* Data;
};

struct PlatformWorkQueue {
    u32 volatile completion_goal;
    u32 volatile completion_count;

    u32 volatile NextEntryToWrite;
    u32 volatile NextEntryToRead;
    HANDLE SemaphoreHandle;

    platform_work_queue_entry entries[256];
};

struct win32_thread_startup {
    PlatformWorkQueue* queue;
};

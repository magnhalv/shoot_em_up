#define NOMINMAX
#include <Windows.h>

#include <platform/platform.h>

struct platform_work_queue_entry {
  platform_work_queue_callback* Callback;
  void* Data;
};

struct platform_work_queue {
  u32 volatile CompletionGoal;
  u32 volatile CompletionCount;

  u32 volatile NextEntryToWrite;
  u32 volatile NextEntryToRead;
  HANDLE SemaphoreHandle;

  platform_work_queue_entry entries[256];
};

struct win32_thread_startup {
  platform_work_queue* queue;
};

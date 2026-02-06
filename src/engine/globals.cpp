#include <platform/platform.h>

#include <engine/globals.hpp>

TaskSystem* Task_System = nullptr;
DebugTable* global_debug_table = nullptr;

auto load(PlatformApi* in_platform) -> void {
    Platform = in_platform;
}

auto load(TaskSystem* task_system) -> void {
    Task_System = task_system;
}

auto begin_task(TaskSystem* task_system) -> TaskWithMemory* {
    TaskWithMemory* result = nullptr;
    for (u64 i = 0; i < array_length(task_system->tasks); i++) {
        TaskWithMemory* task = task_system->tasks + i;
        if (!task->in_use) {
            task->in_use = true;
            task->memory->clear_to_zero();
            result = task;
        }
    }
    return result;
}
auto end_task(TaskWithMemory* task) -> void {
    Assert(task->in_use);

    // TODO: CompletePreviousWrites??
    task->in_use = false;
}

#include "engine/globals.hpp"
#include "platform/platform.h"

PlatformApi* Platform = nullptr;
Options* g_graphics_options = nullptr;
TaskSystem* Task_System = nullptr;

auto load(PlatformApi* in_platform) -> void {
    Platform = in_platform;
}

auto load(Options* options) -> void {
    g_graphics_options = options;
}

auto load(TaskSystem* task_system) -> void {
    Task_System = task_system;
}

auto begin_task(TaskSystem* task_system) -> TaskWithMemory* {
    TaskWithMemory* result = nullptr;
    for (auto i = 0; i < array_length(task_system->tasks); i++) {
        TaskWithMemory* task = task_system->tasks + i;
        if (!task->in_use) {
            task->in_use = true;
            task->memory->clear();
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

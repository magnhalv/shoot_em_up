#pragma once

#include <platform/platform.h>

struct TaskWithMemory {
    bool in_use;
    MemoryArena* memory;
};

struct TaskSystem {
    PlatformWorkQueue* queue;
    TaskWithMemory tasks[4];
};

extern PlatformApi* Platform;
extern TaskSystem* Task_System;

global_variable DebugTable* global_debug_table = nullptr;

void load(PlatformApi* platform);

void load(TaskSystem* task_system);
auto begin_task(TaskSystem* task_system) -> TaskWithMemory*;
auto end_task(TaskWithMemory* task) -> void;

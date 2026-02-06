#pragma once

#include <engine/profiling.hpp>
#include <platform/platform.h>

#include <core/memory_arena.h>

struct TaskWithMemory {
    bool in_use;
    MemoryArena* memory;
};

struct TaskSystem {
    PlatformWorkQueue* queue;
    TaskWithMemory tasks[4];
};

extern TaskSystem* Task_System;
extern DebugTable* global_debug_table;

void load(PlatformApi* platform);

void load(TaskSystem* task_system);
auto begin_task(TaskSystem* task_system) -> TaskWithMemory*;
auto end_task(TaskWithMemory* task) -> void;

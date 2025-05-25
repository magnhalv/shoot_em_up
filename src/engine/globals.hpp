#pragma once

#include <platform/platform.h>

#include <engine/options.hpp>

struct TaskWithMemory {
    bool in_use;
    MemoryArena* memory;
};

struct TaskSystem {
    PlatformWorkQueue* queue;
    TaskWithMemory tasks[4];
};

global_variable PlatformApi* Platform;
global_variable Options* g_graphics_options;
global_variable TaskSystem* Task_System;

void load(PlatformApi* platform);
void load(Options* platform);

void load(TaskSystem* task_system);
auto begin_task(TaskSystem* task_system) -> TaskWithMemory*;
auto end_task(TaskWithMemory* task) -> void;

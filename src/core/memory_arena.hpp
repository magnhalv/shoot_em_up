#pragma once

#include "platform/assert.hpp"
#include <platform/types.hpp>

enum ArenaPushFlag : u32 {
    ArenaPushFlag_ClearToZero = 0x1,
};

struct ArenaPushParams {
    u32 alignment;
    u32 flags;
};

auto constexpr DefaultArenaParams() -> ArenaPushParams {
    ArenaPushParams result = {};
    result.alignment = 4;
    result.flags = ArenaPushFlag_ClearToZero;
    return result;
}

auto constexpr DoNotClearArenaParams() -> ArenaPushParams {
    ArenaPushParams result = {};
    result.alignment = 4;
    result.flags = 0;
    return result;
}

struct MemoryArena {
    u8* memory;
    Size size;
    Size capacity;
    i32 temp_count;

    auto init(void* in_memory, u64 in_size) -> void;
    auto allocate(u64 request_size, ArenaPushParams params = DefaultArenaParams()) -> void*;
    auto shrink(void* memory, u64 size) -> void;
    auto allocate_arena(u64 request_size) -> MemoryArena*;
    auto clear() -> void;
    auto clear_to_zero() -> void;
};

auto inline allocate_arena(void* memory, Size size) -> MemoryArena* {
    u8* data = (u8*)memory;
    MemoryArena* result = (MemoryArena*)data;
    result->init(data + sizeof(MemoryArena), size - sizeof(MemoryArena));
    return result;
}

#define PushArray(Arena, Count, Type, ...) \
    ((Type*)((Arena)->allocate(sizeof(Type) * (Count)__VA_OPT__(, ) __VA_ARGS__)))

template <typename T>
auto inline allocate(MemoryArena& arena, u64 count = 1, ArenaPushParams params = DefaultArenaParams()) -> T* {
    return static_cast<T*>(arena.allocate(sizeof(T) * count, params));
}

template <typename T>
auto inline allocate(MemoryArena* arena, u64 count = 1, ArenaPushParams params = DefaultArenaParams()) -> T* {
    return static_cast<T*>(arena->allocate(sizeof(T) * count, params));
}

struct Temp {
    MemoryArena* arena;
    Size size;
};

auto inline temp_begin(MemoryArena* arena) -> Temp {
    Temp temp = {};
    temp.arena = arena;
    temp.size = arena->size;
    temp.arena->temp_count++;
    return temp;
}

auto inline temp_end(Temp temp) {
    Assert(temp.arena);
    Assert(temp.size <= temp.arena->size);
    temp.arena->size = temp.size;
    temp.arena->temp_count--;
}

// TODO: Remove, use get_scratch
extern MemoryArena* g_transient; // This one is erased every frame.

void set_transient_arena(MemoryArena* arena);
auto debug_arena() -> MemoryArena;
void clear_transient();
void unset_transient_arena();

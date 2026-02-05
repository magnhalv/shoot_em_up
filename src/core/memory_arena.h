#pragma once

#include <platform/types.h>

const u32 SENTINEL_PATTERN = 0xEFBEADDE; // DEADBEEF

struct MemorySentinel {
    u32 sentinel_pattern;
    u64 block_size;
};

struct MemoryArena {
    u8* m_memory = nullptr;
    u64 m_used = 0;
    u64 m_size = 0;
    MemorySentinel* m_last = nullptr; // perhaps rather keep track of the last block?

    auto init(void* in_memory, u64 in_size) -> void;
    auto allocate(u64 request_size) -> void*;
    auto shrink(void* memory, u64 size) -> void;
    auto allocate_arena(u64 request_size) -> MemoryArena*;
    auto clear_to_zero() -> void;
    auto check_integrity() const -> void;
};

template <typename T> auto inline allocate(MemoryArena& arena, u64 count = 1) -> T* {
    return static_cast<T*>(arena.allocate(sizeof(T) * count));
}

template <typename T> auto inline allocate(MemoryArena* arena, u64 count = 1) -> T* {
    return static_cast<T*>(arena->allocate(sizeof(T) * count));
}

extern MemoryArena* g_transient; // This one is erased every frame.

void set_transient_arena(MemoryArena* arena);
void clear_transient();
void unset_transient_arena();

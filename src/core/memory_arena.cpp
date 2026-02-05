#include <cassert>
#include <cinttypes>
#include <cstring>

#include <core/logger.h>
#include <core/memory.h>

#include <engine/hm_assert.h>

#include "memory_arena.h"

MemoryArena* g_transient = nullptr;

auto MemoryArena::allocate(u64 request_size) -> void* {
    Assert(m_memory);
    if (m_size < m_used + request_size + sizeof(MemorySentinel)) {
        crash_and_burn("Failed to allocate %" PRIu64 " bytes. Only %" PRIu64 " remaining.", request_size,
            m_size - m_used - sizeof(MemorySentinel));
    }

    auto* previous_guard = reinterpret_cast<MemorySentinel*>(&m_memory[m_used - sizeof(MemorySentinel)]);
    HM_ASSERT(previous_guard->sentinel_pattern == SENTINEL_PATTERN);

    memset(&m_memory[m_used], 0, request_size);

    void* result = &m_memory[m_used];
    m_used += request_size + sizeof(MemorySentinel);

    auto* new_guard = reinterpret_cast<MemorySentinel*>(&m_memory[m_used - sizeof(MemorySentinel)]);
    new_guard->sentinel_pattern = SENTINEL_PATTERN;
    new_guard->block_size = 0;

    previous_guard->block_size = request_size;
    m_last = new_guard;

    // log_info("MemoryArena: allocated %llu bytes. Capacity: %.2f %%.", request_size,
    //   (static_cast<f32>(used) * 100.0f) / static_cast<f32>(size));

    return result;
}

auto MemoryArena::shrink(void* block_in, u64 reduction_size) -> void {
    u8* block = static_cast<u8*>(block_in);
    MemorySentinel* previous_guard = (MemorySentinel*)(block - sizeof(MemorySentinel));
    MemorySentinel* last_guard = (MemorySentinel*)(block + previous_guard->block_size);
    if (last_guard->block_size != 0) {
        crash_and_burn("MemoryArena: Tried to shrink memory block that is not the final block.");
    }

    if (reduction_size > previous_guard->block_size) {
        crash_and_burn("MemoryArena: Tried to shrink memory block passed itself.");
    }
    m_used -= reduction_size;
    previous_guard->block_size -= reduction_size;
    memset(m_memory + m_used, 0, reduction_size);
    MemorySentinel* new_last_sentinel = (MemorySentinel*)(m_memory + m_used - sizeof(MemorySentinel));
    new_last_sentinel->block_size = 0;
    new_last_sentinel->sentinel_pattern = SENTINEL_PATTERN;
}

auto MemoryArena::allocate_arena(u64 request_size) -> MemoryArena* {
    void* mem_block = static_cast<u8*>(allocate(request_size + sizeof(MemoryArena)));
    auto* new_arena = static_cast<MemoryArena*>(mem_block);
    new_arena->init(static_cast<u8*>(mem_block) + sizeof(MemoryArena), request_size);
    return new_arena;
}

auto MemoryArena::clear_to_zero() -> void {
    clear_memory(m_memory, m_size);

    m_used = sizeof(MemorySentinel);
    auto* first_guard = reinterpret_cast<MemorySentinel*>(m_memory);
    first_guard->sentinel_pattern = SENTINEL_PATTERN;
    first_guard->block_size = 0;
    m_last = first_guard;
}

auto MemoryArena::check_integrity() const -> void {
    u8* memory = m_memory;
    MemorySentinel* guard = (MemorySentinel*)(memory);

    if (guard == nullptr) {
        crash_and_burn("MemoryArena: is not initialized. Always initialize arenas before use!");
    }

    i32 guard_index = 0;
    if (guard->sentinel_pattern != SENTINEL_PATTERN) {
        crash_and_burn("MemoryArena: integrity check failed at guard index %d", guard_index);
    }

    while (guard->block_size != 0) {
        memory += sizeof(MemorySentinel) + guard->block_size;
        guard = (MemorySentinel*)memory;
        guard_index++;
        if (guard->sentinel_pattern != SENTINEL_PATTERN) {
            crash_and_burn("MemoryArena: integrity check failed at guard index %d", guard_index);
        }

        if (memory > m_memory + m_size) {
            crash_and_burn("MemoryArena: integrity check failed. We went passed the capacity");
        }
    }
}

auto MemoryArena::init(void* in_memory, u64 in_size) -> void {
    m_memory = (u8*)in_memory;
    m_size = in_size;
    clear_to_zero();
}

void set_transient_arena(MemoryArena* arena) {
    assert(arena->m_memory != nullptr);
    assert(g_transient == nullptr);
    g_transient = arena;
}

void unset_transient_arena() {
    g_transient = nullptr;
}

void clear_transient() {
    assert(g_transient != nullptr);
    g_transient->clear_to_zero();
}

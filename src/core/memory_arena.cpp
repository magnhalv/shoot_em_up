#include <cassert>
#include <cinttypes>
#include <cstring>

#include <platform/platform.h>

#include <core/logger.h>
#include <core/memory.h>

#include <engine/hm_assert.h>

#include "memory_arena.h"

MemoryArena* g_transient = nullptr;

auto is_power_of_two(u32 value) -> bool {
    return value && ((value & (value - 1)) == 0);
}

auto MemoryArena::allocate(u64 request_size, ArenaPushParams params) -> void* {
    Assert(is_power_of_two(params.alignment));
    Assert(m_memory);

    MemorySentinel* previous_sentinel = (MemorySentinel*)(m_memory + m_size - sizeof(MemorySentinel));
    HM_ASSERT(previous_sentinel->sentinel_pattern == SENTINEL_PATTERN);

    u64 alignment_mask = params.alignment - 1;
    const u64 padding_size = 1;
    u64 base = (u64)m_memory + m_size;
    u64 base_with_padding = base + padding_size;
    u64 aligned_address = (base_with_padding + alignment_mask) & ~(alignment_mask);
    u64 padding = aligned_address - base;

    u64 block_size = padding + request_size;
    u64 total_size = block_size + sizeof(MemorySentinel);

    if (m_capacity < m_size + total_size) {
        crash_and_burn("Failed to allocate %" PRIu64 " bytes. Only %" PRIu64 " remaining.", total_size, m_capacity - m_size);
    }

    memset(m_memory + m_size, 0, total_size);

    void* result = (void*)aligned_address;
    m_size += total_size;

    // Store how much padding was added in the byte previous to the returned address.
    u8* padding_address = (u8*)(aligned_address)-1;
    Assert(padding <= 255);
    *padding_address = (u8)padding;

    auto* new_sentinel = (MemorySentinel*)(m_memory + m_size - sizeof(MemorySentinel));
    new_sentinel->sentinel_pattern = SENTINEL_PATTERN;
    new_sentinel->block_size = 0;

    previous_sentinel->block_size = block_size;
    m_last = new_sentinel;

    return result;
}

auto MemoryArena::shrink(void* aligned_block_in, u64 reduction_size) -> void {
    u8* aligned_block = (u8*)aligned_block_in;
    u8 padding = *((u8*)aligned_block - 1);
    MemorySentinel* previous_guard = (MemorySentinel*)(aligned_block - padding - sizeof(MemorySentinel));
    MemorySentinel* last_guard = (MemorySentinel*)(aligned_block - padding + previous_guard->block_size);

    Assert(previous_guard->sentinel_pattern == SENTINEL_PATTERN);
    Assert(last_guard->sentinel_pattern == SENTINEL_PATTERN);
    if (last_guard->block_size != 0) {
        crash_and_burn("MemoryArena: Tried to shrink memory block that is not the final block.");
    }

    if (reduction_size > previous_guard->block_size) {
        crash_and_burn("MemoryArena: Tried to shrink memory block passed itself.");
    }
    m_size -= reduction_size;
    previous_guard->block_size -= reduction_size;
    memset(m_memory + m_size, 0, reduction_size);
    MemorySentinel* new_last_sentinel = (MemorySentinel*)(m_memory + m_size - sizeof(MemorySentinel));
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
    clear_memory(m_memory, m_capacity);
    m_size = sizeof(MemorySentinel);
    MemorySentinel* first_guard = (MemorySentinel*)(m_memory);
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

        if (memory > m_memory + m_capacity) {
            crash_and_burn("MemoryArena: integrity check failed. We went passed the capacity");
        }
    }
}

auto MemoryArena::init(void* in_memory, u64 in_size) -> void {
    m_memory = (u8*)in_memory;
    m_capacity = in_size;
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

#include <cassert>
#include <cinttypes>
#include <cstdlib>
#include <cstring>

#include <core/logger.hpp>
#include <core/memory.hpp>

#include <engine/hm_assert.hpp>

#include "core/util.hpp"
#include "memory_arena.hpp"
#include "platform/types.hpp"

MemoryArena* g_transient = nullptr;

internal auto is_power_of_two(u32 value) -> bool {
    return value && ((value & (value - 1)) == 0);
}

auto debug_arena() -> MemoryArena {
    const u64 bytes = MegaBytes(1);
    MemoryArena arena = {};
    arena.init(calloc(sizeof(u8), bytes), bytes);
    return arena;
}

auto MemoryArena::allocate(u64 request_size, ArenaPushParams params) -> void* {
    Assert(is_power_of_two(params.alignment));
    Assert(memory);

    Size alignment_mask = params.alignment - 1;
    Size base = (Size)memory + size;
    Size aligned_address = (base + alignment_mask) & ~(alignment_mask);
    Size padding = aligned_address - base;

    Size block_size = padding + request_size;

    if (capacity < size + block_size) {
        MemoryArena local_debug_arena = debug_arena();
        CString8 total_size_formatted = format_bytes(block_size, local_debug_arena);
        CString8 remaning_formatted = format_bytes(capacity - size, local_debug_arena);
        crash_and_burn("Failed to allocate %s. Only %s remaining.", total_size_formatted.data, remaning_formatted.data);
    }

    if (params.flags & ArenaPushFlag_ClearToZero) {
        memset(memory + size, 0, block_size);
    }

    void* result = (void*)aligned_address;
    size += block_size;

    return result;
}

auto MemoryArena::shrink(void* aligned_block_in, u64 reduction_size) -> void {
    size -= reduction_size;
}

auto MemoryArena::allocate_arena(u64 request_size) -> MemoryArena* {
    ArenaPushParams params = DefaultArenaParams();
    params.alignment = alignof(MemoryArena);
    void* mem_block = static_cast<u8*>(allocate(request_size + sizeof(MemoryArena), params));
    auto* new_arena = static_cast<MemoryArena*>(mem_block);
    new_arena->init(static_cast<u8*>(mem_block) + sizeof(MemoryArena), request_size);
    return new_arena;
}

auto MemoryArena::clear() -> void {
    size = 0;
}

auto MemoryArena::clear_to_zero() -> void {
    clear_memory(memory, capacity);
}

auto MemoryArena::init(void* in_memory, u64 in_size) -> void {
    size = 0;
    temp_count = 0;
    memory = (u8*)in_memory;
    capacity = in_size;
    clear_to_zero();
}

void set_transient_arena(MemoryArena* arena) {
    assert(arena->memory != nullptr);
    assert(g_transient == nullptr);
    g_transient = arena;
}

void unset_transient_arena() {
    g_transient = nullptr;
}

void clear_transient() {
    assert(g_transient != nullptr);
    g_transient->clear();
}

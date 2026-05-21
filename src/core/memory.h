#pragma once

#include <core/logger.h>
#include <platform/types.h>

auto initialize_memory_lib() -> void;

#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
#define ZeroArray(Count, Pointer) ZeroSize(Count * sizeof((Pointer)[0]), Pointer)
inline void ZeroSize(memory_index Size, void* Ptr) {
    u8* Byte = (u8*)Ptr;
    while (Size--) {
        *Byte++ = 0;
    }
}

// Alignment in bytes
auto inline is_aligned(void* memory, u64 alignment) -> bool {
    return (((uintptr_t)memory) & (alignment - 1)) == 0;
}

typedef void (*copy_memory_fn)(void*, void*, u64);
inline void copy_memory_no_init(void* src, void* dest, u64 size) {
    crash_and_burn("copy_memory has not been initialized. Call initialize_core_lib() first.");
}
global_variable copy_memory_fn copy_memory = copy_memory_no_init;

void clear_memory(void* memory, u64 size);

void copy_memory_AVX2(void* src, void* dest, u64 size);
void copy_memory_scalar(void* src, void* dest, u64 size);

void set_memory_u32(void* dest, u32 value, u64 count);

void DEBUG_print_memory_as_hex(void* memory, u64 size);

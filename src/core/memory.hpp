#pragma once

#include <platform/types.hpp>

#include <core/logger.hpp>

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
void copy_memory_no_init(void* src, void* dest, u64 size);
global_variable copy_memory_fn copy_memory = copy_memory_no_init;
void copy_memory_AVX2(void* src, void* dest, u64 size);
void copy_memory_scalar(void* src, void* dest, u64 size);

typedef void (*set_memory_u32_fn)(u32*, u32, u64);
void set_memory_u32_init(u32* dest, u32 value, u64 count);
void set_memory_u32_scalar(u32* dest, u32 value, u64 count);
void set_memory_u32_avx512(u32* dest, u32 value, u64 count);
global_variable set_memory_u32_fn set_memory_u32 = set_memory_u32_init;

void clear_memory(void* memory, u64 size);

void DEBUG_print_memory_as_hex(void* memory, u64 size);

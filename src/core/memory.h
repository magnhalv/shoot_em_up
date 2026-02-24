#pragma once

#include <platform/types.h>

#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
#define ZeroArray(Count, Pointer) ZeroSize(Count * sizeof((Pointer)[0]), Pointer)
inline void ZeroSize(memory_index Size, void* Ptr) {
    u8* Byte = (u8*)Ptr;
    while (Size--) {
        *Byte++ = 0;
    }
}

// Alignment in bytes
auto inline is_aligned(void* memory, u32 alignment) -> bool {
  return (((uintptr_t)memory) & (alignment - 1)) == 0;
}

void clear_memory(void* memory, u64 size);
void copy_memory(void* src, void* dest, u64 size);

void DEBUG_print_memory_as_hex(void* memory, u64 size);

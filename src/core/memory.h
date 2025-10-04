#pragma once

#include <platform/types.h>

#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
#define ZeroArray(Count, Pointer) ZeroSize(Count * sizeof((Pointer)[0]), Pointer)
inline void ZeroSize(memory_index Size, void* Ptr) {
    // TODO(casey): Check this guy for performance
    u8* Byte = (u8*)Ptr;
    while (Size--) {
        *Byte++ = 0;
    }
}

void debug_set_memory(void* memory, u64 size);
void copy_memory_from_to(void* src, void* dest, u64 size);

void DEBUG_print_memory_as_hex(void* memory, u64 size);

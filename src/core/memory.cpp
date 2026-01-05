#include <cassert>
#include <cstdio>
#include <cstring>

#include "memory.h"

void clear_memory(void* memory, u64 size) {
    assert(size % 4 == 0);
    memset(memory, 0, size);
}

auto copy_memory(void* src, void* dest, u64 size) -> void {
    u8* s = (u8*)src;
    u8* d = (u8*)dest;
    for (auto i = 0; i < size; i++) {
        *(d + i) = *(s + i);
    }
}

void DEBUG_print_memory_as_hex(void* memory, u64 size) {
    u32* data = (u32*)memory;
    for (auto i = 0; i < size; i++) {
        printf("0x%08x\n", data[0]);
    }
}

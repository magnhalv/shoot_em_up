#include <cassert>
#include <cstdio>
#include <cstring>

#include <platform/platform.h>

#include "memory.h"

void clear_memory(void* memory, u64 size) {
    assert(size % 4 == 0);
    memset(memory, 0, size);
}

auto copy_memory_slow(void* src, void* dest, u64 size) -> void {
    u8* s = (u8*)src;
    u8* d = (u8*)dest;
    for (u64 i = 0; i < size; i++) {
        *(d + i) = *(s + i);
    }
}

auto copy_memory(void* src, void* dest, u64 size) -> void {
    u8* s = (u8*)src;
    u8* d = (u8*)dest;
    const i32 byte_width = 32;
    if (byte_width <= size) {
        const u64 vectorazible_size = size - byte_width;
        for (u64 i = 0; i <= vectorazible_size; i += byte_width) {
            __m256i data = _mm256_loadu_si256((const __m256i*)s);
            _mm256_storeu_si256((__m256i*)d, data);
            s += byte_width;
            d += byte_width;
        }
    }

    i32 remainder = size % 32;
    for (i32 i = 0; i < remainder; i++) {
        *(d++) = *(s++);
    }
}

void DEBUG_print_memory_as_hex(void* memory, u64 size) {
    u32* data = (u32*)memory;
    for (u64 i = 0; i < size; i++) {
        printf("0x%08x\n", data[0]);
    }
}

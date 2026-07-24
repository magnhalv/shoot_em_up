#include <cassert>
#include <cstdio>
#include <cstring>

#include <math/simd.hpp>
#include <platform/platform.hpp>

#include "memory.hpp"

auto initialize_memory_lib() -> void {
    if (cpu_supports_avx512f()) {
        // TODO: Implement AVX512
        copy_memory = copy_memory_AVX2;
    }
    else if (cpu_supports_avx2()) {
        copy_memory = copy_memory_AVX2;
    }
    else {
        copy_memory = copy_memory_scalar;
    }
}

void clear_memory(void* memory, u64 size) {
    assert(size % 4 == 0);
    memset(memory, 0, size);
}

auto copy_memory_scalar(void* src, void* dest, u64 size) -> void {
    u8* s = (u8*)src;
    u8* d = (u8*)dest;
    for (u64 i = 0; i < size; i++) {
        *(d + i) = *(s + i);
    }
}

auto copy_memory_AVX2(void* src, void* dest, u64 size) -> void {
    // AVX
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

auto set_memory_u32_init(u32* dest, u32 value, u64 count) -> void {
    if (cpu_supports_avx512f()) {
        set_memory_u32 = &set_memory_u32_avx512;
    }
    else {
        set_memory_u32 = &set_memory_u32_scalar;
    }
    set_memory_u32(dest, value, count);
}

auto set_memory_u32_scalar(u32* dest, u32 value, u64 count) -> void {
    u32* dest_u32 = (u32*)dest;
    for (u64 i = 0; i < count; i++) {
        dest_u32[i] = value;
    }
}

auto set_memory_u32_avx512(u32* dest, u32 value, u64 count) -> void {
    const i32 lane_count = 16;
    i32x16 value_v16 = _mm512_set1_epi32(value);
    u64 i = 0;
    for (; (i + lane_count) <= count; i += lane_count) {
        _mm512_storeu_si512((__m512i*)(dest + i), value_v16);
    }

    for (; i < count; i++) {
        *(dest + i) = value;
    }
}

void DEBUG_print_memory_as_hex(void* memory, u64 size) {
    u32* data = (u32*)memory;
    for (u64 i = 0; i < size; i++) {
        printf("0x%08x\n", data[0]);
    }
}

void copy_memory_no_init(void* src, void* dest, u64 size) {
    crash_and_burn("copy_memory has not been initialized. Call initialize_core_lib() first.");
}

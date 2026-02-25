#pragma once

#include <platform/platform.h>
#include <platform/types.h>

// Reverse of _MM_SHUFFLE_, as I find that more intuitive
#define SHUFFLE_MASK(a, b, c, d) (((d) << 6) | ((c) << 4) | ((b) << 2) | (a))

auto inline dot(__m256 ax, __m256 ay, __m256 bx, __m256 by) -> __m256 {
    __m256 x = _mm256_mul_ps(ax, bx);
    __m256 y = _mm256_mul_ps(ay, by);
    return _mm256_add_ps(x, y);
}

auto inline clamp_i32_v8(__m256i min, __m256i values, __m256i max) -> __m256i {
    values = _mm256_max_epi32(min, values);
    values = _mm256_min_epi32(max, values);
    return values;
}

auto inline clamp_i32_v8(i32 min, __m256i values, i32 max) -> __m256i {
    __m256i min_v8 = _mm256_set1_epi32(min);
    __m256i max_v8 = _mm256_set1_epi32(max);
    values = _mm256_max_epi32(min_v8, values);
    values = _mm256_min_epi32(max_v8, values);
    return values;
}

auto inline clamp_f32_v8(__m256 min, __m256 values, __m256 max) -> __m256 {
    values = _mm256_max_ps(min, values);
    values = _mm256_min_ps(max, values);
    return values;
}

auto inline clamp_f32_v8(f32 min, __m256 values, f32 max) -> __m256 {
    __m256 min_v8 = _mm256_set1_ps(min);
    __m256 max_v8 = _mm256_set1_ps(max);
    values = _mm256_max_ps(min_v8, values);
    values = _mm256_min_ps(max_v8, values);
    return values;
}

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

auto inline lerp(__m256 A_v8, __m256 t_v8, __m256 B_v8) -> __m256 {
    __m256 one = _mm256_set1_ps(1.0f);
    __m256 A_part_v8 = _mm256_mul_ps(A_v8, _mm256_sub_ps(one, t_v8));
    __m256 B_part_v8 = _mm256_mul_ps(B_v8, t_v8);
    return _mm256_add_ps(A_part_v8, B_part_v8);
}

struct color_v8 {
    __m256 r;
    __m256 g;
    __m256 b;
    __m256 a;
};

auto inline lerp(color_v8 A, __m256 t_v8, color_v8 B) -> color_v8 {
    color_v8 result;
    result.r = lerp(A.r, t_v8, B.r);
    result.g = lerp(A.g, t_v8, B.g);
    result.b = lerp(A.b, t_v8, B.b);
    result.a = lerp(A.a, t_v8, B.a);
    return result;
}

inline auto pack4x8_linear1_to_srgb255(color_v8 color) -> __m256i {
    const __m256 zero = _mm256_setzero_ps();
    const __m256 one = _mm256_set1_ps(1.0f);
    const __m256 n255 = _mm256_set1_ps(255.0f);

    // Clamp to [0,1] before sqrt to avoid NaN from negative values
    __m256 r = _mm256_min_ps(_mm256_max_ps(color.r, zero), one);
    __m256 g = _mm256_min_ps(_mm256_max_ps(color.g, zero), one);
    __m256 b = _mm256_min_ps(_mm256_max_ps(color.b, zero), one);
    __m256 a = _mm256_min_ps(_mm256_max_ps(color.a, zero), one);

    // sqrt approximates linear->sRGB gamma (1/2.0 vs exact 1/2.2, good enough)
    __m256i ri = _mm256_cvtps_epi32(_mm256_mul_ps(_mm256_sqrt_ps(r), n255));
    __m256i gi = _mm256_cvtps_epi32(_mm256_mul_ps(_mm256_sqrt_ps(g), n255));
    __m256i bi = _mm256_cvtps_epi32(_mm256_mul_ps(_mm256_sqrt_ps(b), n255));
    __m256i ai = _mm256_cvtps_epi32(_mm256_mul_ps(a, n255)); // alpha is linear

    __m256i result = _mm256_or_si256(                                          //
        _mm256_or_si256(_mm256_slli_epi32(ai, 24), _mm256_slli_epi32(ri, 16)), //
        _mm256_or_si256(_mm256_slli_epi32(gi, 8), bi)                          //
    );
    return result;
}

auto inline blend_color_v8(color_v8 dest, color_v8 src) -> color_v8 {
    // Cout = (A * A.a) + (B * (1 - A.a))
    color_v8 blended;
    blended.r = _mm256_mul_ps(src.r, src.a);
    blended.g = _mm256_mul_ps(src.g, src.a);
    blended.b = _mm256_mul_ps(src.b, src.a);
    blended.a = _mm256_mul_ps(src.a, src.a);
    __m256 one_minus_a = _mm256_sub_ps(_mm256_set1_ps(1.0f), src.a);

    blended.r = _mm256_add_ps(blended.r, _mm256_mul_ps(dest.r, one_minus_a));
    blended.g = _mm256_add_ps(blended.g, _mm256_mul_ps(dest.g, one_minus_a));
    blended.b = _mm256_add_ps(blended.b, _mm256_mul_ps(dest.b, one_minus_a));
    blended.a = _mm256_add_ps(blended.a, _mm256_mul_ps(dest.a, one_minus_a));

    return blended;
}
auto inline get_color(__m256i packed_color_v8, f32* srgb255_to_linear_lut) -> color_v8 {
    const __m256i maskFF = _mm256_set1_epi32(0xFF);
    const __m256 inv255 = _mm256_set1_ps(1.0f / 255.0f);

    __m256 r = _mm256_mul_ps(_mm256_cvtepi32_ps(_mm256_and_si256(_mm256_srli_epi32(packed_color_v8, 16), maskFF)), inv255);
    __m256 g = _mm256_mul_ps(_mm256_cvtepi32_ps(_mm256_and_si256(_mm256_srli_epi32(packed_color_v8, 8), maskFF)), inv255);
    __m256 b = _mm256_mul_ps(_mm256_cvtepi32_ps(_mm256_and_si256(packed_color_v8, maskFF)), inv255);
    __m256 a = _mm256_mul_ps(_mm256_cvtepi32_ps(_mm256_srli_epi32(packed_color_v8, 24)), inv255);

    color_v8 result;
    result.r = _mm256_mul_ps(r, r); // gamma 2.0 approximation
    result.g = _mm256_mul_ps(g, g);
    result.b = _mm256_mul_ps(b, b);
    result.a = a; // alpha is linear
    return result;
}

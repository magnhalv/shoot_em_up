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

inline auto pack4x8_linear1_to_srgb255(color_v8 color, u32* linear1_to_srgb255_lut) -> __m256i {
    // NOTE: Not safe if r, g, b, a is NOT [0.0, 1.0f]
    __m256 N255 = _mm256_set1_ps(255.0f);
    __m256i r_idx_v8 = _mm256_cvtps_epi32(_mm256_mul_ps(color.r, N255));
    __m256i g_idx_v8 = _mm256_cvtps_epi32(_mm256_mul_ps(color.g, N255));
    __m256i b_idx_v8 = _mm256_cvtps_epi32(_mm256_mul_ps(color.b, N255));
    __m256i a_idx_v8 = _mm256_cvtps_epi32(_mm256_mul_ps(color.a, N255));

    __m256i r = _mm256_i32gather_epi32((const i32*)linear1_to_srgb255_lut, r_idx_v8, sizeof(u32));
    __m256i g = _mm256_i32gather_epi32((const i32*)linear1_to_srgb255_lut, g_idx_v8, sizeof(u32));
    __m256i b = _mm256_i32gather_epi32((const i32*)linear1_to_srgb255_lut, b_idx_v8, sizeof(u32));
    __m256i a = _mm256_i32gather_epi32((const i32*)linear1_to_srgb255_lut, a_idx_v8, sizeof(u32));

    __m256i result = _mm256_or_si256(                                        //
        _mm256_or_si256(_mm256_slli_epi32(a, 24), _mm256_slli_epi32(r, 16)), //
        _mm256_or_si256(_mm256_slli_epi32(g, 8), b)                          //
    );

    return result;
}

auto inline get_color(__m256i packed_color_v8, f32* srgb255_to_linear_lut) -> color_v8 {
    const __m256i maskFF = _mm256_set1_epi32(0x000000FF);
    __m256i r_idx_v8 = _mm256_and_epi32(_mm256_srli_epi32(packed_color_v8, 16), maskFF);
    __m256i g_idx_v8 = _mm256_and_epi32(_mm256_srli_epi32(packed_color_v8, 8), maskFF);
    __m256i b_idx_v8 = _mm256_and_epi32(_mm256_srli_epi32(packed_color_v8, 0), maskFF);
    __m256i a_rgba255_v8 = _mm256_and_si256(_mm256_srli_epi32(packed_color_v8, 24), maskFF);

    color_v8 result;
    result.r = _mm256_i32gather_ps(srgb255_to_linear_lut, r_idx_v8, sizeof(f32));
    result.g = _mm256_i32gather_ps(srgb255_to_linear_lut, g_idx_v8, sizeof(f32));
    result.b = _mm256_i32gather_ps(srgb255_to_linear_lut, b_idx_v8, sizeof(f32));

    const __m256 inv255 = _mm256_set1_ps(1.0f / 255.0f);
    result.a = _mm256_mul_ps(_mm256_cvtepi32_ps(a_rgba255_v8), inv255);
    return result;
}

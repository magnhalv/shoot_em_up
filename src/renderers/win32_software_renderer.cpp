#include <immintrin.h>
#include <platform/platform.h>

#include <math/mat2.h>
#include <math/mat3.h>
#include <math/math.h>
#include <math/simd.hpp>
#include <math/util.hpp>

#include <core/logger.h>
#include <core/memory.h>
#include <core/memory_arena.h>
#include <core/sort.hpp>
// TODO: Move to core
#include <engine/hm_assert.h>
#include <engine/profiling.hpp>

#include <renderers/renderer.h>
#include <renderers/win32_renderer.h>

#include "../core/lib.cpp"
#include "../math/unit.cpp"
#include "platform/types.h"

struct WindowDimension {
    i32 width;
    i32 height;
};

struct OffscreenBuffer {
    BITMAPINFO Info;
    void* memory;
    i32 memory_size;
    i32 width;
    i32 height;
    i32 bytes_per_pixel;
    i32 pitch;
};

struct Win32Texture {
    void* data;
    i32 width;
    i32 height;
    i32 size;
    i32 count;
    i32 pitch;
    i32 bytes_per_pixel;
};

// TODO: This should probably go to an arena
struct SWRendererState {
    OffscreenBuffer global_offscreen_buffer;
    MemoryArena permanent;
    MemoryArena transient;

    Win32Texture textures[MaxTextureId];
};

static SWRendererState state = {};

DebugTable* global_debug_table = nullptr;

static WindowDimension get_window_dimension(HWND window) {
    WindowDimension result;
    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.width = clientRect.right - clientRect.left;
    result.height = clientRect.bottom - clientRect.top;
    return result;
}

static void resize_dib_section(OffscreenBuffer* buffer, int width, int height) {
    // TODO: Bulletproof this
    // Maybe don't free first, free after, then free first if that fails. I.e. if VirtualAlloc fails.
    if (buffer->memory) {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;

    buffer->Info.bmiHeader.biSize = sizeof(buffer->Info.bmiHeader);
    buffer->Info.bmiHeader.biWidth = buffer->width;
    buffer->Info.bmiHeader.biHeight = buffer->height;
    buffer->Info.bmiHeader.biPlanes = 1;
    buffer->Info.bmiHeader.biBitCount = 32;
    buffer->Info.bmiHeader.biCompression = BI_RGB;

    buffer->bytes_per_pixel = 4;
    buffer->memory_size = buffer->bytes_per_pixel * (buffer->width * buffer->height);
    buffer->memory = VirtualAlloc(0, buffer->memory_size, MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = buffer->width * buffer->bytes_per_pixel;

    // Should always be 64KB aligned from virtual alloc
    Assert(is_aligned(buffer->memory, 65536));
}

auto square(f32 v) -> f32 {
    return v * v;
}

auto square_root(f32 v) -> f32 {
    return sqrtf(v);
}

const u32 LUT_ENTRY_COUNT = 256;
global_variable f32 srgb255_to_linear_lut[LUT_ENTRY_COUNT];
global_variable u8 linear1_to_srgb255_lut[LUT_ENTRY_COUNT];

internal auto generate_luts() -> void {
    // Foundations of Game Engine Development: Rendering, page 19
    for (u32 i = 0; i < LUT_ENTRY_COUNT; i++) {
        f32 c = (f32)i;
        c = c / 255.0f;

        if (c <= 0.04045f) {
            c = c / 12.92f;
        }
        else {
            c = powf((c + 0.055f) / 1.055f, 2.4);
        }
        srgb255_to_linear_lut[i] = c;
    }

    for (u32 i = 0; i < LUT_ENTRY_COUNT; i++) {
        f32 c = (f32)i;
        c = c / 255.0f;

        if (c <= 0.0031308f) {
            c = c * 12.92f;
        }
        else {
            c = 1.055f * powf(c, 1.0f / 2.4f) - 0.055f;
        }
        c = c * 255.0f + 0.5f;
        c = clamp(c, 0.0f, 255.0f);
        linear1_to_srgb255_lut[i] = (u8)c;
    }
}

inline auto srgb255_to_linear1_lookup(f32 color) -> f32 {
    Assert(color >= 0.0f && color <= 255.0f);
    i32 index = (i32)color;
    return srgb255_to_linear_lut[index];
}

inline auto linear1_to_srgb255_lookup(f32 linear1) -> f32 {
    Assert(linear1 >= 0.0f && linear1 <= 1.0f);
    i32 index = (i32)(linear1 * 255.0f);
    return linear1_to_srgb255_lut[index];
}

inline auto unpack4x8(u32 packed) -> vec4 {
    vec4 result = {
        (f32)((packed >> 16) & 0xFF), //
        (f32)((packed >> 8) & 0xFF),  //
        (f32)((packed >> 0) & 0xFF),  //
        (f32)((packed >> 24) & 0xFF)  //
    };
    return result;
}

inline auto unpack4x8_srgb255_to_linear1(u32 packed) -> vec4 {
    u8 r = (packed >> 16) & 0xFF;
    u8 g = (packed >> 8) & 0xFF;
    u8 b = (packed >> 0) & 0xFF;
    u8 a = (packed >> 24) & 0xFF;

    vec4 result;

    result.r = srgb255_to_linear_lut[r];
    result.g = srgb255_to_linear_lut[g];
    result.b = srgb255_to_linear_lut[b];
    result.a = (f32)a / 255.0f;
    return result;
}

inline auto pack4x8_linear1_to_srgb255(vec4 color) -> u32 {
    // NOTE: Not clamping miight be dangerous here
    u32 r_idx = (u32)(color.r * 255.0f + 0.5f);
    u32 g_idx = (u32)(color.g * 255.0f + 0.5f);
    u32 b_idx = (u32)(color.b * 255.0f + 0.5f);
    u32 a = (u32)(color.a * 255.0f + 0.5f);
    u32 result =                              //
        a << 24 |                             //
        linear1_to_srgb255_lut[r_idx] << 16 | //
        linear1_to_srgb255_lut[g_idx] << 8 |  //
        linear1_to_srgb255_lut[b_idx] << 0;

    return result;
}

inline auto pack4x8(vec4 color) -> u32 {
    u32 result =                        //
        ((u32)(color.a + 0.5f)) << 24 | //
        ((u32)(color.r + 0.5f)) << 16 | //
        ((u32)(color.g + 0.5f)) << 8 |  //
        ((u32)(color.b + 0.5f)) << 0;

    return result;
}

inline vec4 srgb255_to_linear1(vec4 color) {
    vec4 result = {};

    f32 inv_255 = 1.0f / 255.0f;

    result.r = srgb255_to_linear1_lookup(color.r);
    result.g = srgb255_to_linear1_lookup(color.g);
    result.b = srgb255_to_linear1_lookup(color.b);
    result.a = inv_255 * color.a;

    return result;
}

inline vec4 linear1_to_srgb255(vec4 color) {
    vec4 result = {};

    f32 one255 = 255.0f;

    result.r = linear1_to_srgb255_lookup(color.r);
    result.g = linear1_to_srgb255_lookup(color.g);
    result.b = linear1_to_srgb255_lookup(color.b);
    result.a = one255 * color.a;

    return (result);
}

static void draw_rectangle(OffscreenBuffer* buffer, Rectangle2i rect, f32 r, f32 g, f32 b) {
    u32 color = (round_f32_to_i32(r * 255.0f) << 16) | (round_f32_to_i32(g * 255.0f) << 8) | (round_f32_to_i32(b * 255.0f));

    for (int y = rect.min_y; y < rect.max_y; y++) {
        for (int x = rect.min_x; x < rect.max_x; x++) {
            u8* dest = ((u8*)buffer->memory + (y * buffer->pitch) + (x * buffer->bytes_per_pixel));
            u32* pixel = (u32*)dest;
            *pixel = color;
        }
    }
}

static auto clear(i32 client_width, i32 client_height, vec4 color, Rectangle2i* clip_rect) {
    draw_rectangle(                     //
        &state.global_offscreen_buffer, //
        *clip_rect,                     //
        color.r, color.g, color.b       //
    );
}

static inline auto get_texel(Win32Texture* texture, i32 x, i32 y) -> u32 {
    x = clamp(x, 0, texture->width - 1);
    y = clamp(y, 0, texture->width - 1);
    return *((u8*)texture->data + (y * texture->pitch) + (x * texture->bytes_per_pixel));
}

auto color_channel_f32_to_u8(f32 channel) {
    u8 result = 0;
    if (channel < 0) {
        result = 0;
    }
    else if (channel > 255.0f) {
        result = 255;
    }
    else {
        result = (u8)(channel + 0.5f);
    }
    return result;
}

static auto draw_bitmap(Quadrilateral quad, vec2 offset, vec2 scale, f32 rotation, vec4 color, i32 texture_id,
    ivec2 uv_min, ivec2 uv_max, i32 screen_width, i32 screen_height, Rectangle2i* clip_rect) {

    f32 model_width = (quad.br.x - quad.bl.x);
    f32 model_height = (quad.tr.y - quad.br.y);

    f32 scaled_width = (quad.br.x - quad.bl.x) * scale.x;
    f32 scaled_height = (quad.tr.y - quad.br.y) * scale.y;

    vec2 translation = offset;

    mat2 rot_mat = mat2_rotate(rotation);
    mat2 scale_mat = mat2_scale(scale);

    // model to camera
    mat2 M_m_to_c = rot_mat * scale_mat;
    mat2 M_c_to_m = inverse(M_m_to_c);

    vec2 bl_c = quad.bl * M_m_to_c;
    vec2 tl_c = quad.tl * M_m_to_c;
    vec2 tr_c = quad.tr * M_m_to_c;
    vec2 br_c = quad.br * M_m_to_c;

    bl_c = bl_c + translation;
    tl_c = tl_c + translation;
    tr_c = tr_c + translation;
    br_c = br_c + translation;

    i32 min_x = round_f32_to_i32(hm::min(bl_c.x, tl_c.x, tr_c.x, br_c.x));
    i32 max_x = round_f32_to_i32(hm::max(bl_c.x, tl_c.x, tr_c.x, br_c.x));
    i32 min_y = round_f32_to_i32(hm::min(bl_c.y, tl_c.y, tr_c.y, br_c.y));
    i32 max_y = round_f32_to_i32(hm::max(bl_c.y, tl_c.y, tr_c.y, br_c.y));

    OffscreenBuffer* buffer = &state.global_offscreen_buffer;
    min_x = hm::max(min_x, clip_rect->min_x);
    max_x = hm::min(max_x, clip_rect->max_x);
    min_y = hm::max(min_y, clip_rect->min_y);
    max_y = hm::min(max_y, clip_rect->max_y);

    vec2 edge1 = tl_c - bl_c;
    vec2 edge2 = tr_c - tl_c;
    vec2 edge3 = br_c - tr_c;
    vec2 edge4 = bl_c - br_c;

    Win32Texture* texture = &state.textures[texture_id];
    i32 u_min = uv_min.x;
    i32 u_max = uv_max.x;
    i32 v_min = uv_min.y;
    i32 v_max = uv_max.y;

    if (u_max == 0 || v_max == 0) {
        u_max = texture->width - 1;
        v_max = texture->height - 1;
    }

    f32 tex_range_u = (f32)(u_max - u_min);
    f32 tex_range_v = (f32)(v_max - v_min);

    f32 scaled_du = (scale.x * (f32)tex_range_u) / (scaled_width - 1.0f);
    f32 scaled_dv = (scale.y * (f32)tex_range_v) / (scaled_height - 1.0f);

    vec2 ds_dx = vec2(M_c_to_m.xx * scaled_du, M_c_to_m.xy * scaled_dv);
    vec2 ds_dy = vec2(M_c_to_m.yx * scaled_du, M_c_to_m.yy * scaled_dv);

    // model space, but scaled!
    f32 start_x_m = (f32)min_x - translation.x;
    f32 start_y_m = (f32)min_y - translation.y;

    // Start coordinate for the "current" texel row in texel space
    f32 texel_u_row = (start_x_m * M_c_to_m.xx + start_y_m * M_c_to_m.yx + model_width * 0.5f) * scaled_du + (f32)u_min;
    f32 texel_v_row = (start_x_m * M_c_to_m.xy + start_y_m * M_c_to_m.yy + model_height * 0.5f) * scaled_dv + (f32)v_min;

    vec4 default_color_l1 = srgb255_to_linear1(color);
    for (int y = min_y; y < max_y; y++) {
        f32 u = texel_u_row;
        f32 v = texel_v_row;
        for (int x = min_x; x < max_x; x++) {
            u8* dest = ((u8*)buffer->memory + (y * buffer->pitch) + (x * buffer->bytes_per_pixel));
            u32* pixel = (u32*)dest;

            vec2 camera_point{ (f32)x, (f32)y };

            bool is_inside = true;
            if (rotation != 0.0f) {
                f32 dot1 = dot(camera_point - bl_c, edge1);
                f32 dot2 = dot(camera_point - tl_c, edge2);
                f32 dot3 = dot(camera_point - tr_c, edge3);
                f32 dot4 = dot(camera_point - br_c, edge4);
                is_inside = dot1 > 0 && dot2 > 0 && dot3 > 0 && dot4 > 0;
            }

            if (is_inside) {

                vec4 src_color_l1;
                if (texture_id == 0) {
                    src_color_l1 = default_color_l1;
                }
                else {
                    i32 x0 = (i32)floor(u);
                    i32 y0 = (i32)floor(v);
                    i32 x1 = x0 + 1;
                    i32 y1 = y0 + 1;

                    x0 = clamp(x0, u_min, u_max);
                    y0 = clamp(y0, v_min, v_max);
                    x1 = clamp(x1, u_min, u_max);
                    y1 = clamp(y1, v_min, v_max);

                    f32 u_frac = clamp(u - (f32)x0, 0.0f, 1.0f);
                    f32 v_frac = clamp(v - (f32)y0, 0.0f, 1.0f);

                    Assert(texture->bytes_per_pixel == 4);
                    u32* data = (u32*)state.textures[texture_id].data;
                    // Bilinear filtering
                    vec4 texel00_l1 = unpack4x8_srgb255_to_linear1(*(data + (y0 * texture->width) + x0));
                    vec4 texel10_l1 =
                        unpack4x8_srgb255_to_linear1(*((u32*)state.textures[texture_id].data + (y0 * texture->width) + x1));
                    vec4 texel01_l1 =
                        unpack4x8_srgb255_to_linear1(*((u32*)state.textures[texture_id].data + (y1 * texture->width) + x0));
                    vec4 texel11_l1 =
                        unpack4x8_srgb255_to_linear1(*((u32*)state.textures[texture_id].data + (y1 * texture->width) + x1));

                    src_color_l1 = lerp(                       //
                        lerp(texel00_l1, u_frac, texel10_l1),  //
                        v_frac,                                //
                        lerp(texel01_l1, u_frac, texel11_l1)); //
                }

                vec4 dest_l1 = unpack4x8_srgb255_to_linear1(*pixel);

                // Cout = Cf * Af + Cb * (1 - Af)
                vec4 blended = src_color_l1;
                if (blended.a < 1.0f) {
                    blended = src_color_l1 * src_color_l1.a + (dest_l1 * (1.0f - src_color_l1.a));
                }
                // vec4 blended = dest_l1 * (1.0f - src_color_l1.a) + src_color_l1;

                *pixel = pack4x8_linear1_to_srgb255(blended);
            }
            u += ds_dx.x;
            v += ds_dx.y;
        }
        texel_u_row += ds_dy.x;
        texel_v_row += ds_dy.y;
    }
}

static auto draw_bitmap_avx2(Quadrilateral quad, vec2 offset, vec2 scale, f32 rotation, vec4 color, i32 texture_id,
    ivec2 uv_min, ivec2 uv_max, i32 screen_width, i32 screen_height, Rectangle2i* clip_rect) {

    f32 model_width = (quad.br.x - quad.bl.x);
    f32 model_height = (quad.tr.y - quad.br.y);

    f32 scaled_width = (quad.br.x - quad.bl.x) * scale.x;
    f32 scaled_height = (quad.tr.y - quad.br.y) * scale.y;

    vec2 translation = offset;

    mat2 rot_mat = mat2_rotate(rotation);
    mat2 scale_mat = mat2_scale(scale);

    // model to camera
    mat2 M_m_to_c = rot_mat * scale_mat;
    mat2 M_c_to_m = inverse(M_m_to_c);

    vec2 bl_c = quad.bl * M_m_to_c;
    vec2 tl_c = quad.tl * M_m_to_c;
    vec2 tr_c = quad.tr * M_m_to_c;
    vec2 br_c = quad.br * M_m_to_c;

    bl_c = bl_c + translation;
    tl_c = tl_c + translation;
    tr_c = tr_c + translation;
    br_c = br_c + translation;

    i32 min_x = round_f32_to_i32(hm::min(bl_c.x, tl_c.x, tr_c.x, br_c.x));
    i32 max_x = round_f32_to_i32(hm::max(bl_c.x, tl_c.x, tr_c.x, br_c.x));
    i32 min_y = round_f32_to_i32(hm::min(bl_c.y, tl_c.y, tr_c.y, br_c.y));
    i32 max_y = round_f32_to_i32(hm::max(bl_c.y, tl_c.y, tr_c.y, br_c.y));

    OffscreenBuffer* buffer = &state.global_offscreen_buffer;
    min_x = hm::max(min_x, clip_rect->min_x);
    max_x = hm::min(max_x, clip_rect->max_x);
    min_y = hm::max(min_y, clip_rect->min_y);
    max_y = hm::min(max_y, clip_rect->max_y);

    vec2 edge1 = tl_c - bl_c;
    vec2 edge2 = tr_c - tl_c;
    vec2 edge3 = br_c - tr_c;
    vec2 edge4 = bl_c - br_c;

    Win32Texture* texture = &state.textures[texture_id];
    i32 u_min = uv_min.x;
    i32 u_max = uv_max.x;
    i32 v_min = uv_min.y;
    i32 v_max = uv_max.y;

    if (u_max == 0 || v_max == 0) {
        u_max = texture->width - 1;
        v_max = texture->height - 1;
    }

    f32 tex_range_u = (f32)(u_max - u_min);
    f32 tex_range_v = (f32)(v_max - v_min);

    f32 scaled_du = (scale.x * (f32)tex_range_u) / (scaled_width - 1.0f);
    f32 scaled_dv = (scale.y * (f32)tex_range_v) / (scaled_height - 1.0f);

    vec2 ds_dx = vec2(M_c_to_m.xx * scaled_du, M_c_to_m.xy * scaled_dv);
    vec2 ds_dy = vec2(M_c_to_m.yx * scaled_du, M_c_to_m.yy * scaled_dv);

    // model space, but scaled!
    f32 start_x_m = (f32)min_x - translation.x;
    f32 start_y_m = (f32)min_y - translation.y;

    // Start coordinate for the "current" texel row in texel space
    f32 texel_u_row = (start_x_m * M_c_to_m.xx + start_y_m * M_c_to_m.yx + model_width * 0.5f) * scaled_du + (f32)u_min;
    f32 texel_v_row = (start_x_m * M_c_to_m.xy + start_y_m * M_c_to_m.yy + model_height * 0.5f) * scaled_dv + (f32)v_min;

    vec4 default_color_l1 = srgb255_to_linear1(color);

    for (int y = min_y; y < max_y; y++) {
        /*f32 u = texel_u_row;*/
        /*f32 v = texel_v_row;*/

        __m256 u_v8 = _mm256_set1_ps(texel_u_row);
        __m256 v_v8 = _mm256_set1_ps(texel_v_row);
        {
            __m256 lane = _mm256_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f);
            __m256 du_dx_v8 = _mm256_set1_ps(ds_dx.x);
            __m256 dv_dx_v8 = _mm256_set1_ps(ds_dx.y);
            u_v8 = _mm256_fmadd_ps(lane, du_dx_v8, u_v8);
            v_v8 = _mm256_fmadd_ps(lane, dv_dx_v8, v_v8);
        }
        for (int x = min_x; x < max_x; x++) {

            u8* dest = ((u8*)buffer->memory + (y * buffer->pitch) + (x * buffer->bytes_per_pixel));
            u32* pixel = (u32*)dest;

            vec2 camera_point{ (f32)x, (f32)y };

            __m256 cp_x_v8 = _mm256_set1_ps((f32)x);
            __m256 incr_v8 = _mm256_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f);
            cp_x_v8 = _mm256_add_ps(cp_x_v8, incr_v8);
            __m256 cp_y_v8 = _mm256_set1_ps((f32)y);

            bool is_inside = true;
            if (rotation != 0.0f) {
                __m256 dot1_v8;
                // f32 dot1 = dot(camera_point - bl_c, edge1);
                {
                    __m256 x_v8 = _mm256_set1_ps(bl_c.x);
                    __m256 y_v8 = _mm256_set1_ps(bl_c.y);

                    x_v8 = _mm256_sub_ps(cp_x_v8, x_v8);
                    y_v8 = _mm256_sub_ps(cp_y_v8, y_v8);

                    __m256 edge1_x_v8 = _mm256_set1_ps(edge1.x);
                    __m256 edge1_y_v8 = _mm256_set1_ps(edge1.y);

                    dot1_v8 = dot(x_v8, y_v8, edge1_x_v8, edge1_y_v8);
                }
                // f32 dot2 = dot(camera_point - tl_c, edge2);
                __m256 dot2_v8;
                {
                    __m256 x_v8 = _mm256_set1_ps(tl_c.x);
                    __m256 y_v8 = _mm256_set1_ps(tl_c.y);

                    x_v8 = _mm256_sub_ps(cp_x_v8, x_v8);
                    y_v8 = _mm256_sub_ps(cp_y_v8, y_v8);

                    __m256 edge2_x_v8 = _mm256_set1_ps(edge2.x);
                    __m256 edge2_y_v8 = _mm256_set1_ps(edge2.y);

                    dot2_v8 = dot(x_v8, y_v8, edge2_x_v8, edge2_y_v8);
                }
                // f32 dot3 = dot(camera_point - tr_c, edge3);
                __m256 dot3_v8;
                {
                    __m256 x_v8 = _mm256_set1_ps(tr_c.x);
                    __m256 y_v8 = _mm256_set1_ps(tr_c.y);

                    x_v8 = _mm256_sub_ps(cp_x_v8, x_v8);
                    y_v8 = _mm256_sub_ps(cp_y_v8, y_v8);

                    __m256 edge3_x_v8 = _mm256_set1_ps(edge3.x);
                    __m256 edge3_y_v8 = _mm256_set1_ps(edge3.y);

                    dot3_v8 = dot(x_v8, y_v8, edge3_x_v8, edge3_y_v8);
                }
                // f32 dot4 = dot(camera_point - br_c, edge4);
                __m256 dot4_v8;
                {
                    __m256 x_v8 = _mm256_set1_ps(br_c.x);
                    __m256 y_v8 = _mm256_set1_ps(br_c.y);

                    x_v8 = _mm256_sub_ps(cp_x_v8, x_v8);
                    y_v8 = _mm256_sub_ps(cp_y_v8, y_v8);

                    __m256 edge4_x_v8 = _mm256_set1_ps(edge4.x);
                    __m256 edge4_y_v8 = _mm256_set1_ps(edge4.y);

                    dot4_v8 = dot(x_v8, y_v8, edge4_x_v8, edge4_y_v8);
                }
                // is_inside = dot1 > 0 && dot2 > 0 && dot3 > 0 && dot4 > 0;
                const __m256 zero_v8 = _mm256_setzero_ps();
                __m256 is_inside_v8;
                {
                    __m256 is_inside_dot1_v8 = _mm256_cmp_ps(dot1_v8, zero_v8, _CMP_GT_OQ);
                    __m256 is_inside_dot2_v8 = _mm256_cmp_ps(dot2_v8, zero_v8, _CMP_GT_OQ);
                    __m256 is_inside_dot3_v8 = _mm256_cmp_ps(dot3_v8, zero_v8, _CMP_GT_OQ);
                    __m256 is_inside_dot4_v8 = _mm256_cmp_ps(dot4_v8, zero_v8, _CMP_GT_OQ);

                    is_inside_v8 = _mm256_and_ps(is_inside_dot1_v8, is_inside_dot2_v8);
                    is_inside_v8 = _mm256_and_ps(is_inside_v8, is_inside_dot3_v8);
                    is_inside_v8 = _mm256_and_ps(is_inside_v8, is_inside_dot4_v8);

                    f32 is_inside_arr[8];
                    _mm256_storeu_ps(is_inside_arr, is_inside_v8);
                    is_inside = is_inside_arr[0] > 0.0f;
                }
            }

            if (is_inside) {

                vec4 src_color_l1;
                if (texture_id == 0) {
                    src_color_l1 = default_color_l1;
                }
                else {
                    f32 u, v;
                    {
                        f32 u_arr8[8];
                        f32 v_arr8[8];

                        _mm256_store_ps(u_arr8, u_v8);
                        _mm256_store_ps(v_arr8, v_v8);

                        u = u_arr8[0];
                        v = v_arr8[0];
                    }

                    __m256i one_v8 = _mm256_set1_epi32(1);
                    __m256i x0_v8 = _mm256_cvttps_epi32(u_v8);
                    __m256i y0_v8 = _mm256_cvttps_epi32(v_v8);
                    __m256i x1_v8 = _mm256_add_epi32(x0_v8, one_v8);
                    __m256i y1_v8 = _mm256_add_epi32(y0_v8, one_v8);

                    /*i32 x0 = (i32)floor(u);*/
                    /*i32 y0 = (i32)floor(v);*/
                    /*i32 x1 = x0 + 1;*/
                    /*i32 y1 = y0 + 1;*/
                    i32 x0, y0, x1, y1;
                    {
                        x0_v8 = clamp_i32_v8(u_min, x0_v8, u_max);
                        x1_v8 = clamp_i32_v8(u_min, x1_v8, u_max);
                        y0_v8 = clamp_i32_v8(v_min, y0_v8, v_max);
                        y1_v8 = clamp_i32_v8(v_min, y1_v8, v_max);

                        i32 x0_arr8[8];
                        _mm256_storeu_si256((__m256i*)x0_arr8, x0_v8);
                        x0 = x0_arr8[0];

                        i32 x1_arr8[8];
                        _mm256_storeu_si256((__m256i*)x1_arr8, x1_v8);
                        x1 = x1_arr8[0];

                        i32 y0_arr8[8];
                        _mm256_storeu_si256((__m256i*)y0_arr8, y0_v8);
                        y0 = y0_arr8[0];

                        i32 y1_arr8[8];
                        _mm256_storeu_si256((__m256i*)y1_arr8, y1_v8);
                        y1 = y1_arr8[0];
                    }

                    f32 u_frac = clamp(u - (f32)x0, 0.0f, 1.0f);
                    f32 v_frac = clamp(v - (f32)y0, 0.0f, 1.0f);
                    {
                        __m256 x0_f32_v8 = _mm256_cvtepi32_ps(x0_v8);
                        __m256 y0_f32_v8 = _mm256_cvtepi32_ps(y0_v8);
                        __m256 u_frac_v8 = clamp_f32_v8(0.0f, _mm256_sub_ps(u_v8, x0_f32_v8), 1.0f);
                        __m256 v_frac_v8 = clamp_f32_v8(0.0f, _mm256_sub_ps(v_v8, y0_f32_v8), 1.0f);

                        f32 u_frac_arr[8];
                        _mm256_store_ps(u_frac_arr, u_frac_v8);
                        u_frac = u_frac_arr[0];
                        f32 v_frac_arr[8];
                        _mm256_store_ps(v_frac_arr, v_frac_v8);
                        v_frac = v_frac_arr[0];
                    }

                    Assert(texture->bytes_per_pixel == 4);
                    u32* data = (u32*)state.textures[texture_id].data;
                    // Bilinear filtering

                    /*vec4 texel00_l1 = unpack4x8_srgb255_to_linear1(*(data + (y0 * texture->width) + x0));*/
                    /*vec4 texel10_l1 =*/
                    /*    unpack4x8_srgb255_to_linear1(*((u32*)state.textures[texture_id].data + (y0 * texture->width) + x1));*/
                    /*vec4 texel01_l1 =*/
                    /*    unpack4x8_srgb255_to_linear1(*((u32*)state.textures[texture_id].data + (y1 * texture->width) + x0));*/
                    /*vec4 texel11_l1 =*/
                    /*    unpack4x8_srgb255_to_linear1(*((u32*)state.textures[texture_id].data + (y1 * texture->width) + x1));*/

                    __m256i width_v8 = _mm256_set1_epi32(texture->width);

                    __m256i texel_idx_00_v8 = _mm256_add_epi32(x0_v8, _mm256_mullo_epi32(y0_v8, width_v8));
                    __m256i texel_idx_10_v8 = _mm256_add_epi32(x1_v8, _mm256_mullo_epi32(y0_v8, width_v8));
                    __m256i texel_idx_01_v8 = _mm256_add_epi32(x0_v8, _mm256_mullo_epi32(y1_v8, width_v8));
                    __m256i texel_idx_11_v8 = _mm256_add_epi32(x1_v8, _mm256_mullo_epi32(y1_v8, width_v8));

                    __m256i texel00_li_v8 = _mm256_i32gather_epi32((const i32*)data, texel_idx_00_v8, sizeof(u32));
                    __m256i texel01_li_v8 = _mm256_i32gather_epi32((const i32*)data, texel_idx_01_v8, sizeof(u32));
                    __m256i texel10_li_v8 = _mm256_i32gather_epi32((const i32*)data, texel_idx_10_v8, sizeof(u32));
                    __m256i texel11_li_v8 = _mm256_i32gather_epi32((const i32*)data, texel_idx_11_v8, sizeof(u32));

                    u32 texel00_rgb255_arr[8];
                    u32 texel01_rgb255_arr[8];
                    u32 texel10_rgb255_arr[8];
                    u32 texel11_rgb255_arr[8];

                    _mm256_storeu_si256((__m256i*)texel00_rgb255_arr, texel00_li_v8);
                    _mm256_storeu_si256((__m256i*)texel10_rgb255_arr, texel10_li_v8);
                    _mm256_storeu_si256((__m256i*)texel01_rgb255_arr, texel01_li_v8);
                    _mm256_storeu_si256((__m256i*)texel11_rgb255_arr, texel11_li_v8);

                    vec4 texel00_l1 = unpack4x8_srgb255_to_linear1(texel00_rgb255_arr[0]);
                    vec4 texel10_l1 = unpack4x8_srgb255_to_linear1(texel10_rgb255_arr[0]);
                    vec4 texel01_l1 = unpack4x8_srgb255_to_linear1(texel01_rgb255_arr[0]);
                    vec4 texel11_l1 = unpack4x8_srgb255_to_linear1(texel11_rgb255_arr[0]);

                    __m256 texel00_r_v8;
                    __m256 texel00_g_v8;
                    __m256 texel00_b_v8;
                    __m256 texel00_a_v8;

                    src_color_l1 = lerp(                       //
                        lerp(texel00_l1, u_frac, texel10_l1),  //
                        v_frac,                                //
                        lerp(texel01_l1, u_frac, texel11_l1)); //
                }

                vec4 dest_l1 = unpack4x8_srgb255_to_linear1(*pixel);

                // Cout = Cf * Af + Cb * (1 - Af)
                vec4 blended = src_color_l1;
                if (blended.a < 1.0f) {
                    blended = src_color_l1 * src_color_l1.a + (dest_l1 * (1.0f - src_color_l1.a));
                }
                // vec4 blended = dest_l1 * (1.0f - src_color_l1.a) + src_color_l1;

                *pixel = pack4x8_linear1_to_srgb255(blended);
            }
            /*u += ds_dx.x;*/
            /*v += ds_dx.y;*/
            {
                __m256 du_dx_v8 = _mm256_set1_ps(ds_dx.x);
                __m256 dv_dx_v8 = _mm256_set1_ps(ds_dx.y);
                u_v8 = _mm256_add_ps(u_v8, du_dx_v8);
                v_v8 = _mm256_add_ps(v_v8, dv_dx_v8);
            }
        }
        texel_u_row += ds_dy.x;
        texel_v_row += ds_dy.y;
    }
}

extern "C" __declspec(dllexport) RENDERER_INIT(win32_renderer_init) {
    log_info("Using software renderer.");

    // TODO: We should check for need of resizing on every draw call.

    // resize_dib_section(&state.global_offscreen_buffer, 48, 58);
    resize_dib_section(&state.global_offscreen_buffer, SCREEN_WIDTH, SCREEN_HEIGHT);

    HM_ASSERT(memory != nullptr);
    HM_ASSERT(memory->data != nullptr);
    state.permanent.init(memory->data, memory->size);
    state.transient = *state.permanent.allocate_arena(MegaBytes(10));

    Platform = platform_api;
    global_debug_table = Platform->debug_table;

    Win32Texture* null_texture = &state.textures[0];
    null_texture->height = 1;
    null_texture->width = 1;
    null_texture->count = 1;
    null_texture->size = sizeof(u32);
    null_texture->bytes_per_pixel = sizeof(u32);
    null_texture->data = allocate<u32>(state.permanent);
    u32* data = (u32*)null_texture->data;
    *data = pack4x8(vec4(1.0f, 0.0f, 0.0f, 1.0f));

    generate_luts();
}

extern "C" __declspec(dllexport) RENDERER_DELETE_CONTEXT(win32_renderer_delete_context) {
}

extern "C" __declspec(dllexport) RENDERER_ADD_TEXTURE(win32_renderer_add_texture) {
    HM_ASSERT(texture_id != 0);
    if (texture_id >= MaxTextureId) {
        // TODO: Return empty texture
        log_error("Max texture id is %d, %d provided.", MaxTextureId, texture_id);
        HM_ASSERT(false);
        return false;
    }

    Win32Texture* texture = &state.textures[texture_id];
    if (texture->data == nullptr) {
        texture->height = height;
        texture->width = width;
        texture->bytes_per_pixel = bytes_per_pixel;
        texture->size = width * height * texture->bytes_per_pixel;
        texture->count = width * height;
        texture->pitch = width * texture->bytes_per_pixel;
        texture->data = state.permanent.allocate(texture->size);

        Assert(texture->bytes_per_pixel == 4);
        // Windows want ARGB
        u32* source = (u32*)data;
        u32* dest = (u32*)texture->data;
        for (auto i = 0; i < texture->count; i++) {
            u8 red = (*source >> 0) & 0xFF;
            u8 green = (*source >> 8) & 0xFF;
            u8 blue = (*source >> 16) & 0xFF;
            u8 alpha = (*source >> 24) & 0xFF;
            *dest++ = alpha << 24 | red << 16 | green << 8 | blue;
            source++;
        }
        log_info("Texture added");
        return true;
    }
    else {
        return false;
    }
}

global_variable vec4 global_color_palette[32] = {
    vec4(230.0f, 25.0f, 75.0f, 255.0f),   // vivid red
    vec4(60.0f, 180.0f, 75.0f, 255.0f),   // green
    vec4(255.0f, 225.0f, 25.0f, 255.0f),  // yellow
    vec4(0.0f, 130.0f, 200.0f, 255.0f),   // blue
    vec4(245.0f, 130.0f, 48.0f, 255.0f),  // orange
    vec4(145.0f, 30.0f, 180.0f, 255.0f),  // purple
    vec4(70.0f, 240.0f, 240.0f, 255.0f),  // cyan
    vec4(240.0f, 50.0f, 230.0f, 255.0f),  // magenta
    vec4(210.0f, 245.0f, 60.0f, 255.0f),  // lime
    vec4(250.0f, 190.0f, 212.0f, 255.0f), // pink
    vec4(0.0f, 128.0f, 128.0f, 255.0f),   // teal
    vec4(220.0f, 190.0f, 255.0f, 255.0f), // lavender
    vec4(170.0f, 110.0f, 40.0f, 255.0f),  // brown
    vec4(255.0f, 250.0f, 200.0f, 255.0f), // beige
    vec4(128.0f, 0.0f, 0.0f, 255.0f),     // maroon
    vec4(170.0f, 255.0f, 195.0f, 255.0f), // mint

    vec4(128.0f, 128.0f, 0.0f, 255.0f),   // olive
    vec4(255.0f, 215.0f, 180.0f, 255.0f), // apricot
    vec4(0.0f, 0.0f, 128.0f, 255.0f),     // navy
    vec4(255.0f, 225.0f, 180.0f, 255.0f), // peach
    vec4(0.0f, 255.0f, 0.0f, 255.0f),     // bright green
    vec4(255.0f, 160.0f, 122.0f, 255.0f), // salmon
    vec4(0.0f, 255.0f, 255.0f, 255.0f),   // aqua
    vec4(186.0f, 85.0f, 211.0f, 255.0f),  // medium purple
    vec4(255.0f, 99.0f, 71.0f, 255.0f),   // tomato
    vec4(154.0f, 205.0f, 50.0f, 255.0f),  // yellow green
    vec4(72.0f, 61.0f, 139.0f, 255.0f),   // slate blue
    vec4(255.0f, 140.0f, 0.0f, 255.0f),   // dark orange
    vec4(64.0f, 224.0f, 208.0f, 255.0f),  // turquoise
    vec4(199.0f, 21.0f, 133.0f, 255.0f),  // medium violet red
    vec4(135.0f, 206.0f, 235.0f, 255.0f), // sky blue
    vec4(255.0f, 255.0f, 255.0f, 255.0f)  // white
};

auto execute_render_commands(i32 job_id, RenderCommands* commands, i32* command_render_order, Rectangle2i clip_rect) -> void {
    for (i32 i = 0; i < commands->sort_keys.count(); i++) {
        u64 base_address = commands->sort_entries_offset[command_render_order[i]];
        RenderGroupEntryHeader* header = (RenderGroupEntryHeader*)(commands->push_buffer + base_address);
        base_address += sizeof(RenderGroupEntryHeader);

        void* data = (u8*)header + sizeof(*header);
        switch (header->type) {
        case RenderCommands_RenderEntryClear: {
            RenderEntryClear* entry = (RenderEntryClear*)data;
            clear(commands->screen_width, commands->screen_height, entry->color, &clip_rect);
            base_address += sizeof(*entry);
        } break;
        case RenderCommands_RenderEntryBitmap: {
            auto* entry = (RenderEntryBitmap*)data;
            draw_bitmap_avx2(entry->quad, entry->offset, entry->scale, entry->rotation, entry->color, entry->texture_id,
                entry->uv_min, entry->uv_max, commands->screen_width, commands->screen_height, &clip_rect);

            base_address += sizeof(*entry);
        } break;
        default: InvalidCodePath;
        }
    }
}

struct RenderTileJob {
    i32 id;
    RenderCommands* commands;
    i32* command_render_order;
    Rectangle2i clip_rect;
};

static PLATFORM_WORK_QUEUE_CALLBACK(execute_render_tile_job) {
    RenderTileJob* job = (RenderTileJob*)data;

    Assert(job);
    Assert(job->commands);
    BEGIN_BLOCK("Execute render commands");
    execute_render_commands(job->id, job->commands, job->command_render_order, job->clip_rect);
    END_BLOCK();
    MemoryBarrier();
}

extern "C" __declspec(dllexport) RENDERER_RENDER(win32_renderer_render) {

    i32 const tile_count_x = 4;
    i32 const tile_count_y = 4;

    i32 tile_width = commands->screen_width / tile_count_x;
    i32 tile_height = commands->screen_height / tile_count_y;

    const i32 sse_width = 4;
    tile_width = ((tile_width + (sse_width - 1)) / sse_width) * sse_width;

    RenderTileJob render_tile_jobs[tile_count_x * tile_count_y];

    i32* command_render_order = merge_sort_indices(commands->sort_keys.data(), commands->sort_keys.count(), &state.transient);

    i32 job_count = 0;
    for (i32 y = 0; y < tile_count_y; y++) {
        for (i32 x = 0; x < tile_count_x; x++) {
            RenderTileJob* job = render_tile_jobs + job_count++;

            Rectangle2i rect = {};
            rect.min_x = x * tile_width;
            rect.max_x = rect.min_x + tile_width;
            if (x == tile_count_x - 1) {
                rect.max_x = commands->screen_width;
            }

            rect.min_y = y * tile_height;
            rect.max_y = rect.min_y + tile_height;
            if (y == tile_count_y - 1) {
                rect.max_y = commands->screen_height;
            }

            job->id = job_count;
            job->clip_rect = rect;
            job->commands = commands;
            job->command_render_order = command_render_order;

            Platform->add_work_queue_entry(render_queue, execute_render_tile_job, job);
        }
    }

    Platform->complete_all_work(render_queue);
}

extern "C" __declspec(dllexport) RENDERER_BEGIN_FRAME(win32_renderer_begin_frame) {
    state.transient.clear_to_zero();
}

extern "C" __declspec(dllexport) RENDERER_END_FRAME(win32_renderer_end_frame) {
    Win32RenderContext* win32_context = (Win32RenderContext*)context;
    HDC device_context = GetDC(win32_context->window);

    i32 width = state.global_offscreen_buffer.width;
    i32 height = state.global_offscreen_buffer.height;

    u32 result = StretchDIBits(               //
        device_context,                       //
        0, 0,                                 //
        width, height, 0, 0,                  //
        width, height,                        //
        state.global_offscreen_buffer.memory, //
        &state.global_offscreen_buffer.Info,  //
        DIB_RGB_COLORS, SRCCOPY               //
    );
    if (result == 0 || result == GDI_ERROR) {
        log_error("StretchDIBits failed\n");
    }

    ReleaseDC(win32_context->window, device_context);
}

// TODO: I need this to handle redraw calls from windows, e.g. if you move the window, or move a window above it
// case WM_PAINT: {
//     PAINTSTRUCT paint;
//     HDC deviceContext = BeginPaint(window, &paint);
//     int x = paint.rcPaint.left;
//     int y = paint.rcPaint.top;
//     int width = paint.rcPaint.right - x;
//     int height = paint.rcPaint.bottom - y;
//     local_persist DWORD color = WHITENESS;
//     PatBlt(deviceContext, x, y, width, height, color);
//
//     Win32_WindowDimension dimension = win32_get_window_dimension(window);
//     PatBlt(deviceContext, 0, 0, dimension.width, dimension.height, BLACKNESS);
//     win32_DisplayBufferInWindows(deviceContext, //
//         dimension.width, dimension.height,      //
//         global_offscreen_buffer,                //
//         x, y,                                   //
//         width, height);
//
//     if (color == WHITENESS) {
//         color = BLACKNESS;
//     }
//     else {
//         color = WHITENESS;
//     }
//
//     EndPaint(window, &paint);
// } break;

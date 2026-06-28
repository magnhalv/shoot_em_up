#include <immintrin.h>
#include <platform/platform.hpp>

#include <math/mat2.hpp>
#include <math/mat3.hpp>
#include <math/math.hpp>
#include <math/simd.hpp>
#include <math/util.hpp>

#include <core/logger.hpp>
#include <core/memory.hpp>
#include <core/memory_arena.hpp>
#include <core/sort.hpp>
#include <engine/structs/swap_back_list.hpp>
// TODO: Move to core
#include <engine/hm_assert.hpp>
#include <engine/profiling.hpp>

#include <renderers/cpu_render_algorithms.hpp>
#include <renderers/renderer.hpp>
#include <renderers/win32_renderer.hpp>

#include "../core/lib.cpp"
#include "../math/unit.cpp"
#include "core/lib.hpp"
#include "platform/types.hpp"

struct WindowDimension {
    i32 width;
    i32 height;
};

struct Win32RenderInfo {
    BITMAPINFO Info;
    Framebuffer buffer;
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
    Win32RenderInfo platform_render_info;
    StackSwapBackList<Framebuffer, 5> framebuffers;
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

internal void resize_frame_buffer(Framebuffer* buffer, int width, int height) {
    // TODO: Bulletproof this
    // Maybe don't free first, free after, then free first if that fails. I.e. if VirtualAlloc fails.
    if (buffer->memory) {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }
    if (buffer->z_buffer.m_data) {
        VirtualFree(buffer->z_buffer.m_data, 0, MEM_RELEASE);
    }

    u64 entry_count = width * height;
    buffer->width = width;
    buffer->height = height;
    buffer->bytes_per_pixel = BYTES_PER_PIXEL;
    buffer->memory_size = buffer->bytes_per_pixel * (buffer->width * buffer->height);
    buffer->memory = VirtualAlloc(0, buffer->memory_size, MEM_COMMIT, PAGE_READWRITE);
    buffer->z_buffer.init(                                                            //
        (f32*)VirtualAlloc(0, entry_count * sizeof(f32), MEM_COMMIT, PAGE_READWRITE), //
        entry_count                                                                   //
    );
    buffer->pitch = buffer->width * buffer->bytes_per_pixel;
    // Should always be 64KB aligned from virtual alloc
    Assert(is_aligned(buffer->memory, KiloBytes(64)));
}
static void resize_dib_section(Win32RenderInfo* info, int width, int height) {
    resize_frame_buffer(&info->buffer, width, height);
    info->Info.bmiHeader.biSize = sizeof(info->Info.bmiHeader);
    info->Info.bmiHeader.biWidth = info->buffer.width;
    info->Info.bmiHeader.biHeight = info->buffer.height;
    info->Info.bmiHeader.biPlanes = 1;
    info->Info.bmiHeader.biBitCount = BYTES_PER_PIXEL * 8;
    info->Info.bmiHeader.biCompression = BI_RGB;
}

auto square(f32 v) -> f32 {
    return v * v;
}

auto square_root(f32 v) -> f32 {
    return sqrtf(v);
}

inline auto pack4x8(vec4 color) -> u32 {
    u32 result =                        //
        ((u32)(color.a + 0.5f)) << 24 | //
        ((u32)(color.r + 0.5f)) << 16 | //
        ((u32)(color.g + 0.5f)) << 8 |  //
        ((u32)(color.b + 0.5f)) << 0;

    return result;
}

inline auto srgb_to_linear1_2(vec4 color) -> color_v8 {
    color_v8 result = {};

    f32 r = srgb_to_linear1_lookup(color.r);
    f32 g = srgb_to_linear1_lookup(color.g);
    f32 b = srgb_to_linear1_lookup(color.b);
    f32 a = color.a;

    result.r = _mm256_set1_ps(r);
    result.g = _mm256_set1_ps(g);
    result.b = _mm256_set1_ps(b);
    result.a = _mm256_set1_ps(a);

    return result;
}

static void draw_rectangle(Framebuffer* buffer, Rectangle2i rect, f32 r, f32 g, f32 b) {
    u32 color = (round_f32_to_i32(r * 255.0f) << 16) | (round_f32_to_i32(g * 255.0f) << 8) | (round_f32_to_i32(b * 255.0f));

    u32 min_x = hm::max(rect.min_x, 0);
    u32 max_x = hm::min(rect.max_x, buffer->width);
    u32 min_y = hm::max(rect.min_y, 0);
    u32 max_y = hm::min(rect.max_y, buffer->height);

    for (u32 y = min_y; y < max_y; y++) {
        for (u32 x = min_x; x < max_x; x++) {
            u8* dest = ((u8*)buffer->memory + (y * buffer->pitch) + (x * buffer->bytes_per_pixel));
            u32* pixel = (u32*)dest;
            *pixel = color;
        }
    }
}

static void clear_check_pattern(Framebuffer& buffer, Rectangle2i rect, vec4 color1, vec4 color2) {
    u32 packed_color1 = pack_color_8x4(color1);
    u32 packed_color2 = pack_color_8x4(color2);

    u32 min_x = hm::max(rect.min_x, 0);
    u32 max_x = hm::min(rect.max_x, buffer.width);
    u32 min_y = hm::max(rect.min_y, 0);
    u32 max_y = hm::min(rect.max_y, buffer.height);

    for (u32 y = min_y; y < max_y; y++) {
        u32* pixel_dest = ((u32*)buffer.memory + (y * buffer.width)) + (min_x);
        for (u32 x = min_x; x < max_x; x++) {
            u32 color = (y + x) % 2 == 0 ? packed_color1 : packed_color2;
            *pixel_dest++ = color;
            buffer.z_buffer[y * buffer.width + x] = 0.0f;
        }
    }
}

static auto clear(i32 client_width, i32 client_height, vec4 color, Rectangle2i clip_rect, Framebuffer& buffer) {
    draw_rectangle(               //
        &buffer,                  //
        clip_rect,                //
        color.r, color.g, color.b //
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
    ivec2 uv_min, ivec2 uv_max, i32 screen_width, i32 screen_height, Rectangle2i* clip_rect, f32 border_thickness,
    vec4 border_color, Framebuffer* buffer) {
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

    vec2 bl_border_c, tl_border_c, tr_border_c, br_border_c;
    {
        bl_border_c = quad.bl * scale_mat;
        tl_border_c = quad.tl * scale_mat;
        tr_border_c = quad.tr * scale_mat;
        br_border_c = quad.br * scale_mat;

        bl_border_c = bl_border_c + vec2(border_thickness, border_thickness);
        tl_border_c = tl_border_c + vec2(border_thickness, -border_thickness);
        tr_border_c = tr_border_c + vec2(-border_thickness, -border_thickness);
        br_border_c = br_border_c + vec2(-border_thickness, border_thickness);

        bl_border_c = bl_border_c * rot_mat;
        tl_border_c = tl_border_c * rot_mat;
        tr_border_c = tr_border_c * rot_mat;
        br_border_c = br_border_c * rot_mat;

        bl_border_c = bl_border_c + translation;
        tl_border_c = tl_border_c + translation;
        tr_border_c = tr_border_c + translation;
        br_border_c = br_border_c + translation;
    }

    i32 min_x = round_f32_to_i32(hm::min(bl_c.x, tl_c.x, tr_c.x, br_c.x));
    i32 max_x = round_f32_to_i32(hm::max(bl_c.x, tl_c.x, tr_c.x, br_c.x));
    i32 min_y = round_f32_to_i32(hm::min(bl_c.y, tl_c.y, tr_c.y, br_c.y));
    i32 max_y = round_f32_to_i32(hm::max(bl_c.y, tl_c.y, tr_c.y, br_c.y));

    min_x = hm::max(min_x, clip_rect->min_x);
    max_x = hm::min(max_x, clip_rect->max_x);
    min_y = hm::max(min_y, clip_rect->min_y);
    max_y = hm::min(max_y, clip_rect->max_y);

    // min_x = hm::max(min_x, 0);
    // max_x = hm::min(max_x, state.frame_buffer.width);
    // min_y = hm::max(min_y, 0);
    // max_y = hm::min(max_y, state.frame_buffer.height);

    vec2 edge1 = tl_c - bl_c;
    vec2 edge2 = tr_c - tl_c;
    vec2 edge3 = br_c - tr_c;
    vec2 edge4 = bl_c - br_c;

    vec2 edge1_border = tl_border_c - bl_border_c;
    vec2 edge2_border = tr_border_c - tl_border_c;
    vec2 edge3_border = br_border_c - tr_border_c;
    vec2 edge4_border = bl_border_c - br_border_c;

    Win32Texture* texture = &state.textures[texture_id];
    i32 u_min = uv_min.x;
    i32 u_max = uv_max.x;
    i32 v_min = uv_min.y;
    i32 v_max = uv_max.y;

    if (u_max == 0 || v_max == 0) {
        u_max = texture->width;
        v_max = texture->height;
    }

    f32 tex_range_u = (f32)(u_max - u_min);
    f32 tex_range_v = (f32)(v_max - v_min);

    f32 scaled_du = (scale.x * (f32)tex_range_u) / (scaled_width);
    f32 scaled_dv = (scale.y * (f32)tex_range_v) / (scaled_height);

    vec2 ds_dx = vec2(M_c_to_m.xx * scaled_du, M_c_to_m.xy * scaled_dv);
    vec2 ds_dy = vec2(M_c_to_m.yx * scaled_du, M_c_to_m.yy * scaled_dv);

    // model space, but scaled!
    f32 start_x_m = (f32)min_x - translation.x;
    f32 start_y_m = (f32)min_y - translation.y;

    // Start coordinate for the "current" texel row in texel space
    f32 texel_u_row = (start_x_m * M_c_to_m.xx + start_y_m * M_c_to_m.yx + model_width * 0.5f) * scaled_du + (f32)u_min;
    f32 texel_v_row = (start_x_m * M_c_to_m.xy + start_y_m * M_c_to_m.yy + model_height * 0.5f) * scaled_dv + (f32)v_min;

    vec4 default_color_l1 = srgb_to_linear1(color);
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

            bool is_on_border = false;
            if (is_inside && border_thickness > 0.0f) {
                f32 dot1 = dot(camera_point - bl_border_c, edge1_border);
                f32 dot2 = dot(camera_point - tl_border_c, edge2_border);
                f32 dot3 = dot(camera_point - tr_border_c, edge3_border);
                f32 dot4 = dot(camera_point - br_border_c, edge4_border);
                is_on_border = !(dot1 > 0 && dot2 > 0 && dot3 > 0 && dot4 > 0);
            }

            if (is_inside) {
                vec4 src_color_l1;
                if (is_on_border) {
                    src_color_l1 = srgb_to_linear1(vec4(255.0f, 0.0f, 0.0f, 255.0f));
                }
                else {
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
                        vec4 texel10_l1 = unpack4x8_srgb255_to_linear1(
                            *((u32*)state.textures[texture_id].data + (y0 * texture->width) + x1));
                        vec4 texel01_l1 = unpack4x8_srgb255_to_linear1(
                            *((u32*)state.textures[texture_id].data + (y1 * texture->width) + x0));
                        vec4 texel11_l1 = unpack4x8_srgb255_to_linear1(
                            *((u32*)state.textures[texture_id].data + (y1 * texture->width) + x1));

                        src_color_l1 = lerp(                       //
                            lerp(texel00_l1, u_frac, texel10_l1),  //
                            v_frac,                                //
                            lerp(texel01_l1, u_frac, texel11_l1)); //
                    }
                }

                vec4 dest_l1 = unpack4x8_srgb255_to_linear1(*pixel);

                // Cout = Cf * Af + Cb * (1 - Af)
                vec4 blended = src_color_l1;
                if (blended.a < 1.0f) {
                    blended = src_color_l1 * src_color_l1.a + (dest_l1 * (1.0f - src_color_l1.a));
                }
                // vec4 blended = dest_l1 * (1.0f - src_color_l1.a) + src_color_l1;

                *pixel = linear1_to_packed8x4_srgb255(blended);
            }
            u += ds_dx.x;
            v += ds_dx.y;
        }
        texel_u_row += ds_dy.x;
        texel_v_row += ds_dy.y;
    }
}

static auto draw_bitmap_avx2(Quadrilateral quad, vec2 offset, vec2 scale, f32 rotation, vec4 color, i32 texture_id,
    ivec2 uv_min, ivec2 uv_max, i32 screen_width, i32 screen_height, Rectangle2i* clip_rect, Framebuffer* buffer) {

    f32 model_width = (quad.br.x - quad.bl.x);
    f32 model_height = (quad.tr.y - quad.br.y);

    f32 scaled_width = (quad.br.x - quad.bl.x) * scale.x;
    f32 scaled_height = (quad.tr.y - quad.br.y) * scale.y;

    /*printf("Model width: %f\n", model_width);*/
    /*printf("Scaled width: %f\n", scaled_width);*/

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

    f32 scaled_du = (scale.x * (f32)tex_range_u) / (scaled_width);
    f32 scaled_dv = (scale.y * (f32)tex_range_v) / (scaled_height);

    vec2 ds_dx = vec2(M_c_to_m.xx * scaled_du, M_c_to_m.xy * scaled_dv);
    vec2 ds_dy = vec2(M_c_to_m.yx * scaled_du, M_c_to_m.yy * scaled_dv);

    // model space, but scaled!
    f32 start_x_m = (f32)min_x - translation.x;
    f32 start_y_m = (f32)min_y - translation.y;

    // Start coordinate for the "current" texel row in texel space
    f32 texel_u_row = (start_x_m * M_c_to_m.xx + start_y_m * M_c_to_m.yx + model_width * 0.5f) * scaled_du + (f32)u_min;
    f32 texel_v_row = (start_x_m * M_c_to_m.xy + start_y_m * M_c_to_m.yy + model_height * 0.5f) * scaled_dv + (f32)v_min;

    color_v8 default_color_l1_v8 = srgb_to_linear1_2(color);
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
        const u32 Lane_Width = 8;
        for (int x = min_x; x < max_x; x += Lane_Width) {
            u8* dest = ((u8*)buffer->memory + (y * buffer->pitch) + (x * buffer->bytes_per_pixel));
            u32* pixel = (u32*)dest;

            __m256i is_inside_v8 = _mm256_set1_epi32(0xFFFFFFFF);
            if (rotation != 0.0f) {
                __m256 cp_x_v8 = _mm256_set1_ps((f32)x);
                __m256 incr_v8 = _mm256_setr_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f);
                cp_x_v8 = _mm256_add_ps(cp_x_v8, incr_v8);
                __m256 cp_y_v8 = _mm256_set1_ps((f32)y);

                __m256 dot1_v8;
                {
                    __m256 x_v8 = _mm256_set1_ps(bl_c.x);
                    __m256 y_v8 = _mm256_set1_ps(bl_c.y);

                    x_v8 = _mm256_sub_ps(cp_x_v8, x_v8);
                    y_v8 = _mm256_sub_ps(cp_y_v8, y_v8);

                    __m256 edge1_x_v8 = _mm256_set1_ps(edge1.x);
                    __m256 edge1_y_v8 = _mm256_set1_ps(edge1.y);

                    dot1_v8 = dot(x_v8, y_v8, edge1_x_v8, edge1_y_v8);
                }
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
                {
                    const __m256 zero_v8 = _mm256_setzero_ps();
                    __m256 is_inside_dot1_v8 = _mm256_cmp_ps(dot1_v8, zero_v8, _CMP_GT_OQ);
                    __m256 is_inside_dot2_v8 = _mm256_cmp_ps(dot2_v8, zero_v8, _CMP_GT_OQ);
                    __m256 is_inside_dot3_v8 = _mm256_cmp_ps(dot3_v8, zero_v8, _CMP_GT_OQ);
                    __m256 is_inside_dot4_v8 = _mm256_cmp_ps(dot4_v8, zero_v8, _CMP_GT_OQ);

                    __m256 is_inside_v8f = _mm256_and_ps(is_inside_dot1_v8, is_inside_dot2_v8);
                    is_inside_v8f = _mm256_and_ps(is_inside_v8f, is_inside_dot3_v8);
                    is_inside_v8f = _mm256_and_ps(is_inside_v8f, is_inside_dot4_v8);
                    is_inside_v8 = _mm256_castps_si256(is_inside_v8f);
                }
            }

            color_v8 src_color_l1_v8 = {};
            if (texture_id == 0) {
                src_color_l1_v8 = default_color_l1_v8;
            }
            else {
                __m256i one_v8 = _mm256_set1_epi32(1);
                __m256i x0_v8 = _mm256_cvttps_epi32(_mm256_floor_ps(u_v8));
                __m256i y0_v8 = _mm256_cvttps_epi32(_mm256_floor_ps(v_v8));
                __m256i x1_v8 = _mm256_add_epi32(x0_v8, one_v8);
                __m256i y1_v8 = _mm256_add_epi32(y0_v8, one_v8);

                __m256 u_frac_v8 = _mm256_sub_ps(u_v8, _mm256_floor_ps(u_v8));
                __m256 v_frac_v8 = _mm256_sub_ps(v_v8, _mm256_floor_ps(v_v8));

                x0_v8 = clamp_i32_v8(u_min, x0_v8, u_max);
                x1_v8 = clamp_i32_v8(u_min, x1_v8, u_max);
                y0_v8 = clamp_i32_v8(v_min, y0_v8, v_max);
                y1_v8 = clamp_i32_v8(v_min, y1_v8, v_max);

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

                __m256i texel00_srgba255_v8 = _mm256_i32gather_epi32((const i32*)data, texel_idx_00_v8, sizeof(u32));
                __m256i texel01_srgba255_v8 = _mm256_i32gather_epi32((const i32*)data, texel_idx_01_v8, sizeof(u32));
                __m256i texel10_srgba255_v8 = _mm256_i32gather_epi32((const i32*)data, texel_idx_10_v8, sizeof(u32));
                __m256i texel11_srgba255_v8 = _mm256_i32gather_epi32((const i32*)data, texel_idx_11_v8, sizeof(u32));

                color_v8 texel00_v8 = get_color(texel00_srgba255_v8);
                color_v8 texel10_v8 = get_color(texel10_srgba255_v8);
                color_v8 texel01_v8 = get_color(texel01_srgba255_v8);
                color_v8 texel11_v8 = get_color(texel11_srgba255_v8);

                src_color_l1_v8 = lerp(                      //
                    lerp(texel00_v8, u_frac_v8, texel10_v8), //
                    v_frac_v8,                               //
                    lerp(texel01_v8, u_frac_v8, texel11_v8)  //
                );
            }

            __m256i destination_v8 = _mm256_loadu_si256((const __m256i*)pixel);
            color_v8 destination_color_v8 = get_color(destination_v8);
            color_v8 blended_v8 = blend_color_v8(destination_color_v8, src_color_l1_v8);

            __m256i pixel_v8 = pack4x8_linear1_to_srgb255(blended_v8);
            __m256i result_v8 = _mm256_blendv_epi8(destination_v8, pixel_v8, is_inside_v8);

            i32 lanes_remaining = hm::min(max_x - x, (i32)Lane_Width);
            if (lanes_remaining < (i32)Lane_Width) {
                // build a partial mask and use _mm256_maskstore_epi32
                i32 partial_mask[8] = {};
                for (i32 i = 0; i < lanes_remaining; i++)
                    partial_mask[i] = 0xFFFFFFFF;
                __m256i store_mask = _mm256_loadu_si256((__m256i*)partial_mask);
                _mm256_maskstore_epi32((i32*)pixel, store_mask, pixel_v8);
            }
            else {
                _mm256_storeu_si256((__m256i*)pixel, result_v8);
            }

            {
                __m256 du_dx_v8 = _mm256_set1_ps(Lane_Width * ds_dx.x);
                __m256 dv_dx_v8 = _mm256_set1_ps(Lane_Width * ds_dx.y);
                u_v8 = _mm256_add_ps(u_v8, du_dx_v8);
                v_v8 = _mm256_add_ps(v_v8, dv_dx_v8);
            }
        }
        texel_u_row += ds_dy.x;
        texel_v_row += ds_dy.y;
    }
}

extern "C" __declspec(dllexport) RENDERER_INIT(win32_renderer_init) {
    initialize_core_lib();
    log_info("Using software renderer.");

    // TODO: We should check for need of resizing on every draw call.

    // resize_dib_section(&state.global_offscreen_buffer, 48, 58);
    resize_dib_section(&state.platform_render_info, CLIENT_WIDTH, CLIENT_HEIGHT);
    log_info("Resolution: %d x %d", CLIENT_WIDTH, CLIENT_HEIGHT);

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
    null_texture->bytes_per_pixel = BYTES_PER_PIXEL;
    null_texture->size = null_texture->count * null_texture->bytes_per_pixel;
    null_texture->data = allocate<u32>(state.permanent);
    u32* data = (u32*)null_texture->data;
    *data = pack_color_8x4(vec4(1.0f, 0.0f, 0.0f, 1.0f));
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

auto execute_render_commands(i32 job_id, RenderGroup* group, //
    i32* command_render_order,                               //
    Rectangle2i clip_rect,                                   //
    Framebuffer* framebuffer, MemoryArena& transient) -> void {

    for (i32 i = 0; i < group->sort_keys.count(); i++) {
        u64 base_address = group->sort_entries_offset[command_render_order[i]];
        RenderGroupEntryHeader* header = (RenderGroupEntryHeader*)(group->push_buffer + base_address);
        base_address += sizeof(RenderGroupEntryHeader);

        void* data = (u8*)header + sizeof(*header);
        switch (header->type) {
        case RenderCommands_RenderEntryClear: {
            RenderEntryClear* entry = (RenderEntryClear*)data;
            clear(framebuffer->width, framebuffer->height, entry->color, clip_rect, *framebuffer);
            base_address += sizeof(*entry);
        } break;
        case RenderCommands_RenderEntryClearCheckPattern: {
            auto* entry = (RenderEntryClearCheckPattern*)data;
            clear_check_pattern(*framebuffer, clip_rect, entry->color1, entry->color2);
            base_address += sizeof(*entry);
        } break;
        case RenderCommands_RenderEntryLine: {
            RenderEntryLine* entry = (RenderEntryLine*)data;
            render_line_gambetta(entry->start, entry->end, entry->color, clip_rect, *framebuffer, transient);
            base_address += sizeof(*entry);
        } break;
        case RenderCommands_RenderEntryCircle: {
            auto entry = (RenderEntryCircle*)data;
            render_circle_bresenham(entry->P, entry->radius, entry->color, clip_rect, *framebuffer);
            base_address += sizeof(*entry);
        } break;
        case RenderCommands_RenderEntryFilledCircle: {
            auto entry = (RenderEntryFilledCircle*)data;
            render_filled_circle_bresenham(entry->P, entry->radius, entry->color, clip_rect, *framebuffer);
            base_address += sizeof(*entry);
        } break;
        case RenderCommands_RenderEntryTriangle: {
            auto entry = (RenderEntryTriangle*)data;
            render_triangle_writeframe_gambetta(entry->P0, entry->P1, entry->P2, entry->color, clip_rect, *framebuffer, transient);
            base_address += sizeof(*entry);
        } break;
        case RenderCommands_RenderEntryFilledTriangle: {
            auto entry = (RenderEntryFilledTriangle*)data;
            render_triangle_filled_gambetta(entry->P0, entry->P1, entry->P2, entry->color, clip_rect, *framebuffer, transient);
            base_address += sizeof(*entry);
        } break;
        case RenderCommands_RenderEntryShadedTriangle: {
            auto entry = (RenderEntryShadedTriangle*)data;
            render_shaded_triangle_gambetta(entry->P0, entry->P1, entry->P2, entry->h0, entry->h1, entry->h2,
                entry->color, clip_rect, *framebuffer, transient);
            base_address += sizeof(*entry);
        } break;
        case RenderCommands_RenderEntryTriMesh: {
            auto entry = (RenderEntryTriMesh*)data;
            render_mesh_gambetta(                                                   //
                entry->model.vertices, entry->model.triangles, entry->model.colors, //
                entry->instances,                                                   //
                entry->world_to_view,                                               //
                entry->view_to_clip,                                                //
                false, clip_rect, *framebuffer, transient);
            base_address += sizeof(*entry);
        } break;
        case RenderCommands_RenderEntryTriMeshWireframe: {
            auto entry = (RenderEntryTriMesh*)data;
            render_mesh_gambetta(                                                   //
                entry->model.vertices, entry->model.triangles, entry->model.colors, //
                entry->instances,                                                   //
                entry->world_to_view,                                               //
                entry->view_to_clip,                                                //
                true, clip_rect, *framebuffer, transient);
            base_address += sizeof(*entry);
        } break;
        case RenderCommands_RenderEntryBitmap: {
            auto* entry = (RenderEntryBitmap*)data;
            draw_bitmap(entry->quad, entry->offset,                  //
                entry->scale, entry->rotation, entry->color,         //
                entry->texture_id, entry->uv_min, entry->uv_max,     //
                framebuffer->width, framebuffer->height, &clip_rect, //
                entry->border_thickness, entry->border_color,        //
                framebuffer);
            base_address += sizeof(*entry);
        } break;
        default: InvalidCodePath;
        }
    }
}

struct RenderTileJob {
    i32 id;
    RenderGroup* group;
    i32* command_render_order;
    Rectangle2i clip_rect;
    Framebuffer* framebuffer;
};

static PLATFORM_WORK_QUEUE_CALLBACK(execute_render_tile_job) {
    RenderTileJob* job = (RenderTileJob*)data;

    Assert(job);
    Assert(job->group);
    BEGIN_BLOCK("Execute render commands");

    execute_render_commands(job->id, job->group, job->command_render_order, job->clip_rect, job->framebuffer, *transient);
    END_BLOCK();
    MemoryBarrier(); // TODO: remove?
}

extern "C" __declspec(dllexport) RENDERER_RENDER(win32_renderer_render) {

    i32 const tile_count_x = 1;
    i32 const tile_count_y = 1;

    Framebuffer* buffer = &state.framebuffers[handle.v];
    // TODO: get frame buffer
    i32 width = buffer->width;
    i32 height = buffer->height;
    i32 tile_width = width / tile_count_x;
    i32 tile_height = height / tile_count_y;

    const i32 sse_width = 8;
    tile_width = ((tile_width + (sse_width - 1)) / sse_width) * sse_width;

    RenderTileJob render_tile_jobs[tile_count_x * tile_count_y];

    i32* command_render_order = merge_sort_indices(group->sort_keys.data(), group->sort_keys.count(), &state.transient);

    i32 job_count = 0;
    for (i32 y = 0; y < tile_count_y; y++) {
        for (i32 x = 0; x < tile_count_x; x++) {
            RenderTileJob* job = render_tile_jobs + job_count++;

            Rectangle2i rect = {};
            rect.min_x = (x * tile_width);
            rect.max_x = rect.min_x + tile_width;
            if (x == tile_count_x - 1) {
                rect.max_x = width;
            }

            rect.min_y = (y * tile_height);
            rect.max_y = rect.min_y + tile_height;
            if (y == tile_count_y - 1) {
                rect.max_y = height;
            }

            job->id = job_count;
            job->clip_rect = rect;
            job->group = group;
            job->command_render_order = command_render_order;
            job->framebuffer = buffer;

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

    i32 width = state.platform_render_info.buffer.width;
    i32 height = state.platform_render_info.buffer.height;

    u32 result = StretchDIBits(                   //
        device_context,                           //
        0, 0,                                     //
        width, height, 0, 0,                      //
        width, height,                            //
        state.platform_render_info.buffer.memory, //
        &state.platform_render_info.Info,         //
        DIB_RGB_COLORS, SRCCOPY                   //
    );
    if (result == 0 || result == GDI_ERROR) {
        log_error("StretchDIBits failed\n");
    }

    ReleaseDC(win32_context->window, device_context);
}

extern "C" __declspec(dllexport) RENDERER_CREATE_FRAMEBUFFER(win32_renderer_create_framebuffer) {
    Assert(!state.framebuffers.is_full());

    FrameBufferHandle handle = { .v = (i32)state.framebuffers.count() };
    Framebuffer* f = state.framebuffers.push();
    resize_frame_buffer(f, width, height);
    return handle;
}

extern "C" __declspec(dllexport) RENDERER_APPLY_FRAMEBUFFER(win32_renderer_apply_framebuffer) {

    Framebuffer* framebuffer = &state.framebuffers[handle.v];
    Win32RenderInfo render_info = state.platform_render_info;
    Assert(render_info.buffer.bytes_per_pixel == framebuffer->bytes_per_pixel);

    f32 pixel_size_x = 1;
    f32 pixel_size_y = 1;
    if (width >= framebuffer->width) {
        pixel_size_x = (f32)width / framebuffer->width;
    }
    if (height >= framebuffer->height) {
        pixel_size_y = (f32)height / framebuffer->height;
    }

    i32 start_x = hm::max(offset_x, 0);
    i32 end_x = hm::min(offset_x + width, render_info.buffer.width);
    i32 start_y = hm::max(offset_y, 0);
    i32 end_y = hm::min(offset_y + height, render_info.buffer.height);

    for (i32 y = start_y; y < end_y; y++) {
        u32* dest = render_info.buffer.get_pixel(start_x, y);

        f32 frame_buffer_y = (y - offset_y) / pixel_size_y;
        u32* src = framebuffer->get_pixel(0, (i32)frame_buffer_y);
        for (i32 x = start_x; x < end_x; x++) {
            i32 frame_buffer_x = (i32)((x - offset_x) / pixel_size_x);
            *dest++ = *(src + frame_buffer_x);
        }
    }
}

extern "C" __declspec(dllexport) RENDERER_GET_COLOR(win32_renderer_get_color) {
    Color result = {};
    return result;
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

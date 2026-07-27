#include <renderers/renderer.hpp>

auto create_frame_buffer(MemoryArena& arena, i32 width, i32 height) -> Framebuffer {
    Framebuffer buffer = {};
    buffer.width = width;
    buffer.height = height;
    buffer.bytes_per_pixel = BYTES_PER_PIXEL;
    buffer.pitch = buffer.width * buffer.bytes_per_pixel;
    buffer.memory_size = buffer.pitch * buffer.height;
    buffer.memory = arena.allocate(buffer.memory_size);
    buffer.z_buffer = Array<f32>::create((u64)width * height, arena);
    return buffer;
}

auto initialize_renderer_lib() -> void {
    if (cpu_supports_avx512f()) {
        apply_frame_buffer = apply_frame_buffer_AVX512;
    }
    else {
        apply_frame_buffer = apply_frame_buffer_scalar;
    }
}

auto apply_frame_buffer_AVX512(           //
    Framebuffer* src_buffer, Tile* tile,  //
    Framebuffer* dest_buffer, ivec2 scale //
    ) -> void {
    if (!tile->is_dirty) {
        return;
    }

    Assert(dest_buffer->bytes_per_pixel == src_buffer->bytes_per_pixel);
    Assert(dest_buffer->width >= src_buffer->width);
    Assert(dest_buffer->height >= src_buffer->height);

    // We know this is within bounds.
    ivec2 src_start = { tile->rect.min_x, tile->rect.min_y };
    ivec2 src_end = { tile->rect.max_x, tile->rect.max_y };

    ivec2 dest_start = { tile->rect.min_x * scale.x, tile->rect.min_y * scale.y };
    ivec2 dest_end = { tile->rect.max_x * scale.x, tile->rect.max_y * scale.y };

    const i32 LANE_COUNT = 16;
    if (scale.x == 1) {
        for (i32 y = dest_start.y; y < dest_end.y; y++) {
            u32* dest = GET_PIXEL(dest_buffer, dest_start.x, y);

            i32 offset_y = y - dest_start.y;
            u32* src = GET_PIXEL(src_buffer, src_start.x, (i32)(src_start.y + offset_y));
            for (i32 x = dest_start.x; x < dest_end.x; x += LANE_COUNT) {
                __m512i src_colors_v16 = _mm512_load_epi32(src);
                __mmask16 mask_16 = _mm512_cmpgt_epu32_mask(src_colors_v16, _mm512_setzero_si512());
                _mm512_mask_storeu_epi32((void*)dest, mask_16, src_colors_v16);
                dest += LANE_COUNT;
                src += LANE_COUNT;
            }
        }
    }
    else if (scale.x == 4) {
        __m512i repeat_v16 = _mm512_setr_epi32(0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3);
        i32 dx = LANE_COUNT / 4;
        f32 dy = 1.0f / 4.0f;
        f32 offset_y = 0;
        for (i32 y = dest_start.y; y < dest_end.y; y++) {
            u32* dest = GET_PIXEL(dest_buffer, dest_start.x, y);

            u32* src = GET_PIXEL(src_buffer, src_start.x, (i32)(src_start.y + offset_y));
            for (i32 x = dest_start.x; x < dest_end.x; x += 16) {
                __m128i src_v4 = _mm_load_si128((__m128i const*)(src));
                __m512i src_colors_v16 = _mm512_permutexvar_epi32(repeat_v16, _mm512_castsi128_si512(src_v4));
                // Store all values that are not 0
                __mmask16 mask16 = _mm512_cmpgt_epu32_mask(src_colors_v16, _mm512_setzero_si512());
                _mm512_mask_storeu_epi32((void*)dest, mask16, src_colors_v16);
                dest += LANE_COUNT;
                src += dx;
            }
            offset_y += dy;
        }
    }
    else {
        InvalidCodePath;
    }
}

auto apply_frame_buffer_scalar(Framebuffer* src_buffer, Tile* tile, Framebuffer* dest_buffer, ivec2 scale) -> void {
    if (!tile->is_dirty) {
        return;
    }
    Assert(dest_buffer->bytes_per_pixel == src_buffer->bytes_per_pixel);
    Assert(dest_buffer->width >= src_buffer->width);
    Assert(dest_buffer->height >= src_buffer->height);

    // We know this is within bounds.
    vec2 src_start = { (f32)tile->rect.min_x, (f32)tile->rect.min_y };
    vec2 src_end = { (f32)tile->rect.max_x, (f32)tile->rect.max_y };

    ivec2 dest_start = { tile->rect.min_x * scale.x, tile->rect.min_y * scale.y };
    ivec2 dest_end = { tile->rect.max_x * scale.x, tile->rect.max_y * scale.y };

    f32 dy = 1.0f / scale.y;
    f32 dx = 1.0f / scale.x;
    f32 offset_y = 0.0f;
    for (i32 y = dest_start.y; y < dest_end.y; y++) {
        u32* dest = GET_PIXEL(dest_buffer, dest_start.x, y);

        u32* src = GET_PIXEL(src_buffer, 0, (i32)(src_start.y + offset_y));
        f32 offset_x = 0.0f;
        for (i32 x = dest_start.x; x < dest_end.x; x++) {
            i32 src_buffer_x = (i32)(src_start.x + offset_x);
            u32 src_color = *(src + src_buffer_x);
            if (src_color > 0) {
                *dest = src_color;
            }
            dest++;
            offset_x += dx;
        }
        offset_y += dy;
    }
}

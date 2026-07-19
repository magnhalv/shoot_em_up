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

auto apply_frame_buffer(Framebuffer* src_buffer, Framebuffer* dest_buffer, i32 width, i32 height, i32 offset_x, i32 offset_y) {
    Assert(dest_buffer->bytes_per_pixel == src_buffer->bytes_per_pixel);

    f32 pixel_size_x = 1;
    f32 pixel_size_y = 1;
    if (width >= src_buffer->width) {
        pixel_size_x = (f32)width / src_buffer->width;
    }
    if (height >= src_buffer->height) {
        pixel_size_y = (f32)height / src_buffer->height;
    }

    const i32 start_x = hm::max(offset_x, 0);
    const i32 end_x = hm::min(offset_x + width, dest_buffer->width);
    const i32 start_y = hm::max(offset_y, 0);
    const i32 end_y = hm::min(offset_y + height, dest_buffer->height);

    const i32 LANE_COUNT = 16;
    Assert((end_x - start_x) % LANE_COUNT == 0);

    f32 frame_buffer_y = (start_y - offset_y) / pixel_size_y;
    f32 frame_buffer_y1 = ((start_y + 1) - offset_y) / pixel_size_y;
    f32 dy = frame_buffer_y1 - frame_buffer_y;

    f32 frame_buffer_start_x = ((start_x - offset_x) / pixel_size_x);
    f32 frame_buffer_x_1 = (((start_x + 1) - offset_x) / pixel_size_x);
    f32 dx = frame_buffer_x_1 - frame_buffer_start_x;

    if ((i32)pixel_size_x == 1) {
        for (i32 y = start_y; y < end_y; y++) {
            u32* dest = GET_PIXEL(dest_buffer, start_x, y);

            u32* src = GET_PIXEL(src_buffer, 0, (i32)frame_buffer_y);
            for (i32 x = start_x; x < end_x; x += LANE_COUNT) {
                i32 x_idx = x - start_x;
                __m512i src_colors_v16 = _mm512_load_epi32(src + x_idx);
                __mmask16 mask_16 = _mm512_cmpgt_epu32_mask(src_colors_v16, _mm512_setzero_si512());
                _mm512_mask_storeu_epi32((void*)dest, mask_16, src_colors_v16);
                dest += LANE_COUNT;
            }
            frame_buffer_y += dy;
        }
    }
    else if ((i32)pixel_size_x == 4) {
        __m512i repeat_v16 = _mm512_setr_epi32(0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3);
        for (i32 y = start_y; y < end_y; y++) {
            u32* dest = GET_PIXEL(dest_buffer, start_x, y);

            u32* src = GET_PIXEL(src_buffer, 0, (i32)frame_buffer_y);
            for (i32 x = start_x; x < end_x; x += 16) {
                i32 x_idx = x - start_x;
                i32 src_idx = x_idx / 4;
                __m128i src_v4 = _mm_load_si128((__m128i const*)(src + src_idx));
                __m512i src_colors_v16 = _mm512_permutexvar_epi32(repeat_v16, _mm512_castsi128_si512(src_v4));
                __mmask16 mask16 = _mm512_cmpgt_epu32_mask(src_colors_v16, _mm512_setzero_si512());
                _mm512_mask_storeu_epi32((void*)dest, mask16, src_colors_v16);
                dest += 16;
            }
            frame_buffer_y += dy;
        }
    }
    else {
        __m512 lane_offsets = _mm512_setr_ps(
            0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f);
        for (i32 y = start_y; y < end_y; y++) {
            u32* dest = GET_PIXEL(dest_buffer, start_x, y);

            u32* src = GET_PIXEL(src_buffer, 0, (i32)frame_buffer_y);
            for (i32 x = start_x; x < end_x; x += 16) {
                i32 x_idx = x - start_x;
                f32 x_start = frame_buffer_start_x;

                __m512 indices_ps = _mm512_set1_ps(x_start);
                __m512 dx_ps = _mm512_set1_ps(dx);
                __m512 x_idx_ps = _mm512_set1_ps((f32)x_idx);
                x_idx_ps = _mm512_add_ps(x_idx_ps, lane_offsets);
                dx_ps = _mm512_mul_ps(dx_ps, x_idx_ps);
                indices_ps = _mm512_add_ps(indices_ps, dx_ps);
                __m512i indices = _mm512_cvttps_epi32(indices_ps);
                __m512i src_colors = _mm512_i32gather_epi32(indices, src, 4);
                __mmask16 mask = _mm512_cmpgt_epu32_mask(src_colors, _mm512_setzero_si512());
                _mm512_mask_storeu_epi32((void*)dest, mask, src_colors);
                dest += 16;
            }
            frame_buffer_y += dy;
        }
    }
}

auto apply_frame_buffer_scalar(Framebuffer* src_buffer, Framebuffer* dest_buffer, i32 width, i32 height, i32 offset_x, i32 offset_y) {
    Assert(dest_buffer->bytes_per_pixel == src_buffer->bytes_per_pixel);

    f32 pixel_size_x = 1;
    f32 pixel_size_y = 1;
    if (width >= src_buffer->width) {
        pixel_size_x = (f32)width / src_buffer->width;
    }
    if (height >= src_buffer->height) {
        pixel_size_y = (f32)height / src_buffer->height;
    }

    const i32 start_x = hm::max(offset_x, 0);
    const i32 end_x = hm::min(offset_x + width, dest_buffer->width);
    const i32 start_y = hm::max(offset_y, 0);
    const i32 end_y = hm::min(offset_y + height, dest_buffer->height);

    f32 frame_buffer_y = (start_y - offset_y) / pixel_size_y;
    f32 frame_buffer_y1 = ((start_y + 1) - offset_y) / pixel_size_y;
    f32 dy = frame_buffer_y1 - frame_buffer_y;
    for (i32 y = start_y; y < end_y; y++) {
        u32* dest = GET_PIXEL(dest_buffer, start_x, y);

        u32* src = GET_PIXEL(src_buffer, 0, (i32)frame_buffer_y);
        for (i32 x = start_x; x < end_x; x++) {
            i32 frame_buffer_x = (i32)((x - offset_x) / pixel_size_x);
            u32 src_color = *(src + frame_buffer_x);
            if (src_color > 0) {
                *dest = src_color;
            }
            dest++;
        }
        frame_buffer_y += dy;
    }
}

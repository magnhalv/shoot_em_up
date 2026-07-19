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

    i32 start_x = hm::max(offset_x, 0);
    i32 end_x = hm::min(offset_x + width, dest_buffer->width);
    i32 start_y = hm::max(offset_y, 0);
    i32 end_y = hm::min(offset_y + height, dest_buffer->height);

    // f32 frame_buffer_y = (start_y - offset_y) / pixel_size_y;
    // f32 frame_buffer_y1 = ((start_y + 1) - offset_y) / pixel_size_y;
    // f32 dy = frame_buffer_y1 - frame_buffer_y;
    for (i32 y = start_y; y < end_y; y++) {
        u32* dest = GET_PIXEL(dest_buffer, start_x, y);

        f32 frame_buffer_y = (y - offset_y) / pixel_size_y;
        u32* src = GET_PIXEL(src_buffer, 0, (i32)frame_buffer_y);
        for (i32 x = start_x; x < end_x; x++) {
            i32 frame_buffer_x = (i32)((x - offset_x) / pixel_size_x);
            u32 src_color = *(src + frame_buffer_x);
            if (src_color > 0) {
                *dest = src_color;
            }
            dest++;
        }
        // frame_buffer_y += dy;
    }
}

#include <math/util.hpp>
#include <math/vec2.h>
#include <math/vec4.h>
#include <platform/types.h>

struct FrameBuffer {
    void* memory;
    i32 memory_size;
    i32 width;
    i32 height;
    i32 bytes_per_pixel;
    i32 pitch;
};

auto inline get_color(FrameBuffer* buffer, i32 x, i32 y) -> u32 {
    u32* result = (u32*)(((u8*)buffer->memory) + (y * buffer->pitch) + (x * buffer->bytes_per_pixel));
    return *result;
}
auto inline set_color(FrameBuffer* buffer, u32 color, i32 x, i32 y) -> void {
    u32* result = (u32*)(((u8*)buffer->memory) + (y * buffer->pitch) + (x * buffer->bytes_per_pixel));
    *result = color;
}

auto inline render_line_bresenham_x(
    i32 xs, i32 ys, i32 xe, i32 ye, i32 dx, i32 dy, vec4 color, Rectangle2i clip_rect, FrameBuffer* buffer) {
    i32 e = -(dx >> 1);

    i32 x = xs;
    i32 y = ys;
    u32 c = (round_f32_to_i32(color.a * 255.0f) << 24) | (round_f32_to_i32(color.r * 255.0f) << 16) |
        (round_f32_to_i32(color.g * 255.0f) << 8) | (round_f32_to_i32(color.b * 255.0f));
    while (x <= xe) {
        u32* dest = (u32*)((u8*)buffer->memory + (y * buffer->pitch) + (x * buffer->bytes_per_pixel));
        *dest = c;
        x++;
        e += dy;
        if (e >= 0) {
            y = y + 1;
            e -= dx;
        }
    }
}

auto inline render_line_bresenham_y(
    i32 xs, i32 ys, i32 xe, i32 ye, i32 dx, i32 dy, vec4 color, Rectangle2i clip_rect, FrameBuffer* buffer) {
    i32 e = -(dy >> 1);

    i32 x = xs;
    i32 y = ys;
    u32 c = (round_f32_to_i32(color.a * 255.0f) << 24) | (round_f32_to_i32(color.r * 255.0f) << 16) |
        (round_f32_to_i32(color.g * 255.0f) << 8) | (round_f32_to_i32(color.b * 255.0f));
    while (y <= ye) {
        u32* dest = (u32*)((u8*)buffer->memory + (y * buffer->pitch) + (x * buffer->bytes_per_pixel));
        *dest = c;
        y++;
        e += dx;
        if (e >= 0) {
            x = x + 1;
            e -= dy;
        }
    }
}

auto inline render_line_bresenham(vec2 start, vec2 end, vec4 color, Rectangle2i clip_rect, FrameBuffer* buffer) {
    i32 xs = (i32)start.x;
    i32 xe = (i32)end.x;
    i32 ys = (i32)start.y;
    i32 ye = (i32)end.y;

    i32 dx = xe - xs;
    i32 dy = ye - ys;

    if (dx > dy) {
        render_line_bresenham_x(xs, ys, xe, ye, dx, dy, color, clip_rect, buffer);
    }
    else {
        render_line_bresenham_x(xs, ys, xe, ye, dx, dy, color, clip_rect, buffer);
    }
}

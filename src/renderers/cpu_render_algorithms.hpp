#include <platform/platform.h>
#include <platform/types.h>

#include <math/util.hpp>
#include <math/vec2.h>
#include <math/vec4.h>

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

    i32 y_inc = dy > 0 ? 1 : -1;
    dy = dy > 0 ? dy : -dy;
    i32 x = xs;
    i32 y = ys;
    u32 c = (round_f32_to_i32(color.a * 255.0f) << 24) | (round_f32_to_i32(color.r * 255.0f) << 16) |
        (round_f32_to_i32(color.g * 255.0f) << 8) | (round_f32_to_i32(color.b * 255.0f));
    while (x <= xe) {
        if (is_inside(ivec2(x, y), clip_rect)) {
            u32* dest = (u32*)((u8*)buffer->memory + (y * buffer->pitch) + (x * buffer->bytes_per_pixel));
            *dest = c;
        }
        x++;
        e += dy;
        if (e >= 0) {
            y += y_inc;
            e -= dx;
        }
    }
}

auto inline render_line_bresenham_y(
    i32 xs, i32 ys, i32 xe, i32 ye, i32 dx, i32 dy, vec4 color, Rectangle2i clip_rect, FrameBuffer* buffer) {
    i32 e = -(dy >> 1);

    i32 x = xs;
    i32 y = ys;

    i32 x_inc = dx > 0 ? 1 : -1;
    dx = abs(dx);

    u32 c = (round_f32_to_i32(color.a * 255.0f) << 24) | (round_f32_to_i32(color.r * 255.0f) << 16) |
        (round_f32_to_i32(color.g * 255.0f) << 8) | (round_f32_to_i32(color.b * 255.0f));
    while (y <= ye) {
        if (is_inside(ivec2(x, y), clip_rect)) {
            u32* dest = (u32*)((u8*)buffer->memory + (y * buffer->pitch) + (x * buffer->bytes_per_pixel));
            *dest = c;
        }

        y++;
        e += dx;
        if (e >= 0) {
            x = x + x_inc;
            e -= dy;
        }
    }
}

auto inline render_line_bresenham(vec2 start, vec2 end, vec4 color, Rectangle2i clip_rect, FrameBuffer* buffer) {
    i32 xs = round_f32_to_i32(start.x);
    i32 xe = round_f32_to_i32(end.x);
    i32 ys = round_f32_to_i32(start.y);
    i32 ye = round_f32_to_i32(end.y);

    i32 dx = xe - xs;
    i32 dy = ye - ys;

    bool is_x_major = abs(dx) >= abs(dy);
    if (is_x_major) {
        if (dx >= 0) {
            render_line_bresenham_x(xs, ys, xe, ye, dx, dy, color, clip_rect, buffer);
        }
        else {
            render_line_bresenham_x(xe, ye, xs, ys, -dx, -dy, color, clip_rect, buffer);
        }
    }
    else {
        if (dy >= 0) {
            render_line_bresenham_y(xs, ys, xe, ye, dx, dy, color, clip_rect, buffer);
        }
        else {
            render_line_bresenham_y(xe, ye, xs, ys, -dx, -dy, color, clip_rect, buffer);
        }
    }
}

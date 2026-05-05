#include <platform/platform.h>
#include <platform/types.h>

#include <core/color.hpp>
#include <core/memory_arena.h>
#include <engine/array.h>

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

    auto set_pixel(i32 x, i32 y, u32 color) -> void {
        Assert(x >= 0 && x < width);
        Assert(y >= 0 && y < height);
        Assert(sizeof(color) == sizeof(u32));
        u32* dest = (u32*)((u8*)memory + (y * pitch) + (x * bytes_per_pixel));
        *dest = color;
    }
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
            buffer->set_pixel(x, y, c);
        }

        y++;
        e += dx;
        if (e >= 0) {
            x = x + x_inc;
            e -= dy;
        }
    }
}

auto inline render_line_bresenham(vec2 start, vec2 end, vec4 color, Rectangle2i clip_rect, FrameBuffer* buffer) -> void {
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

auto inline set_pixel(i32 x, i32 y, u32 color, Rectangle2i clip_rect, FrameBuffer* buffer) {
    if (is_inside(ivec2(x, y), clip_rect)) {
        buffer->set_pixel(x, y, color);
    }
}
auto inline set_8_pixels(i32 x, i32 y, i32 cx, i32 cy, u32 color, Rectangle2i clip_rect, FrameBuffer* buffer) {
    set_pixel(cx + x, cy + y, color, clip_rect, buffer);
    set_pixel(cx - x, cy + y, color, clip_rect, buffer);
    render_line_bresenham(vec2((f32)cx + x, (f32)(cy + y)), vec2((f32)cx - x, (f32)cy + y), vec4(1.0, 0, 0, 1), clip_rect, buffer);

    set_pixel(cx + x, cy - y, color, clip_rect, buffer);
    set_pixel(cx - x, cy - y, color, clip_rect, buffer);

    set_pixel(cx + y, cy + x, color, clip_rect, buffer);
    set_pixel(cx - y, cy + x, color, clip_rect, buffer);

    set_pixel(cx + y, cy - x, color, clip_rect, buffer);
    set_pixel(cx - y, cy - x, color, clip_rect, buffer);
}

auto inline render_circle_bresenham(vec2 P, f32 radius, vec4 color, Rectangle2i clip_rect, FrameBuffer* buffer) -> void {
    i32 x = 0;
    i32 y = round_f32_to_i32(radius);
    i32 e = -y;

    i32 px = round_f32_to_i32(P.x);
    i32 py = round_f32_to_i32(P.y);
    u32 c = ncolor_to_u32(color);
    while (x <= y) {
        set_8_pixels(x, y, px, py, c, clip_rect, buffer);
        e = e + (2 * x) + 1;
        x++;
        if (e >= 0) {
            e = e - (2 * y) + 2;
            y--;
        }
    }
}

auto inline interpolate(i32 i0, i32 d0, i32 i1, i32 d1, MemoryArena& arena) -> Array<i32> {
    Assert(i0 <= i1);
    if (i0 == i1) {
        auto result = Array<i32>::create_proper(1, arena);
        result[0] = d0;
        return result;
    }
    i32 length = (i1 - i0) + 1;
    auto result = Array<i32>::create_proper(length, arena);

    f32 a = ((f32)(d1 - d0)) / (i1 - i0); // dx/ix
    f32 d = (f32)d0;
    for (i32 i = 0; i < length; i++) {
        result[i] = round_f32_to_i32(d);
        d += a;
    }
    return result;
}

auto inline render_line_gambetta(vec2 P0, vec2 P1, vec4 color, Rectangle2i clip_rect, FrameBuffer* buffer, MemoryArena& arena)
    -> void {
    u32 c = ncolor_to_u32(color);
    if (abs(P1.x - P0.x) > abs(P1.y - P0.y)) {
        if (P0.x > P1.x) {
            swap(P0, P1);
        }
        i32 x0 = round_f32_to_i32(P0.x);
        i32 y0 = round_f32_to_i32(P0.y);
        i32 x1 = round_f32_to_i32(P1.x);
        i32 y1 = round_f32_to_i32(P1.y);
        Array<i32> ys = interpolate(x0, y0, x1, y1, arena);
        for (i32 x = x0; x <= x1; x++) {
            set_pixel(x, ys[x - x0], c, clip_rect, buffer);
        }
    }
    else {
        if (P0.y > P1.y) {
            swap(P0, P1);
        }
        i32 x0 = round_f32_to_i32(P0.x);
        i32 y0 = round_f32_to_i32(P0.y);
        i32 x1 = round_f32_to_i32(P1.x);
        i32 y1 = round_f32_to_i32(P1.y);
        Array<i32> xs = interpolate(y0, x0, y1, x1, arena);
        for (i32 y = y0; y <= y1; y++) {
            set_pixel(xs[y - y0], y, c, clip_rect, buffer);
        }
    }
}

auto inline render_triangle_gambetta(
    vec2 P0, vec2 P1, vec2 P2, vec4 color, Rectangle2i clip_rect, FrameBuffer* buffer, MemoryArena& arena) -> void {
    render_line_gambetta(P0, P1, color, clip_rect, buffer, arena);
    render_line_gambetta(P1, P2, color, clip_rect, buffer, arena);
    render_line_gambetta(P2, P0, color, clip_rect, buffer, arena);
}

auto inline render_filled_triangle_gambetta(
    vec2 P0, vec2 P1, vec2 P2, vec4 color, Rectangle2i clip_rect, FrameBuffer* buffer, MemoryArena& transient) -> void {

    if (P1.y < P0.y) {
        swap(P0, P1);
    }
    if (P2.y < P0.y) {
        swap(P0, P2);
    }
    if (P2.y < P1.y) {
        swap(P1, P2);
    }

    i32 x0 = round_f32_to_i32(P0.x);
    i32 y0 = round_f32_to_i32(P0.y);
    i32 x1 = round_f32_to_i32(P1.x);
    i32 y1 = round_f32_to_i32(P1.y);
    i32 x2 = round_f32_to_i32(P2.x);
    i32 y2 = round_f32_to_i32(P2.y);

    Array<i32> x01 = interpolate(y0, x0, y1, x1, transient);
    Array<i32> x12 = interpolate(y1, x1, y2, x2, transient);

    x01 = span(x01, 0, x01.count() - 1);
    Array<i32> x012 = concat(x01, x12, transient);

    Array<i32> x02 = interpolate(y0, x0, y2, x2, transient);

    i32 m = (i32)x012.count() / 2;
    Array<i32> x_left;
    Array<i32> x_right;
    if (x02[m] < x012[m]) {
        x_left = x02;
        x_right = x012;
    }
    else {
        x_right = x02;
        x_left = x012;
    }

    const u32 c = ncolor_to_u32(color);
    for (i32 y = y0; y <= y2; y++) {
        for (i32 x = x_left[y - y0]; x <= x_right[y - y0]; x++) {
            set_pixel(x, y, c, clip_rect, buffer);
        }
    }
}

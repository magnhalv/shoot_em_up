#include "math/math.hpp"
#include <core/list.hpp>
#include <core/memory.hpp>
#include <core/util.hpp>
#include <platform/platform.hpp>
#include <platform/types.hpp>

#include <core/color.hpp>
#include <core/memory_arena.hpp>
#include <engine/array.hpp>

#include <math/util.hpp>
#include <math/vec2.hpp>
#include <math/vec4.hpp>

struct FrameBuffer {
    void* memory;
    i32 memory_size;
    i32 width;
    i32 height;
    i32 bytes_per_pixel;
    i32 pitch;

    auto inline set_pixel(i32 x, i32 y, u32 color) -> void {
        Assert(x >= 0 && x < width);
        Assert(y >= 0 && y < height);
        Assert(sizeof(color) == sizeof(u32));
        u32* dest = (u32*)((u8*)memory + (y * pitch) + (x * bytes_per_pixel));
        *dest = color;
    }

    auto inline get_pixel(i32 x, i32 y) -> u32* {
        Assert(x >= 0 && x < width);
        Assert(y >= 0 && y < height);
        return (u32*)((u8*)memory + (y * pitch) + (x * bytes_per_pixel));
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
    i32 xs, i32 ys, i32 xe, i32 ye, i32 dx, i32 dy, u32 color, Rectangle2i clip_rect, FrameBuffer* buffer) {
    i32 e = -(dx >> 1);

    i32 y_inc = dy > 0 ? 1 : -1;
    dy = dy > 0 ? dy : -dy;
    i32 x = xs;
    i32 y = ys;
    while (x <= xe) {
        if (is_inside(ivec2(x, y), clip_rect)) {
            u32* dest = (u32*)((u8*)buffer->memory + (y * buffer->pitch) + (x * buffer->bytes_per_pixel));
            *dest = color;
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
    i32 xs, i32 ys, i32 xe, i32 ye, i32 dx, i32 dy, u32 color, Rectangle2i clip_rect, FrameBuffer* buffer) {
    i32 e = -(dy >> 1);

    i32 x = xs;
    i32 y = ys;

    i32 x_inc = dx > 0 ? 1 : -1;
    dx = abs(dx);

    while (y <= ye) {
        if (is_inside(ivec2(x, y), clip_rect)) {
            buffer->set_pixel(x, y, color);
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

    u32 color_packed = pack_color_8x4(color);
    bool is_x_major = abs(dx) >= abs(dy);
    if (is_x_major) {
        if (dx >= 0) {
            render_line_bresenham_x(xs, ys, xe, ye, dx, dy, color_packed, clip_rect, buffer);
        }
        else {
            render_line_bresenham_x(xe, ye, xs, ys, -dx, -dy, color_packed, clip_rect, buffer);
        }
    }
    else {
        if (dy >= 0) {
            render_line_bresenham_y(xs, ys, xe, ye, dx, dy, color_packed, clip_rect, buffer);
        }
        else {
            render_line_bresenham_y(xe, ye, xs, ys, -dx, -dy, color_packed, clip_rect, buffer);
        }
    }
}

auto inline set_pixel(i32 x, i32 y, u32 color, Rectangle2i clip_rect, FrameBuffer* buffer) {
    if (is_inside(ivec2(x, y), clip_rect)) {
        buffer->set_pixel(x, y, color);
    }
}

auto inline set_pixels(i32 x_start, i32 x_end, i32 y, u32 color, Rectangle2i clip_rect, FrameBuffer* buffer) {
    if (!in_range(clip_rect.min_y, y, clip_rect.max_y)) {
        return;
    }
    x_start = hm::max(x_start, clip_rect.min_x);
    x_end = hm::min(x_end, clip_rect.min_x);
    Assert(x_start <= x_end);
    set_memory_u32(buffer->get_pixel(x_start, y), color, x_end - x_start);
}

auto inline set_8_pixels(i32 x, i32 y, i32 cx, i32 cy, u32 color, Rectangle2i clip_rect, FrameBuffer* buffer) {
    set_pixel(cx + x, cy + y, color, clip_rect, buffer);
    set_pixel(cx - x, cy + y, color, clip_rect, buffer);

    set_pixel(cx + x, cy - y, color, clip_rect, buffer);
    set_pixel(cx - x, cy - y, color, clip_rect, buffer);

    set_pixel(cx + y, cy + x, color, clip_rect, buffer);
    set_pixel(cx - y, cy + x, color, clip_rect, buffer);

    set_pixel(cx + y, cy - x, color, clip_rect, buffer);
    set_pixel(cx - y, cy - x, color, clip_rect, buffer);
}

auto inline fill_8_pixels(i32 x, i32 y, i32 cx, i32 cy, u32 color, Rectangle2i clip_rect, FrameBuffer* buffer) {
    set_8_pixels(x, y, cx, cy, color, clip_rect, buffer);
    {
        // Note, order matters here! Assuming that start < end
        i32 xs = cx - x + 1;
        i32 ys = cy + y;
        i32 xe = cx + x - 1;
        i32 ye = cy + y;
        i32 dx = xe - xs;
        i32 dy = ye - ys;
        render_line_bresenham_x(xs, ys, xe, ye, dx, dy, color, clip_rect, buffer);
    }

    {
        i32 xs = cx - x + 1;
        i32 ys = cy - y;
        i32 xe = cx + x - 1;
        i32 ye = cy - y;
        i32 dx = xe - xs;
        i32 dy = ye - ys;
        render_line_bresenham_x(xs, ys, xe, ye, dx, dy, color, clip_rect, buffer);
    }

    {
        i32 xs = cx - y + 1;
        i32 ys = cy + x;
        i32 xe = cx + y - 1;
        i32 ye = cy + x;
        i32 dx = xe - xs;
        i32 dy = ye - ys;
        render_line_bresenham_x(xs, ys, xe, ye, dx, dy, color, clip_rect, buffer);
    }

    {
        i32 xs = cx - y + 1;
        i32 ys = cy - x;
        i32 xe = cx + y - 1;
        i32 ye = cy - x;
        i32 dx = xe - xs;
        i32 dy = ye - ys;
        render_line_bresenham_x(xs, ys, xe, ye, dx, dy, color, clip_rect, buffer);
    }
}

auto inline render_circle_bresenham(vec2 P, f32 radius, vec4 color, Rectangle2i clip_rect, FrameBuffer* buffer) -> void {
    i32 x = 0;
    i32 y = round_f32_to_i32(radius);
    i32 e = -y;

    i32 px = round_f32_to_i32(P.x);
    i32 py = round_f32_to_i32(P.y);
    u32 c = pack_color_8x4(color);
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

auto inline render_filled_circle_bresenham(vec2 P, f32 radius, vec4 color, Rectangle2i clip_rect, FrameBuffer* buffer) -> void {
    i32 x = 0;
    i32 y = round_f32_to_i32(radius);
    i32 e = -y;

    i32 px = round_f32_to_i32(P.x);
    i32 py = round_f32_to_i32(P.y);
    u32 c = pack_color_8x4(color);
    while (x <= y) {
        fill_8_pixels(x, y, px, py, c, clip_rect, buffer);
        e = e + (2 * x) + 1;
        x++;
        if (e >= 0) {
            e = e - (2 * y) + 2;
            y--;
        }
    }
}

/// @brief: Moving from i0 to i1, this function will return all the "d"s for every discrete value of i between i0 and i1
auto inline interpolate_i32(i32 i0, i32 d0, i32 i1, i32 d1, MemoryArena& arena) -> Array<i32> {
    Assert(i0 <= i1);
    if (i0 == i1) {
        auto result = Array<i32>::create(1, arena);
        result[0] = d0;
        return result;
    }
    i32 length = (i1 - i0) + 1;
    auto result = Array<i32>::create(length, arena);

    f32 a = ((f32)(d1 - d0)) / (i1 - i0); // dd/di
    f32 d = (f32)d0;
    for (i32 i = 0; i < length; i++) {
        result[i] = round_f32_to_i32(d);
        d += a;
    }
    return result;
}

/// @brief: Moving from i0 to i1, this function will return all the "d"s for every discrete value of i between i0 and i1
auto inline interpolate_f32(i32 i0, f32 d0, i32 i1, f32 d1, MemoryArena& arena) -> Array<f32> {
    // Assert(i0 <= i1);
    if (i0 == i1) {
        auto result = Array<f32>::create(1, arena);
        result[0] = d0;
        return result;
    }
    i32 length = abs(i1 - i0) + 1;
    auto result = Array<f32>::create(length, arena);

    f32 a = (d1 - d0) / (i1 - i0); // dd/di
    f32 d = d0;
    for (i32 i = 0; i < length; i++) {
        result[i] = d;
        d += a;
    }
    return result;
}

auto inline render_line_gambetta_internal(
    vec2 P0, vec2 P1, u32 color, Rectangle2i clip_rect, FrameBuffer* buffer, MemoryArena& arena) -> void {
    if (abs(P1.x - P0.x) > abs(P1.y - P0.y)) {
        if (P0.x > P1.x) {
            swap_vec2(P0, P1);
        }
        i32 x0 = round_f32_to_i32(P0.x);
        i32 y0 = round_f32_to_i32(P0.y);
        i32 x1 = round_f32_to_i32(P1.x);
        i32 y1 = round_f32_to_i32(P1.y);
        Array<i32> ys = interpolate_i32(x0, y0, x1, y1, arena);
        for (i32 x = x0; x <= x1; x++) {
            set_pixel(x, ys[x - x0], color, clip_rect, buffer);
        }
    }
    else {
        if (P0.y > P1.y) {
            swap_vec2(P0, P1);
        }
        i32 x0 = round_f32_to_i32(P0.x);
        i32 y0 = round_f32_to_i32(P0.y);
        i32 x1 = round_f32_to_i32(P1.x);
        i32 y1 = round_f32_to_i32(P1.y);
        Array<i32> xs = interpolate_i32(y0, x0, y1, x1, arena);
        for (i32 y = y0; y <= y1; y++) {
            set_pixel(xs[y - y0], y, color, clip_rect, buffer);
        }
    }
}

auto inline render_line_gambetta_intensity_internal(vec2 P0, vec2 P1, f32 h0, f32 h1, vec4 color_l1,
    Rectangle2i clip_rect, FrameBuffer* buffer, MemoryArena& arena) -> void {
    if (abs(P1.x - P0.x) > abs(P1.y - P0.y)) {
        if (P0.x > P1.x) {
            swap_vec2(P0, P1);
            swap_f32(h0, h1);
        }
        i32 x0 = round_f32_to_i32(P0.x);
        i32 y0 = round_f32_to_i32(P0.y);
        i32 x1 = round_f32_to_i32(P1.x);
        i32 y1 = round_f32_to_i32(P1.y);
        Array<i32> ys = interpolate_i32(x0, y0, x1, y1, arena);
        Array<f32> hs = interpolate_f32(x0, h0, x1, h1, arena);
        for (i32 x = x0; x <= x1; x++) {
            i32 index = x - x0;
            vec4 h_corrected_color = hs[index] * color_l1;
            set_pixel(x, ys[x - x0], linear1_to_packed8x4_srgb255(h_corrected_color), clip_rect, buffer);
        }
    }
    else {
        if (P0.y > P1.y) {
            swap_vec2(P0, P1);
            swap_f32(h0, h1);
        }
        i32 x0 = round_f32_to_i32(P0.x);
        i32 y0 = round_f32_to_i32(P0.y);
        i32 x1 = round_f32_to_i32(P1.x);
        i32 y1 = round_f32_to_i32(P1.y);
        Array<i32> xs = interpolate_i32(y0, x0, y1, x1, arena);
        Array<f32> hs = interpolate_f32(y0, h0, y1, h1, arena);
        for (i32 y = y0; y <= y1; y++) {
            i32 index = y - y0;
            vec4 h_corrected_color = hs[index] * color_l1;
            set_pixel(xs[y - y0], y, linear1_to_packed8x4_srgb255(h_corrected_color), clip_rect, buffer);
        }
    }
}

auto inline render_line_gambetta(vec2 P0, vec2 P1, vec4 color, Rectangle2i clip_rect, FrameBuffer* buffer, MemoryArena& arena)
    -> void {
    u32 c = pack_color_8x4(color);
    render_line_gambetta_internal(P0, P1, c, clip_rect, buffer, arena);
}

auto inline render_triangle_writeframe_gambetta(
    vec2 P0, vec2 P1, vec2 P2, vec4 color, Rectangle2i clip_rect, FrameBuffer* buffer, MemoryArena& arena) -> void {
    render_line_gambetta(P0, P1, color, clip_rect, buffer, arena);
    render_line_gambetta(P1, P2, color, clip_rect, buffer, arena);
    render_line_gambetta(P2, P0, color, clip_rect, buffer, arena);
}

auto inline render_filled_triangle_gambetta(
    vec2 P0, vec2 P1, vec2 P2, vec4 color, Rectangle2i clip_rect, FrameBuffer* buffer, MemoryArena& arena) -> void {

    if (P1.y < P0.y) {
        swap_vec2(P0, P1);
    }
    if (P2.y < P0.y) {
        swap_vec2(P0, P2);
    }
    if (P2.y < P1.y) {
        swap_vec2(P1, P2);
    }

    u32 packed_color = pack_color_8x4(color);

    i32 x0 = round_f32_to_i32(P0.x);
    i32 y0 = round_f32_to_i32(P0.y);
    i32 x1 = round_f32_to_i32(P1.x);
    i32 y1 = round_f32_to_i32(P1.y);
    i32 x2 = round_f32_to_i32(P2.x);
    i32 y2 = round_f32_to_i32(P2.y);

    Array<i32> x01 = interpolate_i32(y0, x0, y1, x1, arena);
    Array<i32> x12 = interpolate_i32(y1, x1, y2, x2, arena);

    x01 = span(x01, 0, x01.count() - 1);
    Array<i32> x012 = concat(x01, x12, arena);

    Array<i32> x02 = interpolate_i32(y0, x0, y2, x2, arena);

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

    for (i32 y = y0; y <= y2; y++) {
        for (i32 x = x_left[y - y0]; x <= x_right[y - y0]; x++) {
            set_pixel(x, y, packed_color, clip_rect, buffer);
        }
    }

    // Hmm, gotta figure out what to do about this
    render_line_gambetta_internal(P0, P1, packed_color, clip_rect, buffer, arena);
    render_line_gambetta_internal(P1, P2, packed_color, clip_rect, buffer, arena);
    render_line_gambetta_internal(P2, P0, packed_color, clip_rect, buffer, arena);
}

auto inline render_shaded_triangle_gambetta(vec2 P0, vec2 P1, vec2 P2, f32 h0, f32 h1, f32 h2, vec4 color,
    Rectangle2i clip_rect, FrameBuffer* buffer, MemoryArena& arena) -> void {

    if (P1.y < P0.y) {
        swap_vec2(P0, P1);
        swap_f32(h0, h1);
    }
    if (P2.y < P0.y) {
        swap_vec2(P0, P2);
        swap_f32(h0, h2);
    }
    if (P2.y < P1.y) {
        swap_vec2(P1, P2);
        swap_f32(h1, h2);
    }

    vec4 color_l1 = srgb_to_linear1(color);

    i32 x0 = round_f32_to_i32(P0.x);
    i32 y0 = round_f32_to_i32(P0.y);
    i32 x1 = round_f32_to_i32(P1.x);
    i32 y1 = round_f32_to_i32(P1.y);
    i32 x2 = round_f32_to_i32(P2.x);
    i32 y2 = round_f32_to_i32(P2.y);

    Array<i32> x01 = interpolate_i32(y0, x0, y1, x1, arena);
    Array<f32> h01 = interpolate_f32(y0, h0, y1, h1, arena);

    Array<i32> x12 = interpolate_i32(y1, x1, y2, x2, arena);
    Array<f32> h12 = interpolate_f32(y1, h1, y2, h2, arena);

    x01 = span(x01, 0, x01.count() - 1);
    h01 = span(h01, 0, h01.count() - 1);
    Array<i32> x012 = concat(x01, x12, arena);
    Array<f32> h012 = concat(h01, h12, arena);

    Array<i32> x02 = interpolate_i32(y0, x0, y2, x2, arena);
    Array<f32> h02 = interpolate_f32(y0, h0, y2, h2, arena);

    i32 m = (i32)x012.count() / 2;
    Array<i32> x_left;
    Array<i32> x_right;

    Array<f32> h_left;
    Array<f32> h_right;
    if (x02[m] <= x012[m]) {
        x_left = x02;
        h_left = h02;

        x_right = x012;
        h_right = h012;
    }
    else {
        x_right = x02;
        h_right = h02;

        x_left = x012;
        h_left = h012;
    }

    for (i32 y = y0; y <= y2; y++) {
        const i32 y_idx = y - y0;
        const i32 x_l = x_left[y_idx];
        const i32 x_r = x_right[y_idx];

        Array<f32> h_l_to_r = interpolate_f32(x_l, h_left[y_idx], x_r, h_right[y_idx], arena);
        for (i32 x = x_l; x <= x_r; x++) {
            vec4 h_corrected_color = h_l_to_r[x - x_l] * color_l1;
            set_pixel(x, y, linear1_to_packed8x4_srgb255(h_corrected_color), clip_rect, buffer);
        }
    }

    render_line_gambetta_intensity_internal(P0, P1, h0, h1, color_l1, clip_rect, buffer, arena);
    render_line_gambetta_intensity_internal(P1, P2, h1, h2, color_l1, clip_rect, buffer, arena);
    render_line_gambetta_intensity_internal(P2, P0, h2, h0, color_l1, clip_rect, buffer, arena);
}

auto inline viewport_to_canvas(f32 x, f32 y) -> vec2 {
    const f32 new_x = (x * 0.5f + 0.5f) * (INTERNAL_WIDTH - 1);
    const f32 new_y = (y * 0.5f + 0.5f) * (INTERNAL_HEIGHT - 1);
    return { new_x, new_y };
}

auto inline project_vertex(vec4 v) -> vec2 {
    return viewport_to_canvas(v.x / v.w, v.y / v.w);
}

auto inline is_inside_view(vec4 p) -> bool {
    return in_range(-p.w, p.x, p.w) && in_range(-p.w, p.y, p.w) && in_range(-p.w, p.z, p.w);
}

auto inline clip_triangle_against_plane(vec4 v1, vec4 v2, vec4 v3, List<vec4>& clipped_triangles, i32 plane, i32 side) {
    const i32 Edge_Count = 3;
    f32 ds[3] = {
        v1.x + (side * v1.w),
        v2.x + (side * v2.w),
        v3.x + (side * v3.w),
    };
    vec4 v[3] = { v1, v2, v3 };

    i32 out_count = 0;
    i32 in_count = 0;
    i32 out_idx[Edge_Count] = { -1, -1, -1 };
    i32 in_idx[Edge_Count] = { -1, -1, -1 };
    for (auto i = 0; i < 3; i++) {
        if ((side * ds[i]) < 0) {
            out_idx[out_count++] = i;
        }
        else {
            in_idx[in_count++] = i;
        }
    }

    if (out_count == 0) {
        clipped_triangles.push(v1);
        clipped_triangles.push(v2);
        clipped_triangles.push(v3);
    }
    else if (out_count == 1) {
        i32 a_idx = out_idx[0];
        i32 b_idx = (a_idx + 1) % Edge_Count;
        i32 c_idx = (a_idx + 2) % Edge_Count;
        vec4 a = v[a_idx];
        vec4 b = v[b_idx];
        vec4 c = v[c_idx];

        // ab
        vec4 ab = {};
        {
            f32 d_a = a.x + (side * a.w);
            f32 d_b = b.x + (side * b.w);
            f32 t = -(d_a) / (d_b - d_a);
            ab = lerp(a, t, b);
        }
        vec4 ac = {};
        {
            f32 d_a = a.x + (side * a.w);
            f32 d_c = c.x + (side * c.w);
            f32 t = -(d_a) / (d_c - d_a);
            ac = lerp(a, t, c);
        }

        // ab -> b -> c;
        clipped_triangles.push(ab);
        clipped_triangles.push(b);
        clipped_triangles.push(c);
        // ac -> ab -> c;
        clipped_triangles.push(ac);
        clipped_triangles.push(ab);
        clipped_triangles.push(c);
    }
    else if (out_count == 2) {
        i32 a_idx = in_idx[0];
        i32 b_idx = (a_idx + 1) % Edge_Count;
        i32 c_idx = (a_idx + 2) % Edge_Count;

        vec4 a = v[a_idx];
        vec4 b = v[b_idx];
        vec4 c = v[c_idx];

        clipped_triangles.push(a);
        // b'
        {
            f32 d_a = a.x + (side * a.w);
            f32 d_b = b.x + (side * b.w);
            f32 t = -(d_a) / (d_b - d_a);
            vec4 b_new = lerp(a, t, b);
            clipped_triangles.push(b_new);
        }
        {
            f32 d_a = a.x + (side * a.w);
            f32 d_c = c.x + (side * c.w);
            f32 t = -(d_a) / (d_c - d_a);
            vec4 c_new = lerp(a, t, c);
            clipped_triangles.push(c_new);
        }

        // c'
    }
    else if (out_count == 3) {
    }
}

// For each plane
//   For each triangle
//     Add vertices and indices

enum PlaneIdx : i8 {
    PlaneIdx_X = 0,
    PlaneIdx_Y = 1,
    PlaneIdx_Z = 2,
};

enum PlaneDirection : i8 {
    PlaneDirection_Forward = 1,
    PlaneDirection_Backwards = -1,
};

auto inline clip_triangle_against_plane2(                 //
    Array<vec4> in_vertices, Array<ivec3> in_indices,     //
    List<vec4>& out_vertices, List<ivec3>& out_indices,   //
    PlaneIdx plane_index, PlaneDirection plane_direction, //
    MemoryArena& arena) {
    Assert(plane_index >= 0 && plane_index < 4);
    Assert(plane_direction == 1 || plane_direction == -1);

    Array<i32> index_map = Array<i32>::create(in_vertices.count(), arena);
    fill_inc(index_map);

    f32 dir = (f32)plane_direction;
    for (auto triangle_indices : in_indices) {
        vec4 v0 = in_vertices[index_map[triangle_indices.x]];
        vec4 v1 = in_vertices[index_map[triangle_indices.y]];
        vec4 v2 = in_vertices[index_map[triangle_indices.z]];

        const i32 Edge_Count = 3;
        f32 ds[3] = {
            v0.v[plane_index] + (dir * v0.w),
            v1.v[plane_index] + (dir * v1.w),
            v2.v[plane_index] + (dir * v2.w),
        };
        vec4 v[3] = { v0, v1, v2 };

        i32 out_count = 0;
        i32 in_count = 0;
        i32 out_idx[Edge_Count] = { -1, -1, -1 };
        i32 in_idx[Edge_Count] = { -1, -1, -1 };
        for (auto i = 0; i < 3; i++) {
            if ((dir * ds[i]) < 0) {
                out_idx[out_count++] = i;
            }
            else {
                in_idx[in_count++] = i;
            }
        }

        if (out_count == 0) {
            // TODO: We create more vertexes than neccessary here.
            i32 start_idx = out_vertices.count();
            ivec3 indices = { start_idx, start_idx + 1, start_idx + 2 };
            out_indices.push(indices);
            out_vertices.push(v0);
            out_vertices.push(v1);
            out_vertices.push(v2);
        }
        else if (out_count == 1) {
            i32 a_idx = out_idx[0];
            i32 b_idx = (a_idx + 1) % Edge_Count;
            i32 c_idx = (a_idx + 2) % Edge_Count;
            vec4 a = v[a_idx];
            vec4 b = v[b_idx];
            vec4 c = v[c_idx];

            // ab
            vec4 ab = {};
            {
                f32 d_a = a.v[plane_index] + (dir * a.w);
                f32 d_b = b.v[plane_index] + (dir * b.w);
                f32 t = -(d_a) / (d_b - d_a);
                ab = lerp(a, t, b);
            }
            vec4 ac = {};
            {
                f32 d_a = a.v[plane_index] + (dir * a.w);
                f32 d_c = c.v[plane_index] + (dir * c.w);
                f32 t = -(d_a) / (d_c - d_a);
                ac = lerp(a, t, c);
            }

            i32 start_idx = out_vertices.count();
            out_vertices.push(ab);
            out_vertices.push(b);
            out_vertices.push(c);
            out_vertices.push(ac);

            ivec3 ab_b_c_indices = {
                start_idx,     //
                start_idx + 1, //
                start_idx + 2  //
            };
            ivec3 ac_ab_c_indices = {
                start_idx + 3, //
                start_idx + 0, //
                start_idx + 2  //
            };
            // ab -> b -> c;
            out_indices.push(ab_b_c_indices);
            // // ac -> ab -> c;
            out_indices.push(ac_ab_c_indices);
        }
        else if (out_count == 2) {
            i32 a_idx = in_idx[0];
            i32 b_idx = (a_idx + 1) % Edge_Count;
            i32 c_idx = (a_idx + 2) % Edge_Count;

            vec4 a = v[a_idx];
            vec4 b = v[b_idx];
            vec4 c = v[c_idx];

            vec4 b_new;
            // b'
            {
                f32 d_a = a.v[plane_index] + (dir * a.w);
                f32 d_b = b.v[plane_index] + (dir * b.w);
                f32 t = -(d_a) / (d_b - d_a);
                b_new = lerp(a, t, b);
            }
            vec4 c_new;
            {
                f32 d_a = a.v[plane_index] + (dir * a.w);
                f32 d_c = c.v[plane_index] + (dir * c.w);
                f32 t = -(d_a) / (d_c - d_a);
                c_new = lerp(a, t, c);
            }

            i32 start_idx = out_vertices.count();
            out_vertices.push(a);
            out_vertices.push(b_new);
            out_vertices.push(c_new);

            ivec3 a_bNew_cNew_indices = {
                start_idx,     //
                start_idx + 1, //
                start_idx + 2  //
            };
            out_indices.push(a_bNew_cNew_indices);
        }
        else if (out_count == 3) {
        }
    }
}

auto inline clip_triangles_against_all_planes(          //
    Array<vec4> in_vertices, Array<ivec3> in_indices,   //
    List<vec4>& out_vertices, List<ivec3>& out_indices, //
    MemoryArena& arena) {

    auto temp_vertices = List<vec4>::create(out_vertices.max_count(), arena);
    auto temp_indices = List<ivec3>::create(out_indices.max_count(), arena);

    {
        clip_triangle_against_plane2(           //
            in_vertices, in_indices,            //
            temp_vertices, temp_indices,        //
            PlaneIdx_X, PlaneDirection_Forward, //
            arena                               //
        );
        clip_triangle_against_plane2(                          //
            temp_vertices.to_array(), temp_indices.to_array(), //
            out_vertices, out_indices,                         //
            PlaneIdx_X, PlaneDirection_Backwards,              //
            arena                                              //
        );
    }

    {
        clip_triangle_against_plane2(                        //
            out_vertices.to_array(), out_indices.to_array(), //
            temp_vertices, temp_indices,                     //
            PlaneIdx_Y, PlaneDirection_Forward,              //
            arena                                            //
        );
        clip_triangle_against_plane2(                          //
            temp_vertices.to_array(), temp_indices.to_array(), //
            out_vertices, out_indices,                         //
            PlaneIdx_Y, PlaneDirection_Backwards,              //
            arena                                              //
        );
    }

    temp_vertices.clear();
    temp_indices.clear();
}

auto inline clip_triangle(Array<vec4> vertices, ivec3 indices, List<vec4>& clipped_triangles) {
    // Left plane
    const i32 Edge_Count = 3;
    i32 idx[Edge_Count] = {
        indices.x,
        indices.y,
        indices.z,
    };

    const vec4& v1 = vertices[idx[0]];
    const vec4& v2 = vertices[idx[1]];
    const vec4& v3 = vertices[idx[2]];

    // X
    clip_triangle_against_plane(v1, v2, v3, clipped_triangles, 0, 1);
    clip_triangle_against_plane(v1, v2, v3, clipped_triangles, 0, -1);

    // Y
    // clip_triangle_against_plane(v1, v2, v3, clipped_triangles, 1, 1);
    // clip_triangle_against_plane(v1, v2, v3, clipped_triangles, 1, -1);
    //
    // // Z
    // clip_triangle_against_plane(v1, v2, v3, clipped_triangles, 2, 1);
    // clip_triangle_against_plane(v1, v2, v3, clipped_triangles, 2, -1);

    //  Left pane
    //  {
    //      f32 ds[3] = {
    //          v1.x + v1.w,
    //          v2.x + v2.w,
    //          v3.x + v3.w,
    //      };
    //
    //      i32 out_count = 0;
    //      i32 in_count = 0;
    //      i32 out_idx[Edge_Count] = { -1, -1, -1 };
    //      i32 in_idx[Edge_Count] = { -1, -1, -1 };
    //      for (auto i = 0; i < 3; i++) {
    //          if (ds[i] < 0) {
    //              out_idx[out_count++] = i;
    //          }
    //          else {
    //              in_idx[in_count++] = i;
    //          }
    //      }
    //
    //      if (out_count == 0) {
    //          clipped_triangles.push(v1);
    //          clipped_triangles.push(v2);
    //          clipped_triangles.push(v3);
    //      }
    //      else if (out_count == 1) {
    //          i32 a_idx = out_idx[0];
    //          i32 b_idx = (a_idx + 1) % Edge_Count;
    //          i32 c_idx = (a_idx + 2) % Edge_Count;
    //          vec4 a = vertices[idx[a_idx]];
    //          vec4 b = vertices[idx[b_idx]];
    //          vec4 c = vertices[idx[c_idx]];
    //
    //          // ab
    //          vec4 ab = {};
    //          {
    //              f32 d_a = a.x + a.w;
    //              f32 d_b = b.x + b.w;
    //              f32 t = -(d_a) / (d_b - d_a);
    //              ab = lerp(a, t, b);
    //          }
    //          vec4 ac = {};
    //          {
    //              f32 d_a = a.x + a.w;
    //              f32 d_c = c.x + c.w;
    //              f32 t = -(d_a) / (d_c - d_a);
    //              ac = lerp(a, t, c);
    //          }
    //
    //          // ab -> b -> c;
    //          clipped_triangles.push(ab);
    //          clipped_triangles.push(b);
    //          clipped_triangles.push(c);
    //          // ac -> ab -> c;
    //          clipped_triangles.push(ac);
    //          clipped_triangles.push(ab);
    //          clipped_triangles.push(c);
    //      }
    //      else if (out_count == 2) {
    //          i32 a_idx = in_idx[0];
    //          i32 b_idx = (a_idx + 1) % Edge_Count;
    //          i32 c_idx = (a_idx + 2) % Edge_Count;
    //
    //          vec4 a = vertices[idx[a_idx]];
    //          vec4 b = vertices[idx[b_idx]];
    //          vec4 c = vertices[idx[c_idx]];
    //
    //          clipped_triangles.push(a);
    //          // b'
    //          {
    //              f32 d_a = a.x + a.w;
    //              f32 d_b = b.x + b.w;
    //              f32 t = -(d_a) / (d_b - d_a);
    //              vec4 b_new = lerp(a, t, b);
    //              clipped_triangles.push(b_new);
    //          }
    //          {
    //              f32 d_a = a.x + a.w;
    //              f32 d_c = c.x + c.w;
    //              f32 t = -(d_a) / (d_c - d_a);
    //              vec4 c_new = lerp(a, t, c);
    //              clipped_triangles.push(c_new);
    //          }
    //
    //          // c'
    //      }
    //      else if (out_count == 3) {
    //      }
    //  }
    //  Right plane
    // {
    //     f32 ds[3] = {
    //         vertices[idx[0]].v[0] + (-1 * vertices[idx[0]].w),
    //         vertices[idx[1]].v[0] + (-1 * vertices[idx[1]].w),
    //         vertices[idx[2]].v[0] + (-1 * vertices[idx[2]].w),
    //     };
    //
    //     i32 out_count = 0;
    //     for (auto i = 0; i < 3; i++) {
    //         if ((-1 * ds[i]) < 0) {
    //             out_count++;
    //         }
    //     }
    //
    //     if (out_count > 0) {
    //         printf("OUT: %d\n", out_count);
    //     }
    // }
}

auto inline render_polygon_gambetta(                                //
    Array<vec4> vertices, Array<ivec3> indices, Array<vec4> colors, //
    Array<Transform> instances,                                     //
    const mat4& world_to_view,                                      //
    const mat4& view_to_clip,                                       //
    Rectangle2i clip_rect, FrameBuffer* buffer, MemoryArena& arena  //
    ) -> void {
    Assert(indices.count() == colors.count());

    for (const auto& instance : instances) {
        auto clip_space_vertices = Array<vec4>::create(vertices.count(), arena);

        mat4 M_to_W = instance.to_mat4();
        mat4 M_to_C = M_to_W * world_to_view;
        mat4 M_to_Clip = M_to_C * view_to_clip;
        for (u32 i = 0; i < vertices.count(); i++) {
            clip_space_vertices[i] = vertices[i] * M_to_Clip;
        }

        auto clipped_vertices = List<vec4>::create(vertices.count() * 10, arena);
        auto clipped_indices = List<ivec3>::create(indices.count() * 10, arena);
        clip_triangles_against_all_planes(clip_space_vertices, indices, clipped_vertices, clipped_indices, arena);

        auto projected_vertices = Array<vec2>::create(clipped_vertices.count(), arena);
        for (i32 i = 0; i < clipped_vertices.count(); i++) {
            projected_vertices[i] = project_vertex(clipped_vertices[i]);
        }

        // for (u32 i = 0; i < vertices.count(); i++) {
        //     projected_vertices[i] = project_vertex(clip_space_vertices[i]);
        // }

        // for (u32 i = 0; i < triangles.count(); i++) {
        //     const auto& t = triangles[i];
        //     render_triangle_writeframe_gambetta( //
        //         projected_vertices[t.x],         //
        //         projected_vertices[t.y],         //
        //         projected_vertices[t.z],         //
        //         colors[i], clip_rect, buffer, arena);
        // }

        for (i32 i = 0; i < clipped_indices.count(); i++) {
            ivec3 index = clipped_indices[i];
            vec2 a = projected_vertices[index.x];
            vec2 b = projected_vertices[index.y];
            vec2 c = projected_vertices[index.z];
            render_triangle_writeframe_gambetta( //
                a, b, c, colors[i % colors.count()], clip_rect, buffer, arena);
        }
    }
}

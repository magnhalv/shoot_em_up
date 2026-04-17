#include "doctest.h"

#include <renderers/cpu_render_algorithms.hpp>
#include <cstring>

static FrameBuffer make_buffer(i32 width, i32 height) {
    FrameBuffer buffer;
    buffer.bytes_per_pixel = 4;
    buffer.height = height;
    buffer.width = width;
    buffer.pitch = buffer.width * buffer.bytes_per_pixel;
    buffer.memory_size = buffer.width * buffer.height * buffer.bytes_per_pixel;
    buffer.memory = calloc(1, buffer.memory_size);
    return buffer;
}

TEST_CASE("render_line_bresenham x-major positive slope") {
    FrameBuffer buffer = make_buffer(5, 3);
    Rectangle2i rect = {0, buffer.width, 0, buffer.height};
    vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
    render_line_bresenham(vec2(0, 0), vec2(4, 2), color, rect, &buffer);

    REQUIRE_EQ(get_color(&buffer, 0, 0), 0xFFFFFFFF);
    REQUIRE_EQ(get_color(&buffer, 1, 1), 0xFFFFFFFF);
    REQUIRE_EQ(get_color(&buffer, 2, 1), 0xFFFFFFFF);
    REQUIRE_EQ(get_color(&buffer, 3, 2), 0xFFFFFFFF);
    REQUIRE_EQ(get_color(&buffer, 4, 2), 0xFFFFFFFF);

    free(buffer.memory);
}

TEST_CASE("render_line_bresenham horizontal line") {
    FrameBuffer buffer = make_buffer(6, 3);
    Rectangle2i rect = {0, buffer.width, 0, buffer.height};
    render_line_bresenham(vec2(0, 1), vec2(5, 1), vec4(1, 1, 1, 1), rect, &buffer);

    for (i32 x = 0; x <= 5; x++)
        REQUIRE_EQ(get_color(&buffer, x, 1), 0xFFFFFFFF);

    free(buffer.memory);
}

TEST_CASE("render_line_bresenham vertical line") {
    FrameBuffer buffer = make_buffer(3, 5);
    Rectangle2i rect = {0, buffer.width, 0, buffer.height};
    render_line_bresenham(vec2(1, 0), vec2(1, 4), vec4(1, 1, 1, 1), rect, &buffer);

    for (i32 y = 0; y <= 4; y++)
        REQUIRE_EQ(get_color(&buffer, 1, y), 0xFFFFFFFF);

    free(buffer.memory);
}

TEST_CASE("render_line_bresenham y-major positive slope") {
    // (0,0)->(2,4): dx=2, dy=4 => y-major
    // render_line_bresenham_y(0,0,2,4,2,4): e=-2, x_inc=1, dx=2
    // y=0: set(0,0); e+=2->0; x=1, e-=4->-4
    // y=1: set(1,1); e+=2->-2; no x
    // y=2: set(1,2); e+=2->0; x=2, e-=4->-4
    // y=3: set(2,3); e+=2->-2; no x
    // y=4: set(2,4)
    FrameBuffer buffer = make_buffer(3, 5);
    Rectangle2i rect = {0, buffer.width, 0, buffer.height};
    render_line_bresenham(vec2(0, 0), vec2(2, 4), vec4(1, 1, 1, 1), rect, &buffer);

    REQUIRE_EQ(get_color(&buffer, 0, 0), 0xFFFFFFFF);
    REQUIRE_EQ(get_color(&buffer, 1, 1), 0xFFFFFFFF);
    REQUIRE_EQ(get_color(&buffer, 1, 2), 0xFFFFFFFF);
    REQUIRE_EQ(get_color(&buffer, 2, 3), 0xFFFFFFFF);
    REQUIRE_EQ(get_color(&buffer, 2, 4), 0xFFFFFFFF);

    free(buffer.memory);
}

TEST_CASE("render_line_bresenham reversed endpoints draw same pixels") {
    i32 w = 5, h = 3;
    FrameBuffer buf1 = make_buffer(w, h);
    FrameBuffer buf2 = make_buffer(w, h);
    Rectangle2i rect = {0, w, 0, h};
    vec4 color = vec4(1, 1, 1, 1);

    render_line_bresenham(vec2(0, 0), vec2(4, 2), color, rect, &buf1);
    render_line_bresenham(vec2(4, 2), vec2(0, 0), color, rect, &buf2);

    REQUIRE_EQ(memcmp(buf1.memory, buf2.memory, buf1.memory_size), 0);

    free(buf1.memory);
    free(buf2.memory);
}

TEST_CASE("render_line_bresenham clipping excludes out-of-bounds pixels") {
    // Line (0,0)->(4,4), clip to x:[1,4) y:[1,4) — corners (0,0) and (4,4) must not be written
    FrameBuffer buffer = make_buffer(5, 5);
    Rectangle2i rect = {1, 4, 1, 4};
    render_line_bresenham(vec2(0, 0), vec2(4, 4), vec4(1, 1, 1, 1), rect, &buffer);

    REQUIRE_EQ(get_color(&buffer, 0, 0), 0x00000000);
    REQUIRE_EQ(get_color(&buffer, 4, 4), 0x00000000);
    REQUIRE_EQ(get_color(&buffer, 1, 1), 0xFFFFFFFF);
    REQUIRE_EQ(get_color(&buffer, 2, 2), 0xFFFFFFFF);
    REQUIRE_EQ(get_color(&buffer, 3, 3), 0xFFFFFFFF);

    free(buffer.memory);
}

TEST_CASE("render_line_bresenham single point") {
    FrameBuffer buffer = make_buffer(3, 3);
    Rectangle2i rect = {0, buffer.width, 0, buffer.height};
    render_line_bresenham(vec2(1, 1), vec2(1, 1), vec4(1, 1, 1, 1), rect, &buffer);

    REQUIRE_EQ(get_color(&buffer, 1, 1), 0xFFFFFFFF);

    free(buffer.memory);
}

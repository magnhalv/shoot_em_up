#include "doctest.h"

#include <renderers/cpu_render_algorithms.hpp>

TEST_CASE("render") {
    FrameBuffer buffer;
    buffer.bytes_per_pixel = 4;
    buffer.height = 3;
    buffer.width = 5;
    buffer.pitch = buffer.width * buffer.bytes_per_pixel;
    buffer.memory_size = buffer.width * buffer.height * buffer.bytes_per_pixel;
    buffer.memory = malloc(buffer.memory_size);

    vec2 start = vec2(0, 0);
    vec2 end = vec2(4, 2);
    Rectangle2i rect = {};
    vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
    render_line_bresenham(start, end, color, rect, &buffer);

    REQUIRE_EQ(get_color(&buffer, 0, 0), 0xFFFFFFFF);
    REQUIRE_EQ(get_color(&buffer, 1, 1), 0xFFFFFFFF);
    REQUIRE_EQ(get_color(&buffer, 2, 1), 0xFFFFFFFF);
    REQUIRE_EQ(get_color(&buffer, 3, 2), 0xFFFFFFFF);
    REQUIRE_EQ(get_color(&buffer, 4, 2), 0xFFFFFFFF);

    free(buffer.memory);
}

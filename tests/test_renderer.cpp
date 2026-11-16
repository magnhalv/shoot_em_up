#include "core/color.hpp"
#include "doctest.h"
#include "renderers/cpu_render_algorithms.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <core/string8.hpp>
#include <renderers/renderer.cpp>

struct RendererArenaFixture {
    static constexpr size_t ArenaSize = KiloBytes(512);

    RendererArenaFixture() {
        arena.init(malloc(ArenaSize), ArenaSize);
    }

    ~RendererArenaFixture() {
        free(arena.memory);
    }

    MemoryArena arena;
};

const u32 Color_A = 0xFFFF0000;        // red    - top-left
const u32 Color_B = 0xFF00FF00;        // green  - top-right
const u32 Color_C = 0xFF0000FF;        // blue   - bottom-left
const u32 Color_D = 0xFFFFFF00;        // yellow - bottom-right
const u32 Color_Sentinel = 0xFF808080; // gray, used to mark "dest pixels the function must not touch"
const u32 Color_Opaque = 0xFFFFFFFF;   // white, used for the checkerboard transparency test

// Renders a buffer as ASCII art so a test failure prints a human-readable picture
// of exactly what went in and what came out, instead of a wall of hex colors.
static char color_to_char(u32 color) {
    if (color == 0)
        return '0';
    if (color == Color_A)
        return 'A';
    if (color == Color_B)
        return 'B';
    if (color == Color_C)
        return 'C';
    if (color == Color_D)
        return 'D';
    if (color == Color_Sentinel)
        return 'S';
    if (color == Color_Opaque)
        return 'X';
    return '?';
}

static CString8 buffer_to_string(Framebuffer& buffer, MemoryArena& arena) {
    Size row_length = buffer.width + 1; // + '\n'
    Size count = row_length * buffer.height;
    char* data = allocate<char>(arena, count + 1); // +1 for the null terminator

    Size i = 0;
    for (i32 y = 0; y < buffer.height; y++) {
        for (i32 x = 0; x < buffer.width; x++) {
            data[i++] = color_to_char(*buffer.get_pixel(x, y));
        }
        data[i++] = '\n';
    }
    data[i] = '\0';
    return { data, count };
}

// Quadrant pattern: four solid blocks (A|B / C|D). Big blocks make scaling and
// offset placement trivial to verify by eye, since block *boundaries* are all
// that need to line up, unlike a per-pixel gradient.
static u32 quadrant_color(i32 x, i32 y, i32 half_w, i32 half_h) {
    if (x < half_w && y < half_h)
        return Color_A;
    if (x >= half_w && y < half_h)
        return Color_B;
    if (x < half_w && y >= half_h)
        return Color_C;
    return Color_D;
}

static void fill_quadrants(Framebuffer& buffer) {
    i32 half_w = buffer.width / 2;
    i32 half_h = buffer.height / 2;
    for (i32 y = 0; y < buffer.height; y++)
        for (i32 x = 0; x < buffer.width; x++)
            buffer.set_pixel(x, y, quadrant_color(x, y, half_w, half_h));
}

static void fill_solid(Framebuffer& buffer, u32 color) {
    for (i32 y = 0; y < buffer.height; y++)
        for (i32 x = 0; x < buffer.width; x++)
            buffer.set_pixel(x, y, color);
}

// Golden reference for a full-canvas nearest-neighbor scale of a src_w x src_h
// quadrant pattern into `dest` (offset 0,0, apply width/height == dest width/height).
static void fill_expected_scaled_quadrants(Framebuffer& dest, i32 src_w, i32 src_h) {
    f32 scale_x = (f32)dest.width / src_w;
    f32 scale_y = (f32)dest.height / src_h;
    i32 half_w = src_w / 2;
    i32 half_h = src_h / 2;
    for (i32 y = 0; y < dest.height; y++) {
        i32 src_y = (i32)(y / scale_y);
        for (i32 x = 0; x < dest.width; x++) {
            i32 src_x = (i32)(x / scale_x);
            dest.set_pixel(x, y, quadrant_color(src_x, src_y, half_w, half_h));
        }
    }
}

// Golden reference for placing an unscaled src_w x src_h quadrant pattern into
// `dest` at (offset_x, offset_y), clipped to dest bounds, with `background`
// everywhere else.
static void fill_expected_translated_quadrants(Framebuffer& dest, u32 background, i32 src_w, i32 src_h, i32 offset_x, i32 offset_y) {
    i32 half_w = src_w / 2;
    i32 half_h = src_h / 2;
    for (i32 y = 0; y < dest.height; y++) {
        for (i32 x = 0; x < dest.width; x++) {
            i32 src_x = x - offset_x;
            i32 src_y = y - offset_y;
            if (src_x >= 0 && src_x < src_w && src_y >= 0 && src_y < src_h) {
                dest.set_pixel(x, y, quadrant_color(src_x, src_y, half_w, half_h));
            }
            else {
                dest.set_pixel(x, y, background);
            }
        }
    }
}

// Checkerboard: every other pixel is transparent (0), forcing per-pixel
// select/masking behaviour rather than a single block copy/skip.
static void fill_checkerboard(Framebuffer& buffer, u32 opaque_color) {
    for (i32 y = 0; y < buffer.height; y++)
        for (i32 x = 0; x < buffer.width; x++)
            buffer.set_pixel(x, y, ((x + y) % 2 == 0) ? opaque_color : 0);
}

static auto fb_to_string(Framebuffer* buffer, MemoryArena* arena) -> String8 {
    String8 result = String8::create_empty(buffer->height * buffer->width, arena);
    char* data = result.data;
    for (i32 y = 0; y < buffer->height; y++) {
        for (i32 x = 0; x < buffer->width; x++) {
            *(data++) = color_to_char(*buffer->get_pixel(x, buffer->height - y - 1));
        }
    }
    return result;
}

static auto print_strings(CString8 a, CString8 b, i32 width) -> void {
    Assert(a.size == b.size);
    i64 height = a.size / width;
    for (i32 y = 0; y < height; y++) {
        for (i32 x = 0; x < width; x++) {
            printf("%c", a[y * width + x]);
        }
        printf("   ");
        for (i32 x = 0; x < width; x++) {
            printf("%c", b[y * width + x]);
        }
        printf("\n");
    }
}

TEST_CASE_FIXTURE(RendererArenaFixture, "render_filled_triangle") {
    i32 width = 3;
    i32 height = 2;
    Framebuffer fb = create_frame_buffer(arena, width, height);
    Rectangle2i clip = {
        .min_x = 0,
        .max_x = width,
        .min_y = 0,
        .max_y = height,
    };
    rasterize_triangle(                                                         //
        vec3(0.0f, 0.0f, 1.0f), vec3(1.5f, 2.0f, 1.0f), vec3(3.0f, 0.0f, 1.0f), //
        vec4(1.0f, 1.0f, 1.0f, 1.0f),                                           //
        TriangleAttribute{}, clip, fb, &arena);

    CString8 expected = "0X0"
                        "XXX";
    CString8 actual = fb_to_string(&fb, &arena);

    REQUIRE_EQ(actual.size, expected.size);
    bool equal = string8_equal(expected, actual);
    if (!equal) {
        print_strings(expected, actual, width);
    }
    REQUIRE(equal);
}

TEST_CASE_FIXTURE(RendererArenaFixture, "render_filled_triangle2") {
    i32 width = 6;
    i32 height = 2;
    Framebuffer fb = create_frame_buffer(arena, width, height);
    Rectangle2i clip = {
        .min_x = 0,
        .max_x = width,
        .min_y = 0,
        .max_y = height,
    };
    rasterize_triangle(                                                         //
        vec3(0.0f, 2.0f, 1.0f), vec3(3.0f, 0.0f, 1.0f), vec3(6.0f, 0.0f, 1.0f), //
        vec4(1.0f, 1.0f, 1.0f, 1.0f),                                           //
        TriangleAttribute{}, clip, fb, &arena);

    CString8 expected = "0X0000"
                        "00XXX0";
    CString8 actual = fb_to_string(&fb, &arena);

    REQUIRE_EQ(actual.size, expected.size);
    bool equal = string8_equal(expected, actual);
    if (!equal) {
        print_strings(expected, actual, width);
    }
    REQUIRE(equal);
}

TEST_CASE_FIXTURE(RendererArenaFixture, "render_filled_triangle3") {
    i32 width = 6;
    i32 height = 6;
    Framebuffer fb = create_frame_buffer(arena, width, height);
    Rectangle2i clip = {
        .min_x = 0,
        .max_x = width,
        .min_y = 0,
        .max_y = height,
    };
    rasterize_triangle_filled_gambetta(                                         //
        vec3(1.5f, 1.5f, 1.0f), vec3(4.5f, 3.5f, 1.0f), vec3(2.5f, 5.5f, 1.0f), //
        vec4(1.0f, 1.0f, 1.0f, 1.0f),                                           //
        clip, fb, &arena);

    CString8 expected = "00X000"
                        "00XX00"
                        "00XXX0"
                        "0XXX00"
                        "0X0000"
                        "000000";
    CString8 actual = fb_to_string(&fb, &arena);

    REQUIRE_EQ(actual.size, expected.size);
    bool equal = string8_equal(expected, actual);
    if (!equal) {
        print_strings(expected, actual, width);
    }
    REQUIRE(equal);
}

TEST_CASE_FIXTURE(RendererArenaFixture, "mipmap") {
    const u32 red = pack_color_8x4(255, 0, 0, 255);
    const u32 gre = pack_color_8x4(0, 255, 0, 255);
    const u32 blu = pack_color_8x4(0, 0, 255, 255);
    const u32 whi = pack_color_8x4(255, 255, 255, 255);
    const u32 silver_grey = pack_color_8x4(188, 188, 188, 255);
    {
        const i32 width = 4;
        const i32 height = 4;
        u32 data[width * height] = {
            red, red, gre, gre, //
            red, red, gre, gre, //
            blu, blu, whi, whi, //
            blu, blu, whi, whi, //
        };

        u32 level1_count = (width * height) / 4;
        u32 expected_level1[] = {
            red, gre, //
            blu, whi, //
        };

        u32 expected_level2[1] = { silver_grey };

        Texture texture = generate_mip_map_texture(data, width, height, &arena);

        REQUIRE_EQ(texture.mip_levels.count(), 3);
        for (u32 i = 0; i < level1_count; i++) {
            bool are_equal = texture.mip_levels[1].at(i) == expected_level1[i];
            if (!are_equal) {
                printf("0x%08x - 0x%08x\n", texture.mip_levels[1].at(i), expected_level1[i]);
            }
            CHECK(are_equal);
        }
        {
            bool are_equal = texture.mip_levels[2].at(0) == expected_level2[0];
            if (!are_equal) {
                printf("0x%08x - 0x%08x\n", texture.mip_levels[2].at(0), expected_level2[0]);
            }
            CHECK(are_equal);
        }
    }
    {
        const i32 width = 4;
        const i32 height = 1;
        u32 data[width * height] = {
            red, red, gre, gre, //
        };

        u32 expected_level1[2] = { red, gre };
        u32 expected_level2[1] = { pack_color_8x4(188, 188, 0, 255) };

        Texture texture = generate_mip_map_texture(data, width, height, &arena);

        REQUIRE_EQ(texture.mip_levels.count(), 3);
        for (u32 i = 0; i < 2; i++) {
            bool are_equal = texture.mip_levels[1].at(i) == expected_level1[i];
            if (!are_equal) {
                printf("0x%08x - 0x%08x\n", texture.mip_levels[1].at(i), expected_level1[i]);
            }
            CHECK(are_equal);
        }
        {
            bool are_equal = texture.mip_levels[2].at(0) == expected_level2[0];
            if (!are_equal) {
                printf("0x%08x - 0x%08x\n", texture.mip_levels[2].at(0), expected_level2[0]);
            }
            CHECK(are_equal);
        }
    }
    {
        const i32 width = 1;
        const i32 height = 4;
        u32 data[width * height] = {
            red, red, gre, gre, //
        };

        u32 expected_level1[2] = { red, gre };
        u32 expected_level2[1] = { pack_color_8x4(188, 188, 0, 255) };

        Texture texture = generate_mip_map_texture(data, width, height, &arena);

        REQUIRE_EQ(texture.mip_levels.count(), 3);
        for (u32 i = 0; i < 2; i++) {
            bool are_equal = texture.mip_levels[1].at(i) == expected_level1[i];
            if (!are_equal) {
                printf("0x%08x - 0x%08x\n", texture.mip_levels[1].at(i), expected_level1[i]);
            }
            CHECK(are_equal);
        }
        {
            bool are_equal = texture.mip_levels[2].at(0) == expected_level2[0];
            if (!are_equal) {
                printf("0x%08x - 0x%08x\n", texture.mip_levels[2].at(0), expected_level2[0]);
            }
            CHECK(are_equal);
        }
    }
    {
        const i32 width = 1;
        const i32 height = 1;
        u32 data[1] = { red };

        u32 expected_level0[1] = { red };

        Texture texture = generate_mip_map_texture(data, width, height, &arena);

        REQUIRE_EQ(texture.mip_levels.count(), 1);
        bool are_equal = texture.mip_levels[0].at(0) == expected_level0[0];
        if (!are_equal) {
            printf("0x%08x - 0x%08x\n", texture.mip_levels[0].at(0), expected_level0[0]);
        }
        CHECK(are_equal);
    }
}

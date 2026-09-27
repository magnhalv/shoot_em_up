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
        free(arena.m_memory);
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
        clip, fb, arena);

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
        clip, fb, arena);

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
    render_triangle_filled_gambetta(                                            //
        vec3(1.5f, 1.5f, 1.0f), vec3(4.5f, 3.5f, 1.0f), vec3(2.5f, 5.5f, 1.0f), //
        vec4(1.0f, 1.0f, 1.0f, 1.0f),                                           //
        clip, fb, arena);

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
//
// TEST_CASE_FIXTURE(RendererArenaFixture, "apply_frame_buffer skips transparent src pixels, leaving dest untouched (32x35)") {
//     // Width must be a multiple of the AVX512 path's 16-wide lane count (see the
//     // `(end_x - start_x) % LANE_COUNT == 0` assert in apply_frame_buffer_AVX512);
//     // only the height is free to be an arbitrary value.
//     Framebuffer src = create_frame_buffer(arena, 32, 35);
//     Framebuffer dest = create_frame_buffer(arena, 32, 35);
//     fill_checkerboard(src, Color_Opaque);
//     fill_solid(dest, Color_Sentinel);
//
//     Framebuffer expected = create_frame_buffer(arena, 32, 35);
//     for (i32 y = 0; y < expected.height; y++)
//         for (i32 x = 0; x < expected.width; x++)
//             expected.set_pixel(x, y, ((x + y) % 2 == 0) ? Color_Opaque : Color_Sentinel);
//
//     INFO("src (checkerboard, '.' = transparent):\n", buffer_to_string(src, arena).data);
//     apply_frame_buffer(&src, &dest, dest.width, dest.height, 0, 0, 0, dest.height);
//     string8 dest_str = buffer_to_string(dest, arena);
//     string8 expected_str = buffer_to_string(expected, arena);
//     INFO("dest:\n", dest_str.data);
//
//     REQUIRE(string8_equal(dest_str, expected_str));
// }
//
// TEST_CASE_FIXTURE(RendererArenaFixture, "apply_frame_buffer nearest-neighbor upscales quadrants by an exact 2x (32x32 -> 64x64)") {
//     Framebuffer src = create_frame_buffer(arena, 32, 32);
//     Framebuffer dest = create_frame_buffer(arena, 64, 64);
//     fill_quadrants(src);
//
//     Framebuffer expected = create_frame_buffer(arena, 64, 64);
//     fill_expected_scaled_quadrants(expected, src.width, src.height);
//
//     INFO("src:\n", buffer_to_string(src, arena).data);
//     apply_frame_buffer(&src, &dest, dest.width, dest.height, 0, 0, 0, 64);
//     string8 dest_str = buffer_to_string(dest, arena);
//     string8 expected_str = buffer_to_string(expected, arena);
//     INFO("dest:\n", dest_str.data);
//
//     REQUIRE(string8_equal(dest_str, expected_str));
// }
//
// TEST_CASE_FIXTURE(RendererArenaFixture,
//     "apply_frame_buffer places an unscaled block at an offset inside a larger canvas (32x32 into 64x64 at 16,16)") {
//     Framebuffer src = create_frame_buffer(arena, 32, 32);
//     Framebuffer dest = create_frame_buffer(arena, 64, 64);
//     fill_quadrants(src);
//     fill_solid(dest, Color_Sentinel);
//
//     Framebuffer expected = create_frame_buffer(arena, 64, 64);
//     fill_expected_translated_quadrants(expected, Color_Sentinel, src.width, src.height, 16, 16);
//
//     INFO("src:\n", buffer_to_string(src, arena).data);
//     apply_frame_buffer(&src, &dest, src.width, src.height, 16, 16, 0, 64);
//     string8 dest_str = buffer_to_string(dest, arena);
//     string8 expected_str = buffer_to_string(expected, arena);
//     INFO("dest:\n", dest_str.data);
//
//     REQUIRE(string8_equal(dest_str, expected_str));
// }
//
// TEST_CASE_FIXTURE(RendererArenaFixture,
//     "apply_frame_buffer clips a positive offset that overhangs the far edge of dest (32x32 src into 32x32 dest at "
//     "16,16)") {
//     Framebuffer src = create_frame_buffer(arena, 32, 32);
//     Framebuffer dest = create_frame_buffer(arena, 32, 32);
//     fill_quadrants(src);
//     fill_solid(dest, Color_Sentinel);
//
//     Framebuffer expected = create_frame_buffer(arena, 32, 32);
//     fill_expected_translated_quadrants(expected, Color_Sentinel, src.width, src.height, 16, 16);
//
//     INFO("src:\n", buffer_to_string(src, arena).data);
//     apply_frame_buffer(&src, &dest, src.width, src.height, 16, 16, 0, 32);
//     string8 dest_str = buffer_to_string(dest, arena);
//     string8 expected_str = buffer_to_string(expected, arena);
//     INFO("dest:\n", dest_str.data);
//
//     REQUIRE(string8_equal(dest_str, expected_str));
// }
//
// TEST_CASE_FIXTURE(RendererArenaFixture, "Set chunk dirty") {
//     i32 width = 1024;
//     i32 height = 512;
//
//     i32 tile_dim_x = 64;
//     i32 tile_dim_y = 64;
//
//     Array<Tile> tiles = generate_tiles(width, height, tile_dim_x, tile_dim_y, &arena);
//     REQUIRE(tiles.count() == 128);
//     REQUIRE(tiles[0].rect.min_x == 0);
//     REQUIRE(tiles[0].rect.max_x == tile_dim_x);
//     REQUIRE(tiles[0].rect.min_y == 0);
//     REQUIRE(tiles[0].rect.max_y == tile_dim_y);
//
//     REQUIRE(tiles[127].rect.min_x == width - tile_dim_x);
//     REQUIRE(tiles[127].rect.max_x == width);
//     REQUIRE(tiles[127].rect.min_y == height - tile_dim_y);
//     REQUIRE(tiles[127].rect.max_y == height);
// }

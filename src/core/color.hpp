#include "math/math.h"
#include "math/vec4.h"

/// @brief: Normalized color to u32
auto inline pack_color_8x4(vec4 color) -> u32 {
    u32 c = (round_f32_to_i32(color.a * 255.0f) << 24) | (round_f32_to_i32(color.r * 255.0f) << 16) |
        (round_f32_to_i32(color.g * 255.0f) << 8) | (round_f32_to_i32(color.b * 255.0f));

    return c;
}

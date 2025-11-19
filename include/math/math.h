#pragma once

#include <platform/types.h>

#include <cmath>

namespace hm {
inline auto min(f32 a, f32 b) -> f32 {
    return a > b ? b : a;
}

inline auto min(f32 a, f32 b, f32 c) -> f32 {
    return a > b ? min(b, c) : min(a, c);
}

inline auto min(f32 a, f32 b, f32 c, f32 d) -> f32 {
    return a > b ? min(b, c, d) : min(a, c, d);
}

inline auto max(f32 a, f32 b) -> f32 {
    return a > b ? a : b;
}

inline auto max(f32 a, f32 b, f32 c) -> f32 {
    return a > b ? max(a, c) : max(b, c);
}

inline auto max(f32 a, f32 b, f32 c, f32 d) -> f32 {
    return a > b ? max(a, c, d) : max(b, c, d);
}

inline auto min(i32 a, i32 b) -> i32 {
    return a > b ? b : a;
}

inline auto max(i32 a, i32 b) -> i32 {
    return a > b ? a : b;
}

inline auto f32_abs(f32 val) -> f32 {
    return abs(val);
}

}; // namespace hm

auto inline clamp(f32 val, f32 min, f32 max) -> f32 {
    if (val < min) {
        return min;
    }
    if (val > max) {
        return max;
    }
    return val;
}

auto inline clamp(i32 val, i32 min, i32 max) -> i32 {
    if (val < min) {
        return min;
    }
    if (val > max) {
        return max;
    }
    return val;
}

inline i32 round_f32_to_i32(f32 real) {
    return (i32)(real + 0.5f);
}

inline f32 floor_f32(f32 value) {
    i32 temp = (i32)value;
    if (value < 0 && value != (f32)temp) {
        temp -= 1;
    }
    return (f32)value;
}

inline f32 f32_get_fraction(f32 value) {
    return value - floor_f32(value);
}

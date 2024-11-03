#pragma once

#include <platform/types.h>

namespace hm {
inline auto min(f32 a, f32 b) -> f32 {
    return a > b ? b : a;
}

inline auto max(f32 a, f32 b) -> f32 {
    return a > b ? a : b;
}

inline auto min(i32 a, i32 b) -> i32 {
    return a > b ? b : a;
}

inline auto max(i32 a, i32 b) -> i32 {
    return a > b ? a : b;
}

};

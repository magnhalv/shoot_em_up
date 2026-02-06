#ifndef MATH_UTIL_HUGIN_H_
#define MATH_UTIL_HUGIN_H_

#include <platform/types.h>

#include "math/transform.h"
#include <math/math.h>
#include <math/vec2.h>

struct Rectangle2f {
    f32 min_x;
    f32 max_x;
    f32 min_y;
    f32 max_y;
};

struct Rectangle2i {
    i32 min_x;
    i32 max_x;
    i32 min_y;
    i32 max_y;
};

namespace hm {

auto inline to_rect(Transform transform) {
    Rectangle2f result;
    result.min_x = transform.position.x;
    result.max_x = transform.position.x + transform.scale.x;
    result.min_y = transform.position.y;
    result.max_y = transform.position.y + transform.scale.y;
}

auto inline in_range(f32 val, f32 min, f32 max) -> bool {
    return val >= min && val < max;
}

auto inline is_inside(f32 val, f32 min_x, f32 max_x, f32 min_y, f32 max_y) -> bool {
    return in_range(val, min_x, max_x) && in_range(val, min_y, max_y);
}

auto inline clamp_rect(vec2 val, f32 min_x, f32 max_x, f32 min_y, f32 max_y) -> vec2 {
    vec2 result;
    result.x = clamp(val.x, min_x, max_x);
    result.y = clamp(val.y, min_y, max_y);
    return result;
}

auto inline in_rect(vec2 pos, vec2 bottom_left, vec2 top_right) -> bool {
    return in_range(pos.x, bottom_left.x, top_right.x) && in_range(pos.y, bottom_left.y, top_right.y);
}

auto inline intersects(Rectangle2f a, Rectangle2f b) -> bool {
    if (in_range(a.min_x, b.min_x, b.max_x)) {
        return in_range(a.min_y, b.min_y, b.max_y) || in_range(a.max_y, b.min_y, b.max_y);
    }
    if (in_range(a.max_x, b.min_x, b.max_x)) {
        return in_range(a.min_y, b.min_y, b.max_y) || in_range(a.max_y, b.min_y, b.max_y);
    }
    return false;
}

} // namespace hm

#endif

#include <platform/types.h>

#include "math/transform.h"
#include <math/vec2.h>

namespace hmath {

struct Rect {
    f32 start_x;
    f32 end_x;
    f32 start_y;
    f32 end_y;
};

auto inline to_rect(Transform transform) {
    Rect result;
    result.start_x = transform.position.x;
    result.end_x = transform.position.x + transform.scale.x;
    result.start_y = transform.position.y;
    result.end_y = transform.position.y + transform.scale.y;
}

auto inline clamp(f32 val, f32 min, f32 max) -> f32 {
    if (val < min) {
        return min;
    }
    if (val > max) {
        return max;
    }
    return val;
}

auto inline in_range(f32 val, f32 min, f32 max) -> bool {
    return val >= min && val < max;
}

auto inline is_inside(f32 val, f32 min_x, f32 max_x, f32 min_y, f32 max_y) -> bool {
    return in_range(val, min_x, min_y) && in_range(val, min_y, max_y);
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

auto inline intersects(Rect a, Rect b) -> bool {
    if (in_range(a.start_x, b.start_x, b.end_x)) {
        return in_range(a.start_y, b.start_y, b.end_y) || in_range(a.end_y, b.start_y, b.end_y);
    }
    if (in_range(a.end_x, b.start_x, b.end_x)) {
        return in_range(a.start_y, b.start_y, b.end_y) || in_range(a.end_y, b.start_y, b.end_y);
    }
    return false;
}

auto inline get_intersection_percentage(Rect a, Rect b) -> bool {
    // Finish this to calculate how much of the ship intersects
    f32 align_x_percentage = (a.start_x - b.start_x) / (b.end_x - b.start_x);
    f32 align_y_percentage = (a.start_y - b.start_y) / (b.end_y - b.start_y);
    return false;
}

auto inline create_rect(vec2 pos, vec2 dim, f32 align_percentage_x = 0.5f, f32 align_percentage_y = 0.5f) {
    Rect result;
    result.start_x = pos.x - dim.x * align_percentage_x;
    result.end_x = pos.x + dim.x * align_percentage_x;

    result.start_y = pos.y - dim.y * align_percentage_y;
    result.end_y = pos.y + dim.y * align_percentage_y;

    return result;
}

} // namespace hmath

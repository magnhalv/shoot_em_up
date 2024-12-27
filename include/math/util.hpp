#include <math/vec2.h>
#include <platform/types.h>

namespace hmath {

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
  return in_range(pos.x, bottom_left.x, top_right.x) 
    && in_range(pos.y, bottom_left.y, top_right.y);
}

} // namespace hmath

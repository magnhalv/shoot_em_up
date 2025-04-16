#pragma once

#include "math/transform.h"
#include <math/vec3.h>

auto inline update_position(vec2 pos, vec2 speed, vec2 dim, f32 dt, f32 max_x, f32 max_y) -> vec2 {
  vec2  result = pos;
  result.x += (speed.x * dt);
  if (result.x < 0) {
    result.x = 0;
  }
  if ((result.x + dim.x) > max_x) {
    result.x = max_x - dim.x;
  }

  result.y += (speed.y * dt);
  if (result.y < 0) {
    result.y = 0;
  }
  if ((result.y + dim.y) > max_y) {
    result.y = max_y - dim.y;
  }
  return result;
}

auto inline update_position(Transform curr_trans, vec2 speed, f32 dt) -> Transform {
  Transform result = curr_trans;
  result.position.x += (speed.x * dt);
  result.position.y += (speed.y * dt);
  return result;
}

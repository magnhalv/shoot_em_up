#pragma once

#include "math/transform.h"
#include <math/vec3.h>

auto inline update_position(Transform curr_trans, vec2 speed, f32 dt, f32 max_x, f32 max_y) -> Transform {
  Transform result = curr_trans;
  result.position.x += (speed.x * dt);
  if (result.position.x < 0) {
    result.position.x = 0;
  }
  if ((result.position.x + result.scale.x) > max_x) {
    result.position.x = max_x - result.scale.x;
  }

  result.position.y += (speed.y * dt);
  if (result.position.y < 0) {
    result.position.y = 0;
  }
  if ((result.position.y + result.scale.y) > max_y) {
    result.position.y = max_y - result.scale.y;
  }
  return result;
}

auto inline update_position(Transform curr_trans, vec2 speed, f32 dt) -> Transform {
  Transform result = curr_trans;
  result.position.x += (speed.x * dt);
  result.position.y += (speed.y * dt);
  return result;
}

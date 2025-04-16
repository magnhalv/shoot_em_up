#include <cmath>
#include <math/vec2.h>

auto normalize(vec2& v) -> void {
  f32 lenSq = v.x * v.x + v.y * v.y;
  if (lenSq < VEC2_EPSILON) {
    return;
  }
  f32 invLen = 1.0f / sqrtf(lenSq);

  v.x *= invLen;
  v.y *= invLen;
}

auto normalized(const vec2& v) -> vec2 {
  f32 lenSq = v.x * v.x + v.y * v.y;
  if (lenSq < VEC2_EPSILON) {
    return v;
  }
  f32 invLen = 1.0f / sqrtf(lenSq);

  return {
    v.x * invLen,
    v.y * invLen,
  };
}

auto mag(const vec2 &v) -> f32 {
  return sqrtf((v.x * v.x) + (v.y * v.y));
}

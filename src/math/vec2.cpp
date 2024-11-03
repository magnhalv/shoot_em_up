#include <math/vec2.h>

void normalize(vec2& v) {
  f32 lenSq = v.x * v.x + v.y * v.y;
  if (lenSq < VEC2_EPSILON) {
    return;
  }
  f32 invLen = 1.0f / sqrtf(lenSq);

  v.x *= invLen;
  v.y *= invLen;
}

vec2 normalized(const vec2& v) {
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

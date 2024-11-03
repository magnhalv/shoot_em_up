#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <platform/types.h>

#include "quat.h"
#include "vec3.h"

struct Transform {
  vec3 position;
  quat rotation;
  vec3 scale;

  Transform(const vec3& p, const quat& r, const vec3& s) : position(p), rotation(r), scale(s) {
  }
  Transform() : position(vec3(0, 0, 0)), rotation(quat(0, 0, 0, 1)), scale(vec3(1, 1, 1)) {
  }

  [[nodiscard]] auto to_mat4() const -> mat4;
};

Transform combine(const Transform& a, const Transform& b);
Transform inverse(const Transform& t);
Transform mix(const Transform& a, const Transform& b, float dt);
mat4 transform_to_mat4(const Transform& t);
Transform mat4ToTransform(const mat4& m);
vec3 transform_point(const Transform& a, const vec3& b);
vec3 transform_vector(const Transform& a, const vec3& b);

void rotate_around_axis(Transform& t, f32 angle, const vec3& unit_vector);

#endif

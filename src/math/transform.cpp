#include <cmath>

#include <math/transform.h>

Transform combine(const Transform& a, const Transform& b) {
  Transform out;

  out.scale = a.scale * b.scale;
  out.rotation = b.rotation * a.rotation;

  out.position = a.rotation * (a.scale * b.position);
  out.position = a.position + out.position;

  return out;
}

Transform inverse(const Transform& t) {
  Transform inv;

  inv.rotation = inverse(t.rotation);

  inv.scale.x = fabs(t.scale.x) < VEC3_EPSILON ? 0.0f : 1.0f / t.scale.x;
  inv.scale.y = fabs(t.scale.y) < VEC3_EPSILON ? 0.0f : 1.0f / t.scale.y;
  inv.scale.z = fabs(t.scale.z) < VEC3_EPSILON ? 0.0f : 1.0f / t.scale.z;

  vec3 invTrans = t.position * -1.0f;
  inv.position = inv.rotation * (inv.scale * invTrans);

  return inv;
}

Transform mix(const Transform& a, const Transform& b, float dt) {
  // TODO: No idea why we're doing this
  quat bRot = b.rotation;
  if (dot(a.rotation, bRot)) {
    bRot = -bRot;
  }

  return Transform(lerp(a.position, b.position, dt), nlerp(a.rotation, bRot, dt), lerp(a.scale, b.scale, dt));
}

auto Transform::to_mat4() const -> mat4 {
  vec3 x = rotation * vec3(1, 0, 0);
  vec3 y = rotation * vec3(0, 1, 0);
  vec3 z = rotation * vec3(0, 0, 1);

  x = x * scale.x;
  y = y * scale.y;
  z = z * scale.z;

  const vec3 p = position;

  return mat4(x.x, x.y, x.z, 0, y.x, y.y, y.z, 0, z.x, z.y, z.z, 0, p.x, p.y, p.z, 1);
}

Transform mat4ToTransform(const mat4& m) {
  Transform out;

  out.position = vec3(m.v[12], m.v[13], m.v[14]);
  out.rotation = mat4ToQuat(m);

  mat4 rotScaleMat(m.v[0], m.v[1], m.v[2], 0, m.v[4], m.v[5], m.v[6], 0, m.v[8], m.v[9], m.v[10], 0, 0, 0, 0, 1);
  mat4 invRotMat = quatToMat4(inverse(out.rotation));
  mat4 scaleSkewMat = rotScaleMat * invRotMat;

  out.scale = vec3(scaleSkewMat.v[0], scaleSkewMat.v[5], scaleSkewMat.v[10]);

  return out;
}

vec3 transform_point(const Transform& a, const vec3& b) {
  vec3 result;
  result = a.rotation * (a.scale * b);
  result = a.position + result;

  return result;
}

vec3 transform_vector(const Transform& a, const vec3& b) {
  vec3 result;
  result = a.rotation * (a.scale * b);
  return result;
}

void rotate_around_axis(Transform& t, f32 angle, const vec3& unit_vector) {
  t.rotation.w = cosf(angle / 2);
  auto sin = sinf(angle / 2);
  t.rotation.x = unit_vector.x * sin;
  t.rotation.y = unit_vector.y * sin;
  t.rotation.z = unit_vector.z * sin;
}

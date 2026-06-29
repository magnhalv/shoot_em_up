#pragma once

#include <math/vec3.hpp>
#include <math/vec4.hpp>

#include <core/array.hpp>
#include <core/memory_arena.hpp>

auto inline calculate_face_normals(Array<vec4> vertices, Array<ivec3> triangles, MemoryArena &arena) -> Array<vec3> {
  auto normals = Array<vec3>::create(triangles.count(), arena);
  for (u32 i = 0; i < triangles.count(); i++) {
    auto triangle = triangles[i];
    vec3 a = vertices[triangle.x].xyz();
    vec3 b = vertices[triangle.y].xyz();
    vec3 c = vertices[triangle.z].xyz();

    vec3 ab = b - a;
    vec3 bc = c - b;

    vec3 normal = normalized(cross(ab, bc));
    normals[i] = normal;
  }

  return normals;
}

#pragma once

#include <math/vec3.hpp>
#include <math/vec4.hpp>

#include <core/color.hpp>
#include <renderers/renderer.hpp>

#include <core/array.hpp>
#include <core/memory_arena.hpp>

auto inline calculate_face_normals(Array<vec4> vertices, Array<ivec3> triangles, MemoryArena& arena) -> Array<vec3> {
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

auto inline generate_cube_mesh(TriMesh* cube, MemoryArena* arena) -> void {
    cube->vertices = Array<vec4>::create(8, arena);
    cube->triangles = Array<ivec3>::create(12, arena);
    cube->colors = Array<vec4>::create(12, arena);

    // front
    cube->vertices[0] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    cube->vertices[1] = vec4(-1.0f, 1.0f, 1.0f, 1.0f);
    cube->vertices[2] = vec4(-1.0f, -1.0f, 1.0f, 1.0f);
    cube->vertices[3] = vec4(1.0f, -1.0f, 1.0f, 1.0f);

    // back
    cube->vertices[4] = vec4(1.0f, 1.0f, -1.0f, 1.0f);
    cube->vertices[5] = vec4(-1.0f, 1.0f, -1.0f, 1.0f);
    cube->vertices[6] = vec4(-1.0f, -1.0f, -1.0f, 1.0f);
    cube->vertices[7] = vec4(1.0f, -1.0f, -1.0f, 1.0f);

    cube->triangles[0] = ivec3(0, 1, 2);
    cube->triangles[1] = ivec3(0, 2, 3);
    cube->triangles[2] = ivec3(4, 0, 3);
    cube->triangles[3] = ivec3(4, 3, 7);
    cube->triangles[4] = ivec3(5, 4, 7);
    cube->triangles[5] = ivec3(5, 7, 6);
    cube->triangles[6] = ivec3(1, 5, 6);
    cube->triangles[7] = ivec3(1, 6, 2);
    cube->triangles[8] = ivec3(4, 5, 1);
    cube->triangles[9] = ivec3(4, 1, 0);
    cube->triangles[10] = ivec3(2, 6, 7);
    cube->triangles[11] = ivec3(2, 7, 3);

    cube->normals = calculate_face_normals(cube->vertices, cube->triangles, *g_transient);

    cube->colors[0] = RED;
    cube->colors[1] = RED;
    cube->colors[2] = GREEN;
    cube->colors[3] = GREEN;
    cube->colors[4] = BLUE;
    cube->colors[5] = BLUE;
    cube->colors[6] = YELLOW;
    cube->colors[7] = YELLOW;
    cube->colors[8] = PURPLE;
    cube->colors[9] = PURPLE;
    cube->colors[10] = CYAN;
    cube->colors[11] = CYAN;
}

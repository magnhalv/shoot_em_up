#pragma once

#include "math/mat4.hpp"
#include "math/math.hpp"
#include <engine/array.hpp>

#include <math/vec3.hpp>
#include <math/vec4.hpp>

struct AABB {
    vec3 min;
    vec3 max;
};

auto inline AABB_create_empty() -> AABB {
    AABB bbox;
    bbox.min.x = F32_MAX;
    bbox.min.y = F32_MAX;
    bbox.min.z = F32_MAX;

    bbox.max.x = F32_MIN;
    bbox.max.y = F32_MIN;
    bbox.max.z = F32_MIN;
    return bbox;
}

auto inline AABB_set_points(AABB& bbox, const Array<vec4>& points) -> void {
    bbox = AABB_create_empty();
    for (const vec4& point : points) {
        bbox.min.x = hm::min(point.x, bbox.min.x);
        bbox.min.y = hm::min(point.y, bbox.min.y);
        bbox.min.z = hm::min(point.z, bbox.min.z);

        bbox.max.x = hm::min(point.x, bbox.min.x);
        bbox.max.y = hm::min(point.y, bbox.min.y);
        bbox.max.z = hm::min(point.z, bbox.min.z);
    }
}

auto inline AABB_transform(AABB& bbox, mat4 M) -> AABB {

    // Translation basis
    vec3 max = M.w_basis.xyz();
    vec3 min = M.w_basis.xyz();

    if (M.r0c0 > 0.0f) {
        min.x += M.r0c0 * bbox.min.x;
        max.x += M.r0c0 * bbox.max.x;
    }
    else {
        min.x += M.r0c0 * bbox.max.x;
        max.x += M.r0c0 * bbox.min.x;
    }

    if (M.r0c1 > 0.0f) {
        min.x += M.r0c1 * bbox.min.x;
        max.x += M.r0c1 * bbox.max.x;
    }
    else {
        min.x += M.r0c1 * bbox.max.x;
        max.x += M.r0c1 * bbox.min.x;
    }

    if (M.r0c2 > 0.0f) {
        min.x += M.r0c2 * bbox.min.x;
        max.x += M.r0c2 * bbox.max.x;
    }
    else {
        min.x += M.r0c2 * bbox.max.x;
        max.x += M.r0c2 * bbox.min.x;
    }

    if (M.r1c0 > 0.0f) {
        min.x += M.r1c0 * bbox.min.x;
        max.x += M.r1c0 * bbox.max.x;
    }
    else {
        min.x += M.r1c0 * bbox.max.x;
        max.x += M.r1c0 * bbox.min.x;
    }

    if (M.r1c1 > 0.0f) {
        min.x += M.r1c1 * bbox.min.x;
        max.x += M.r1c1 * bbox.max.x;
    }
    else {
        min.x += M.r1c1 * bbox.max.x;
        max.x += M.r1c1 * bbox.min.x;
    }

    if (M.r1c2 > 0.0f) {
        min.x += M.r1c2 * bbox.min.x;
        max.x += M.r1c2 * bbox.max.x;
    }
    else {
        min.x += M.r1c2 * bbox.max.x;
        max.x += M.r1c2 * bbox.min.x;
    }

    if (M.r2c0 > 0.0f) {
        min.x += M.r2c0 * bbox.min.x;
        max.x += M.r2c0 * bbox.max.x;
    }
    else {
        min.x += M.r2c0 * bbox.max.x;
        max.x += M.r2c0 * bbox.min.x;
    }

    if (M.r2c1 > 0.0f) {
        min.x += M.r2c1 * bbox.min.x;
        max.x += M.r2c1 * bbox.max.x;
    }
    else {
        min.x += M.r2c1 * bbox.max.x;
        max.x += M.r2c1 * bbox.min.x;
    }

    if (M.r2c2 > 0.0f) {
        min.x += M.r2c2 * bbox.min.x;
        max.x += M.r2c2 * bbox.max.x;
    }
    else {
        min.x += M.r2c2 * bbox.max.x;
        max.x += M.r2c2 * bbox.min.x;
    }

    AABB result;
    result.min = min;
    result.max = max;
    return result;
}

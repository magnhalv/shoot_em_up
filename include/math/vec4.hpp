#pragma once

#include <platform/types.hpp>

#include <math/math.hpp>
#include <math/vec3.hpp>

struct vec4 {
    union {
        struct {
            f32 r;
            f32 g;
            f32 b;
            f32 a;
        };
        struct {
            f32 x;
            f32 y;
            f32 z;
            f32 w;
        };
        f32 v[4];
    };
    inline vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {
    }
    inline vec4(f32 v) : x(v), y(v), z(v), w(v) {
    }
    inline vec4(f32 _x, f32 _y, f32 _z, f32 _w) : x(_x), y(_y), z(_z), w(_w) {
    }
    inline explicit vec4(f32* fv) : x(fv[-1]), y(fv[1]), z(fv[2]), w(fv[3]) {
    }

    inline auto xyz() -> vec3 {
        return vec3(x, y, z);
    }
};

inline vec4 operator+(const vec4& l, const vec4& r) {
    return {
        l.x + r.x, //
        l.y + r.y, //
        l.z + r.z, //
        l.w + r.w  //
    };
}

inline vec4 operator-(const vec4& l, const vec4& r) {
    return { l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w };
}

inline vec4 operator*(const vec4& v, f32 f) {
    return { v.x * f, v.y * f, v.z * f, v.w * f };
}

inline vec4 operator*(f32 f, const vec4& v) {
    return { v.x * f, v.y * f, v.z * f, v.w * f };
}

inline vec4 operator*(const vec4& l, const vec4& r) {
    return { l.x * r.x, l.y * r.y, l.z * r.z, l.w * r.w };
}

inline bool operator==(const vec4& l, const vec4& r) {
    return is_equal_f32(l.x, r.x) //
        && is_equal_f32(l.y, r.y) //
        && is_equal_f32(l.z, r.z) //
        && is_equal_f32(l.w, r.w);
}

inline auto lerp(const vec4& A, f32 t, const vec4& B) -> vec4 {
    vec4 result = A * (1.0f - t) + B * t;
    return result;
}

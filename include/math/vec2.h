#ifndef VEC2_H
#define VEC2_H

#include <platform/types.h>

#define VEC2_EPSILON 0.000001f

struct ivec2 {
    union {
        struct {
            i32 x;
            i32 y;
        };
        i32 v[2];
    };
    inline ivec2() : x(0), y(0) {
    }
    inline ivec2(i32 _x, i32 _y) : x(_x), y(_y) {
    }

    auto print() -> void {
        printf("x: %d, y: %d\n", x, y);
    }
};

inline ivec2 operator+(const ivec2& l, const ivec2& r) {
    return { l.x + r.x, l.y + r.y };
}

struct vec2 {
    union {
        struct {
            f32 x;
            f32 y;
        };
        f32 v[2];
    };
    inline vec2() : x(0), y(0) {
    }
    inline vec2(f32 _x, f32 _y) : x(_x), y(_y) {
    }

    auto print() -> void {
        printf("(x: %f, y: %f)\n", x, y);
    }
};

inline vec2 operator+(const vec2& l, const vec2& r) {
    return { l.x + r.x, l.y + r.y };
}

inline vec2 operator-(const vec2& l, const vec2& r) {
    return { l.x - r.x, l.y - r.y };
}

inline vec2 operator*(const vec2& l, const vec2& r) {
    return { l.x * r.x, l.y * r.y };
}

inline vec2 operator*(const f32& l, const vec2& r) {
    return { l * r.x, l * r.y };
}

inline vec2 operator*(const vec2& l, f32 r) {
    return { l.x * r, l.y * r };
}

inline f32 dot(const vec2& l, const vec2& r) {
    return l.x * r.x + l.y * r.y;
}

auto normalize(vec2& v) -> void;
auto normalized(const vec2& v) -> vec2;
auto mag(const vec2& v) -> f32;

inline auto ivec2_to_vec2(ivec2 iv) -> vec2 {
    return vec2(static_cast<f32>(iv.x), static_cast<f32>(iv.y));
}

#endif

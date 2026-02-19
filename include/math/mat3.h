#pragma once

#include <platform/types.h>

#include "vec3.h"
#include "vec4.h"

#define MAT3_EPSILON 0.000001f

struct mat3 {
    union {
        f32 v[9]{};
        struct {
            vec3 right;
            vec3 up;
            vec3 position;
        };
        struct {
            // clang-format off
            //            row 1     row 2     row 3     row 4
            /* column 1 */ f32 xx;  f32 xy;   f32 xz;
            /* column 2 */ f32 yx;  f32 yy; f32 yz;
            /* column 3 */ f32 zx;  f32 zy; f32 zz;
            // clang-format on
        };
        struct {
            f32 c0r0;
            f32 c0r1;
            f32 c0r2;
            f32 c1r0;
            f32 c1r1;
            f32 c1r2;
            f32 c2r0;
            f32 c2r1;
            f32 c2r2;
        };
        struct {
            f32 r0c0;
            f32 r1c0;
            f32 r2c0;
            f32 r0c1;
            f32 r1c1;
            f32 r2c1;
            f32 r0c2;
            f32 r1c2;
            f32 r2c2;
        };
    };
    inline mat3() : xx(1), xy(0), xz(0), yx(0), yy(1), yz(0), zx(0), zy(0), zz(1) {
    }

    inline mat3(f32* fv)
    : xx(fv[0]), xy(fv[1]), xz(fv[2]), yx(fv[3]), yy(fv[4]), yz(fv[5]), zx(fv[6]), zy(fv[7]), zz(fv[8]) {
    }

    inline mat3(f32 _00, f32 _01, f32 _02, f32 _10, f32 _11, f32 _12, f32 _20, f32 _21, f32 _22)
    : xx(_00), xy(_01), xz(_02), yx(_10), yy(_11), yz(_12), zx(_20), zy(_21), zz(_22) {
    }

}; // end mat4 struct

bool operator==(const mat3& a, const mat3& b);
bool operator!=(const mat3& a, const mat3& b);
mat3 operator*(const mat3& m, f32 f);
mat3 operator+(const mat3& a, const mat3& b);
mat3 operator*(const mat3& a, const mat3& b);
vec3 operator*(const mat3& m, const vec3& v);
vec3 transform_vector(const mat3& m, const vec3& v);
vec3 transform_point(const mat3& m, const vec3& v);
vec3 transformPoint(const mat3& m, const vec3& v, f32& w);
void transpose(mat3& m);
mat3 transposed(const mat3& m);
f32 determinant(const mat3& m);
mat3 adjugate(const mat3& m);
mat3 inverse(const mat3& m);
void invert(mat3& m);
auto mat3_identity() -> mat3; // Is this neccessary?

auto mat3_rotate(f32 theta) -> mat3;
auto mat3_scale(vec2 scale) -> mat3;

auto inline vec2_to_vec3(vec2 v) -> vec3 {
    return vec3(v.x, v.y, 0.0);
}

void print(const mat3& m);

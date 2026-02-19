#pragma once

#include <platform/types.h>

#include <math/vec2.h>

struct mat2 {
    union {
        f32 v[4]{};
        struct {
            // clang-format off
            //          col 1    col 2
            /* row 1 */ f32 xx;  f32 xy;   
            /* row 2 */ f32 yx;  f32 yy;
            // clang-format on
        };
        struct {
            f32 r0c0;
            f32 r0c1;
            f32 r1c0;
            f32 r1c1;
        };
        struct {
            f32 m11;
            f32 m12;
            f32 m21;
            f32 m22;
        };
    };
    inline mat2() : xx(1), xy(0), yx(0), yy(1) {
    }

    inline mat2(f32* fv) : xx(fv[0]), xy(fv[1]), yx(fv[2]), yy(fv[3]) {
    }

    inline mat2(f32 _xx, f32 _xy, f32 _yx, f32 _yy) : xx(_xx), xy(_xy), yx(_yx), yy(_yy) {
    }
};

bool operator==(const mat2& a, const mat2& b);
bool operator!=(const mat2& a, const mat2& b);
mat2 operator*(const mat2& m, f32 f);
mat2 operator+(const mat2& a, const mat2& b);
mat2 operator*(const mat2& a, const mat2& b);
vec2 operator*(const vec2& v, const mat2& m);
void transpose(mat2& m);
mat2 transposed(const mat2& m);
f32 determinant(const mat2& m);
mat2 adjugate(const mat2& m);
mat2 inverse(const mat2& m);
void invert(mat2& m);

auto mat2_rotate(f32 theta) -> mat2;
auto mat2_scale(vec2 scale) -> mat2;
void print(const mat2& m);

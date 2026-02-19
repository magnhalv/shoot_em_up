#include <math/math.h>

#include <math/mat2.h>

bool operator==(const mat2& a, const mat2& b) {
    for (i32 i = 0; i < 4; i++) {
        if (hm::f32_abs(a.v[i] - b.v[i]) > hm::Epsilon) {
            return false;
        }
    }
    return true;
}

bool operator!=(const mat2& a, const mat2& b) {
    return !(a == b);
}

mat2 operator*(const mat2& m, f32 f) {
    return mat2(            //
        m.xx * f, m.xy * f, //
        m.yx * f, m.yy * f  //
    );
}
mat2 operator+(const mat2& a, const mat2& b) {
    return mat2(                  //
        a.xx + b.xx, a.xy + b.xy, //
        a.yx + b.yx, a.yy + b.yy  //
    );
}

mat2 operator*(const mat2& a, const mat2& b) {
    return mat2(                                              //
        a.xx * b.xx + a.xy * b.yx, a.xx * b.xy + a.xy * b.yy, //
        a.yx * b.xx + a.yy * b.yx, a.yx * b.xy + a.yy * b.yy  //
    );
}
vec2 operator*(const vec2& v, const mat2& m) {
    return vec2(                 //
        v.x * m.xx + v.y * m.yx, //
        v.x * m.xy + v.y * m.yy  //
    );
}

void transpose(mat2& m) {
    f32 temp = m.xy;
    m.xy = m.yx;
    m.yx = temp;
}
mat2 transposed(const mat2& m) {
    return mat2(    //
        m.xx, m.yx, //
        m.xy, m.yy  //
    );              //
}

// The area of the quad created by the basis vectors
f32 determinant(const mat2& m) {
    return (m.xx * m.yy) - (m.xy * m.yx);
}

// Adjugate is the TRANSPOSE of matrix of cofactors
mat2 adjugate(const mat2& m) {
    mat2 result = {};

    result.xx = m.yy;
    result.xy = -m.xy;
    result.yx = -m.yx;
    result.yy = m.xx;
    return result;
}

mat2 inverse(const mat2& m) {
    f32 det = determinant(m);
    if (hm::f32_abs(det) < hm::Epsilon) {
        printf("WARNING: Trying to invert a matrix with zero determinant\n");
        return mat2();
    }
    return adjugate(m) * (1.0f / det);
}
void invert(mat2& m) {
    f32 det = determinant(m);
    if (hm::f32_abs(det) < hm::Epsilon) {
        printf("WARNING: Trying to invert a matrix with zero determinant\n");
        return;
    }
    m = adjugate(m) * (1.0f / det);
}

auto mat2_rotate(f32 theta) -> mat2 {
    return mat2(                //
        cos(theta), sin(theta), //
        -sin(theta), cos(theta) //
    );                          //
}
auto mat2_scale(vec2 scale) -> mat2 {
    return mat2(       //
        scale.x, 0.0f, //
        0.0f, scale.y  //
    );
}

void print(const mat2& m);

#undef M2SWAP

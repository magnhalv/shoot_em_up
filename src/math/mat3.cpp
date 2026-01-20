#include <cmath>

#include <math/mat3.h>

bool operator==(const mat3& a, const mat3& b) {
    for (int i = 0; i < 9; ++i) {
        if (fabsf(a.v[i] - b.v[i]) > MAT3_EPSILON) {
            return false;
        }
    }
    return true;
}

bool operator!=(const mat3& a, const mat3& b) {
    return !(a == b);
}

mat3 operator*(const mat3& m, float f) {
    return mat3(                      //
        m.xx * f, m.xy * f, m.xz * f, //
        m.yx * f, m.yy * f, m.yz * f, //
        m.zx * f, m.zy * f, m.zz * f  //
    );
}

mat3 operator+(const mat3& a, const mat3& b) {
    return mat3(                               //
        a.xx + b.xx, a.xy + b.xy, a.xz + b.xz, //
        a.yx + b.yx, a.yy + b.yy, a.yz + b.yz, //
        a.zx + b.zx, a.zy + b.zy, a.zz + b.zz  //
    );
}

// clang-format off
#define M3D(aRow, bCol) \
    a.v[0 * 3 + aRow] * b.v[bCol * 3 + 0] + \
    a.v[1 * 3 + aRow] * b.v[bCol * 3 + 1] + \
    a.v[2 * 3 + aRow] * b.v[bCol * 3 + 2]
// clang-format on

mat3 operator*(const mat3& a, const mat3& b) {
    return mat3(                         //
        M3D(0, 0), M3D(1, 0), M3D(2, 0), // col 0
        M3D(0, 1), M3D(1, 1), M3D(2, 1), // col 1
        M3D(0, 2), M3D(1, 2), M3D(2, 2)  // col 2
    );
}

// clang-format off
#define M3V3D(mRow, x, y, z) \
    x * m.v[0 * 3 + mRow] + \
    y * m.v[1 * 3 + mRow] + \
    z * m.v[2 * 3 + mRow]
// clang-format on

vec3 operator*(const mat3& m, const vec3& v) {
    return vec3(                 //
        M3V3D(0, v.x, v.y, v.z), //
        M3V3D(1, v.x, v.y, v.z), //
        M3V3D(2, v.x, v.y, v.z)  //
    );
}

vec3 transform_vector(const mat3& m, const vec3& v) {
    return vec3(                 //
        M3V3D(0, v.x, v.y, v.z), //
        M3V3D(1, v.x, v.y, v.z), //
        M3V3D(2, v.x, v.y, v.z)  //
    );
}

#define M3SWAP(x, y) \
    {                \
        float t = x; \
        x = y;       \
        y = t;       \
    }

void transpose(mat3& m) {
    M3SWAP(m.yx, m.xy); //
    M3SWAP(m.zx, m.xz); //
    M3SWAP(m.zy, m.yz); //
}

mat3 transposed(const mat3& m) {
    return mat3(          //
        m.xx, m.yx, m.zx, //
        m.xy, m.yy, m.zy, //
        m.xz, m.yz, m.zz  //
    );
}

#define M3_2X2MINOR(c0, c1, r0, r1) (m.v[c0 * 3 + r0] * m.v[c1 * 3 + r1] - m.v[c0 * 3 + r1] * m.v[c1 * 3 + r0])

float determinant(const mat3& m) {
    return m.v[0] * (m.v[4] * m.v[8] - m.v[5] * m.v[7]) //
        - m.v[3] * (m.v[1] * m.v[8] - m.v[2] * m.v[7])  //
        + m.v[6] * (m.v[1] * m.v[5] - m.v[2] * m.v[4]); //
}

mat3 adjugate(const mat3& m) {
    mat3 c;

    c.v[0] = M3_2X2MINOR(1, 2, 1, 2);
    c.v[1] = -M3_2X2MINOR(1, 2, 0, 2);
    c.v[2] = M3_2X2MINOR(1, 2, 0, 1);

    c.v[3] = -M3_2X2MINOR(0, 2, 1, 2);
    c.v[4] = M3_2X2MINOR(0, 2, 0, 2);
    c.v[5] = -M3_2X2MINOR(0, 2, 0, 1);

    c.v[6] = M3_2X2MINOR(0, 1, 1, 2);
    c.v[7] = -M3_2X2MINOR(0, 1, 0, 2);
    c.v[8] = M3_2X2MINOR(0, 1, 0, 1);

    return transposed(c);
}

mat3 inverse(const mat3& m) {
    float det = determinant(m);
    if (fabsf(det) < MAT3_EPSILON) {
        printf("WARNING: Trying to invert a matrix with zero determinant\n");
        return mat3();
    }
    return adjugate(m) * (1.0f / det);
}

void invert(mat3& m) {
    float det = determinant(m);
    if (fabsf(det) < MAT3_EPSILON) {
        printf("WARNING: Trying to invert a matrix with zero determinant\n");
        m = mat3();
        return;
    }
    m = adjugate(m) * (1.0f / det);
}

mat3 mat3_identity() {
    return mat3(          //
        1.0f, 0.0f, 0.0f, //
        0.0f, 1.0f, 0.0f, //
        0.0f, 0.0f, 1.0f  //
    );
}

auto mat3_rotate(f32 theta) -> mat3 {
    return mat3(                    //
        cos(theta), sin(theta), 0,  //
        -sin(theta), cos(theta), 0, //
        0, 0, 1                     //
    );
}

auto mat3_scale(vec2 scale) -> mat3 {
    return mat3(       //
        scale.x, 0, 0, //
        0, scale.y, 0, //
        0, 0, 1        //
    );
}

#undef M3D
#undef M3V3D
#undef M3SWAP
#undef M3_2X2MINOR

#include "doctest.h"

#include <math/mat3.h>

#include <cmath>

// Helpers
static mat3 make_mat3_123() {
    // Column-major: col0=(1,2,3), col1=(4,5,6), col2=(7,8,9)
    return mat3(1, 2, 3, 4, 5, 6, 7, 8, 9);
}

// ---- Construction --------------------------------------------------------

TEST_CASE("mat3: default constructor is identity") {
    mat3 m;
    CHECK(m.xx == doctest::Approx(1));
    CHECK(m.xy == doctest::Approx(0));
    CHECK(m.xz == doctest::Approx(0));
    CHECK(m.yx == doctest::Approx(0));
    CHECK(m.yy == doctest::Approx(1));
    CHECK(m.yz == doctest::Approx(0));
    CHECK(m.zx == doctest::Approx(0));
    CHECK(m.zy == doctest::Approx(0));
    CHECK(m.zz == doctest::Approx(1));
}

TEST_CASE("mat3: 9-float constructor") {
    mat3 m(1, 2, 3, 4, 5, 6, 7, 8, 9);
    CHECK(m.v[0] == doctest::Approx(1));
    CHECK(m.v[1] == doctest::Approx(2));
    CHECK(m.v[2] == doctest::Approx(3));
    CHECK(m.v[3] == doctest::Approx(4));
    CHECK(m.v[4] == doctest::Approx(5));
    CHECK(m.v[5] == doctest::Approx(6));
    CHECK(m.v[6] == doctest::Approx(7));
    CHECK(m.v[7] == doctest::Approx(8));
    CHECK(m.v[8] == doctest::Approx(9));
}

TEST_CASE("mat3: float-array constructor") {
    f32 data[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    mat3 m(data);
    for (int i = 0; i < 9; ++i) {
        CHECK(m.v[i] == doctest::Approx(data[i]));
    }
}

// ---- Equality ------------------------------------------------------------

TEST_CASE("mat3: operator== identical matrices") {
    mat3 a = make_mat3_123();
    mat3 b = make_mat3_123();
    CHECK(a == b);
}

TEST_CASE("mat3: operator!= different matrices") {
    mat3 a;
    mat3 b(2, 0, 0, 0, 2, 0, 0, 0, 2);
    CHECK(a != b);
}

TEST_CASE("mat3: operator== within epsilon") {
    mat3 a;
    mat3 b(1.0f + 5e-8f, 0, 0, 0, 1, 0, 0, 0, 1);
    CHECK(a == b); // difference < MAT3_EPSILON
}

// ---- Arithmetic ----------------------------------------------------------

TEST_CASE("mat3: scalar multiply") {
    mat3 m(1, 2, 3, 4, 5, 6, 7, 8, 9);
    mat3 r = m * 2.0f;
    for (int i = 0; i < 9; ++i) {
        CHECK(r.v[i] == doctest::Approx(m.v[i] * 2.0f));
    }
}

TEST_CASE("mat3: matrix addition") {
    mat3 a(1, 0, 0, 0, 1, 0, 0, 0, 1);
    mat3 b(2, 0, 0, 0, 2, 0, 0, 0, 2);
    mat3 r = a + b;
    CHECK(r.v[0] == doctest::Approx(3));
    CHECK(r.v[4] == doctest::Approx(3));
    CHECK(r.v[8] == doctest::Approx(3));
}

TEST_CASE("mat3: identity * matrix = matrix") {
    mat3 id;
    mat3 m = make_mat3_123();
    mat3 r = id * m;
    CHECK(r == m);
}

TEST_CASE("mat3: matrix * identity = matrix") {
    mat3 id;
    mat3 m = make_mat3_123();
    mat3 r = m * id;
    CHECK(r == m);
}

TEST_CASE("mat3: matrix * vector with identity") {
    mat3 id;
    vec3 v(3, 4, 5);
    vec3 r = id * v;
    CHECK(r.x == doctest::Approx(3));
    CHECK(r.y == doctest::Approx(4));
    CHECK(r.z == doctest::Approx(5));
}

// ---- Transpose -----------------------------------------------------------

TEST_CASE("mat3: transposed returns correct result") {
    // col0=(1,2,3), col1=(4,5,6), col2=(7,8,9)
    // After transpose: col0=(1,4,7), col1=(2,5,8), col2=(3,6,9)
    mat3 m(1, 2, 3, 4, 5, 6, 7, 8, 9);
    mat3 t = transposed(m);
    CHECK(t.xx == doctest::Approx(1));
    CHECK(t.xy == doctest::Approx(4));
    CHECK(t.xz == doctest::Approx(7));
    CHECK(t.yx == doctest::Approx(2));
    CHECK(t.yy == doctest::Approx(5));
    CHECK(t.yz == doctest::Approx(8));
    CHECK(t.zx == doctest::Approx(3));
    CHECK(t.zy == doctest::Approx(6));
    CHECK(t.zz == doctest::Approx(9));
}

TEST_CASE("mat3: transpose in-place") {
    mat3 m(1, 2, 3, 4, 5, 6, 7, 8, 9);
    transpose(m);
    mat3 expected = transposed(mat3(1, 2, 3, 4, 5, 6, 7, 8, 9));
    CHECK(m == expected);
}

TEST_CASE("mat3: double transpose is identity") {
    mat3 original = make_mat3_123();
    mat3 m = original;
    transpose(m);
    transpose(m);
    CHECK(m == original);
}

// ---- Determinant ---------------------------------------------------------

TEST_CASE("mat3: determinant of identity is 1") {
    mat3 id;
    CHECK(determinant(id) == doctest::Approx(1.0f));
}

TEST_CASE("mat3: determinant of singular matrix is 0") {
    // Rows (1,2,3), (4,5,6), (7,8,9) are linearly dependent
    mat3 m(1, 2, 3, 4, 5, 6, 7, 8, 9);
    CHECK(determinant(m) == doctest::Approx(0.0f).epsilon(1e-4));
}

TEST_CASE("mat3: determinant of diagonal matrix") {
    // Diagonal (1, 2, 3) — det = 1*2*3 = 6
    mat3 m(1, 0, 0, 0, 2, 0, 0, 0, 3);
    CHECK(determinant(m) == doctest::Approx(6.0f));
}

// ---- Inverse -------------------------------------------------------------

TEST_CASE("mat3: M * inverse(M) = identity") {
    mat3 m(1, 0, 0, 0, 2, 0, 0, 0, 3);
    mat3 inv = inverse(m);
    mat3 result = m * inv;
    CHECK(result == mat3()); // identity
}

TEST_CASE("mat3: invert in-place") {
    mat3 m(1, 0, 0, 0, 2, 0, 0, 0, 3);
    mat3 original = m;
    invert(m);
    mat3 result = original * m;
    CHECK(result == mat3()); // identity
}

TEST_CASE("mat3: inverse of identity is identity") {
    mat3 id;
    mat3 inv = inverse(id);
    CHECK(inv == id);
}

// ---- Factory functions ---------------------------------------------------

TEST_CASE("mat3: mat3_identity returns identity") {
    mat3 id = mat3_identity();
    CHECK(id == mat3());
}

TEST_CASE("mat3: mat3_rotate 90 degrees transforms (1,0,0) to (0,1,0)") {
    const f32 pi_half = 3.14159265358979323846f / 2.0f;
    mat3 r = mat3_rotate(pi_half);
    vec3 v(1, 0, 0);
    vec3 result = r * v;
    CHECK(result.x == doctest::Approx(0).epsilon(1e-5));
    CHECK(result.y == doctest::Approx(1).epsilon(1e-5));
    CHECK(result.z == doctest::Approx(0).epsilon(1e-5));
}

TEST_CASE("mat3: mat3_rotate 0 degrees is identity") {
    mat3 r = mat3_rotate(0.0f);
    CHECK(r == mat3());
}

TEST_CASE("mat3: mat3_scale scales a vector") {
    mat3 s = mat3_scale(vec2(2.0f, 3.0f));
    vec3 v(1, 1, 0);
    vec3 result = s * v;
    CHECK(result.x == doctest::Approx(2.0f));
    CHECK(result.y == doctest::Approx(3.0f));
    CHECK(result.z == doctest::Approx(0.0f));
}

TEST_CASE("mat3: mat3_scale by 1 is identity") {
    mat3 s = mat3_scale(vec2(1.0f, 1.0f));
    CHECK(s == mat3());
}

#include "doctest.h"

#include <math/mat2.h>

#include <cmath>

// NOTE: operator== in mat2.cpp has inverted logic (uses < instead of >
// for the epsilon check), so tests that rely on it will currently fail.
// Individual element checks are used below to avoid depending on it.

// ---- Construction --------------------------------------------------------

TEST_CASE("mat2: default constructor is identity") {
    mat2 m;
    CHECK(m.xx == doctest::Approx(1));
    CHECK(m.xy == doctest::Approx(0));
    CHECK(m.yx == doctest::Approx(0));
    CHECK(m.yy == doctest::Approx(1));
}

TEST_CASE("mat2: 4-float constructor") {
    mat2 m(1, 2, 3, 4);
    CHECK(m.xx == doctest::Approx(1));
    CHECK(m.xy == doctest::Approx(2));
    CHECK(m.yx == doctest::Approx(3));
    CHECK(m.yy == doctest::Approx(4));
}

TEST_CASE("mat2: float-array constructor") {
    f32 data[4] = { 1, 2, 3, 4 };
    mat2 m(data);
    for (int i = 0; i < 4; ++i) {
        CHECK(m.v[i] == doctest::Approx(data[i]));
    }
}

TEST_CASE("mat2: rNcM aliases match v array") {
    mat2 m(1, 2, 3, 4);
    CHECK(m.r0c0 == doctest::Approx(m.v[0]));
    CHECK(m.r0c1 == doctest::Approx(m.v[1]));
    CHECK(m.r1c0 == doctest::Approx(m.v[2]));
    CHECK(m.r1c1 == doctest::Approx(m.v[3]));
}

// ---- Equality ------------------------------------------------------------

TEST_CASE("mat2: operator== identical matrices") {
    mat2 a(1, 2, 3, 4);
    mat2 b(1, 2, 3, 4);
    CHECK(a == b);
}

TEST_CASE("mat2: operator!= different matrices") {
    mat2 a;
    mat2 b(2, 0, 0, 2);
    CHECK(a != b);
}

// ---- Arithmetic ----------------------------------------------------------

TEST_CASE("mat2: scalar multiply") {
    mat2 m(1, 2, 3, 4);
    mat2 r = m * 3.0f;
    CHECK(r.xx == doctest::Approx(3));
    CHECK(r.xy == doctest::Approx(6));
    CHECK(r.yx == doctest::Approx(9));
    CHECK(r.yy == doctest::Approx(12));
}

TEST_CASE("mat2: matrix addition") {
    mat2 a(1, 2, 3, 4);
    mat2 b(4, 3, 2, 1);
    mat2 r = a + b;
    CHECK(r.xx == doctest::Approx(5));
    CHECK(r.xy == doctest::Approx(5));
    CHECK(r.yx == doctest::Approx(5));
    CHECK(r.yy == doctest::Approx(5));
}

TEST_CASE("mat2: identity * matrix = matrix (row-major)") {
    mat2 id = mat2();
    mat2 m(1, 2, 3, 4);
    mat2 r = id * m;
    CHECK(r.xx == doctest::Approx(m.xx));
    CHECK(r.xy == doctest::Approx(m.xy));
    CHECK(r.yx == doctest::Approx(m.yx));
    CHECK(r.yy == doctest::Approx(m.yy));
}

TEST_CASE("mat2: matrix * identity = matrix (row-major)") {
    mat2 m(1, 2, 3, 4);
    mat2 id;
    mat2 r = m * id;
    CHECK(r.xx == doctest::Approx(m.xx));
    CHECK(r.xy == doctest::Approx(m.xy));
    CHECK(r.yx == doctest::Approx(m.yx));
    CHECK(r.yy == doctest::Approx(m.yy));
}

TEST_CASE("mat2: matrix multiply") {
    // row-major: result[r][c] = sum_k a[r][k] * b[k][c]
    mat2 a(1, 2, 3, 4);
    mat2 b(5, 6, 7, 8);
    mat2 r = a * b;
    CHECK(r.xx == doctest::Approx(1 * 5 + 2 * 7)); // 19
    CHECK(r.xy == doctest::Approx(1 * 6 + 2 * 8)); // 22
    CHECK(r.yx == doctest::Approx(3 * 5 + 4 * 7)); // 43
    CHECK(r.yy == doctest::Approx(3 * 6 + 4 * 8)); // 50
}

// ---- Vector transform ----------------------------------------------------

TEST_CASE("mat2: vec * identity = vec (row vector on left)") {
    mat2 id;
    vec2 v(3, 4);
    vec2 r = v * id;
    CHECK(r.x == doctest::Approx(3));
    CHECK(r.y == doctest::Approx(4));
}

TEST_CASE("mat2: vec * mat (row-major transform)") {
    // result.x = v.x*xx + v.y*yx
    // result.y = v.x*xy + v.y*yy
    mat2 m(1, 2, 3, 4);
    vec2 v(1, 1);
    vec2 r = v * m;
    CHECK(r.x == doctest::Approx(1 * 1 + 1 * 3)); // 4
    CHECK(r.y == doctest::Approx(1 * 2 + 1 * 4)); // 6
}

// ---- Transpose -----------------------------------------------------------

TEST_CASE("mat2: transposed swaps off-diagonal elements") {
    mat2 m(1, 2, 3, 4);
    mat2 t = transposed(m);
    CHECK(t.xx == doctest::Approx(1));
    CHECK(t.xy == doctest::Approx(3));
    CHECK(t.yx == doctest::Approx(2));
    CHECK(t.yy == doctest::Approx(4));
}

TEST_CASE("mat2: transpose in-place") {
    mat2 m(1, 2, 3, 4);
    transpose(m);
    CHECK(m.xx == doctest::Approx(1));
    CHECK(m.xy == doctest::Approx(3));
    CHECK(m.yx == doctest::Approx(2));
    CHECK(m.yy == doctest::Approx(4));
}

TEST_CASE("mat2: double transpose returns original") {
    mat2 original(1, 2, 3, 4);
    mat2 m = original;
    transpose(m);
    transpose(m);
    CHECK(m.xx == doctest::Approx(original.xx));
    CHECK(m.xy == doctest::Approx(original.xy));
    CHECK(m.yx == doctest::Approx(original.yx));
    CHECK(m.yy == doctest::Approx(original.yy));
}

// ---- Determinant ---------------------------------------------------------

TEST_CASE("mat2: determinant of identity is 1") {
    CHECK(determinant(mat2()) == doctest::Approx(1.0f));
}

TEST_CASE("mat2: determinant of singular matrix is 0") {
    mat2 m(1, 2, 2, 4); // row1 = 2*row0
    CHECK(determinant(m) == doctest::Approx(0.0f));
}

TEST_CASE("mat2: determinant value") {
    mat2 m(3, 8, 4, 6);
    // det = 3*6 - 8*4 = 18 - 32 = -14
    CHECK(determinant(m) == doctest::Approx(-14.0f));
}

// ---- Inverse -------------------------------------------------------------

TEST_CASE("mat2: inverse of identity is identity") {
    mat2 inv = inverse(mat2());
    CHECK(inv.xx == doctest::Approx(1));
    CHECK(inv.xy == doctest::Approx(0));
    CHECK(inv.yx == doctest::Approx(0));
    CHECK(inv.yy == doctest::Approx(1));
}

TEST_CASE("mat2: M * inverse(M) = identity") {
    mat2 m(1, 2, 3, 4);
    mat2 inv = inverse(m);
    mat2 r = m * inv;
    CHECK(r.xx == doctest::Approx(1).epsilon(1e-5));
    CHECK(r.xy == doctest::Approx(0).epsilon(1e-5));
    CHECK(r.yx == doctest::Approx(0).epsilon(1e-5));
    CHECK(r.yy == doctest::Approx(1).epsilon(1e-5));
}

TEST_CASE("mat2: invert in-place") {
    mat2 original(1, 2, 3, 4);
    mat2 m = original;
    invert(m);
    mat2 r = original * m;
    CHECK(r.xx == doctest::Approx(1).epsilon(1e-5));
    CHECK(r.xy == doctest::Approx(0).epsilon(1e-5));
    CHECK(r.yx == doctest::Approx(0).epsilon(1e-5));
    CHECK(r.yy == doctest::Approx(1).epsilon(1e-5));
}

// ---- Factory functions ---------------------------------------------------

TEST_CASE("mat2: mat2_rotate 0 degrees is identity") {
    mat2 r = mat2_rotate(0.0f);
    CHECK(r.xx == doctest::Approx(1));
    CHECK(r.xy == doctest::Approx(0));
    CHECK(r.yx == doctest::Approx(0));
    CHECK(r.yy == doctest::Approx(1));
}

TEST_CASE("mat2: mat2_rotate 90 degrees transforms (1,0) to (0,1)") {
    // Row-major: vec * mat, so rotation matrix rows hold basis vectors
    const f32 pi_half = 3.14159265358979323846f / 2.0f;
    mat2 r = mat2_rotate(pi_half);
    vec2 result = vec2(1, 0) * r;
    CHECK(result.x == doctest::Approx(0).epsilon(1e-5));
    CHECK(result.y == doctest::Approx(1).epsilon(1e-5));
}

TEST_CASE("mat2: mat2_scale by (2, 3)") {
    mat2 s = mat2_scale(vec2(2.0f, 3.0f));
    vec2 result = vec2(5, 7) * s;
    CHECK(result.x == doctest::Approx(10.0f));
    CHECK(result.y == doctest::Approx(21.0f));
}

TEST_CASE("mat2: mat2_scale by 1 is identity") {
    mat2 s = mat2_scale(vec2(1.0f, 1.0f));
    CHECK(s.xx == doctest::Approx(1));
    CHECK(s.xy == doctest::Approx(0));
    CHECK(s.yx == doctest::Approx(0));
    CHECK(s.yy == doctest::Approx(1));
}

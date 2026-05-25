#include "doctest.h"

#include <math/mat4.h>

// ---- Construction --------------------------------------------------------

TEST_CASE("mat4: default constructor is identity") {
    mat4 m;
    CHECK(m.v[0] == doctest::Approx(1));
    CHECK(m.v[1] == doctest::Approx(0));
    CHECK(m.v[2] == doctest::Approx(0));
    CHECK(m.v[3] == doctest::Approx(0));

    CHECK(m.v[4] == doctest::Approx(0));
    CHECK(m.v[5] == doctest::Approx(1));
    CHECK(m.v[6] == doctest::Approx(0));
    CHECK(m.v[7] == doctest::Approx(0));

    CHECK(m.v[8] == doctest::Approx(0));
    CHECK(m.v[9] == doctest::Approx(0));
    CHECK(m.v[10] == doctest::Approx(1));
    CHECK(m.v[11] == doctest::Approx(0));

    CHECK(m.v[12] == doctest::Approx(0));
    CHECK(m.v[13] == doctest::Approx(0));
    CHECK(m.v[14] == doctest::Approx(0));
    CHECK(m.v[15] == doctest::Approx(1));
}

// Row-major: 16-float constructor fills row by row.
// mat4(r0c0, r0c1, r0c2, r0c3,
//      r1c0, r1c1, r1c2, r1c3,
//      r2c0, r2c1, r2c2, r2c3,
//      r3c0, r3c1, r3c2, r3c3)
TEST_CASE("mat4: 16-float constructor fills v[] in row-major order") {
    mat4 m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    CHECK(m.v[0] == doctest::Approx(1));
    CHECK(m.v[1] == doctest::Approx(2));
    CHECK(m.v[2] == doctest::Approx(3));
    CHECK(m.v[3] == doctest::Approx(4));
    CHECK(m.v[4] == doctest::Approx(5));
    CHECK(m.v[5] == doctest::Approx(6));
    CHECK(m.v[6] == doctest::Approx(7));
    CHECK(m.v[7] == doctest::Approx(8));
    CHECK(m.v[8] == doctest::Approx(9));
    CHECK(m.v[9] == doctest::Approx(10));
    CHECK(m.v[10] == doctest::Approx(11));
    CHECK(m.v[11] == doctest::Approx(12));
    CHECK(m.v[12] == doctest::Approx(13));
    CHECK(m.v[13] == doctest::Approx(14));
    CHECK(m.v[14] == doctest::Approx(15));
    CHECK(m.v[15] == doctest::Approx(16));
}

TEST_CASE("mat4: float-array constructor copies in order") {
    f32 data[16] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
    mat4 m(data);
    for (int i = 0; i < 16; ++i) {
        CHECK(m.v[i] == doctest::Approx(data[i]));
    }
}

// In row-major storage rNcM aliases must match row-first indexing:
//   r0c0=v[0]  r0c1=v[1]  r0c2=v[2]  r0c3=v[3]
//   r1c0=v[4]  r1c1=v[5]  ...
TEST_CASE("mat4: rNcM aliases match row-major v[] layout") {
    mat4 m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    CHECK(m.r0c0 == doctest::Approx(1));
    CHECK(m.r0c1 == doctest::Approx(2));
    CHECK(m.r0c2 == doctest::Approx(3));
    CHECK(m.r0c3 == doctest::Approx(4));
    CHECK(m.r1c0 == doctest::Approx(5));
    CHECK(m.r1c1 == doctest::Approx(6));
    CHECK(m.r1c2 == doctest::Approx(7));
    CHECK(m.r1c3 == doctest::Approx(8));
    CHECK(m.r2c0 == doctest::Approx(9));
    CHECK(m.r2c1 == doctest::Approx(10));
    CHECK(m.r2c2 == doctest::Approx(11));
    CHECK(m.r2c3 == doctest::Approx(12));
    CHECK(m.r3c0 == doctest::Approx(13));
    CHECK(m.r3c1 == doctest::Approx(14));
    CHECK(m.r3c2 == doctest::Approx(15));
    CHECK(m.r3c3 == doctest::Approx(16));
}

// ---- Equality ------------------------------------------------------------

TEST_CASE("mat4: operator== identical matrices") {
    mat4 a(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    mat4 b(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    CHECK(a == b);
}

TEST_CASE("mat4: operator!= different matrices") {
    mat4 a;
    mat4 b(2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2);
    CHECK(a != b);
}

// ---- Arithmetic ----------------------------------------------------------

TEST_CASE("mat4: scalar multiply") {
    mat4 m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    mat4 r = m * 2.0f;
    for (int i = 0; i < 16; ++i) {
        CHECK(r.v[i] == doctest::Approx(m.v[i] * 2.0f));
    }
}

TEST_CASE("mat4: matrix addition") {
    mat4 a(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
    mat4 b(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
    mat4 r = a + b;
    CHECK(r.v[0] == doctest::Approx(2));
    CHECK(r.v[5] == doctest::Approx(2));
    CHECK(r.v[10] == doctest::Approx(2));
    CHECK(r.v[15] == doctest::Approx(2));
    CHECK(r.v[1] == doctest::Approx(0));
}

TEST_CASE("mat4: identity * M = M") {
    mat4 id;
    mat4 m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    mat4 r = id * m;
    for (int i = 0; i < 16; ++i) {
        CHECK(r.v[i] == doctest::Approx(m.v[i]).epsilon(1e-5));
    }
}

TEST_CASE("mat4: M * identity = M") {
    mat4 m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    mat4 id;
    mat4 r = m * id;
    for (int i = 0; i < 16; ++i) {
        CHECK(r.v[i] == doctest::Approx(m.v[i]).epsilon(1e-5));
    }
}

// Row-major: result[i][j] = sum_k A[i][k] * B[k][j]
//
// A = [1 2 0 0]   B = [5 6 0 0]
//     [3 4 0 0]       [7 8 0 0]
//     [0 0 1 0]       [0 0 1 0]
//     [0 0 0 1]       [0 0 0 1]
//
// (A*B)[0][0] = 1*5+2*7 = 19
// (A*B)[0][1] = 1*6+2*8 = 22
// (A*B)[1][0] = 3*5+4*7 = 43
// (A*B)[1][1] = 3*6+4*8 = 50
TEST_CASE("mat4: matrix multiply (row-major)") {
    mat4 a(         //
        1, 2, 0, 0, //
        3, 4, 0, 0, //
        0, 0, 1, 0, //
        0, 0, 0, 1  //
    );
    mat4 b(         //
        5, 6, 0, 0, //
        7, 8, 0, 0, //
        0, 0, 1, 0, //
        0, 0, 0, 1  //
    );
    mat4 r = a * b;
    CHECK(r.r0c0 == doctest::Approx(19));
    CHECK(r.r0c1 == doctest::Approx(22));
    CHECK(r.r1c0 == doctest::Approx(43));
    CHECK(r.r1c1 == doctest::Approx(50));
    CHECK(r.r2c2 == doctest::Approx(1));
    CHECK(r.r3c3 == doctest::Approx(1));
}

// ---- Vector transform ----------------------------------------------------

TEST_CASE("mat4: vec4 * m uses row-major column-vector multiply") {
    mat4 m(            //
        1, 2, 3, 4,    //
        5, 6, 7, 8,    //
        9, 10, 11, 12, //
        13, 14, 15, 16 //
    );
    vec4 v(1, 2, 3, 4);
    vec4 r = v * m;
    CHECK(r.x == doctest::Approx(90));
    CHECK(r.y == doctest::Approx(100));
    CHECK(r.z == doctest::Approx(110));
    CHECK(r.w == doctest::Approx(120));
}

TEST_CASE("mat4: vec4 * identity = vec4") {
    mat4 id;
    vec4 v(1, 2, 3, 4);
    vec4 r = v * id;
    CHECK(r.x == doctest::Approx(1));
    CHECK(r.y == doctest::Approx(2));
    CHECK(r.z == doctest::Approx(3));
    CHECK(r.w == doctest::Approx(4));
}

// ---- Transpose -----------------------------------------------------------

TEST_CASE("mat4: transposed swaps off-diagonal elements") {
    mat4 m(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    mat4 t = transposed(m);
    // Diagonal unchanged
    CHECK(t.r0c0 == doctest::Approx(1));
    CHECK(t.r1c1 == doctest::Approx(6));
    CHECK(t.r2c2 == doctest::Approx(11));
    CHECK(t.r3c3 == doctest::Approx(16));
    // Off-diagonal swapped
    CHECK(t.r0c1 == doctest::Approx(5)); // was r1c0
    CHECK(t.r1c0 == doctest::Approx(2)); // was r0c1
    CHECK(t.r0c2 == doctest::Approx(9)); // was r2c0
    CHECK(t.r2c0 == doctest::Approx(3)); // was r0c2
}

TEST_CASE("mat4: transpose in-place") {
    mat4 m(1, 2, 0, 0, 3, 4, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
    transpose(m);
    CHECK(m.r0c1 == doctest::Approx(3));
    CHECK(m.r1c0 == doctest::Approx(2));
}

TEST_CASE("mat4: double transpose returns original") {
    mat4 original(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    mat4 m = original;
    transpose(m);
    transpose(m);
    for (int i = 0; i < 16; ++i) {
        CHECK(m.v[i] == doctest::Approx(original.v[i]));
    }
}

// ---- Determinant ---------------------------------------------------------

TEST_CASE("mat4: determinant of identity is 1") {
    CHECK(determinant(mat4()) == doctest::Approx(1.0f));
}

TEST_CASE("mat4: determinant of singular matrix is 0") {
    // Row 0 and row 1 are identical
    mat4 m(1, 2, 3, 4, 1, 2, 3, 4, 0, 0, 1, 0, 0, 0, 0, 1);
    CHECK(determinant(m) == doctest::Approx(0.0f).epsilon(1e-5));
}

TEST_CASE("mat4: determinant of 2x scalar-identity is 16") {
    // det(2*I) = 2^4 = 16
    mat4 m(2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 2);
    CHECK(determinant(m) == doctest::Approx(16.0f).epsilon(1e-4));
}

TEST_CASE("mat4: determinant of a 4x4 matrix is same as transpose of same matrix") {
    mat4 m(          //
        2, 1, 3, 4,  //
        0, -1, 2, 1, //
        3, 2, 0, 5,  //
        -1, 3, 2, 1  //
    );
    CHECK(determinant(m) == doctest::Approx(35.0f).epsilon(1e-4));
    mat4 trans = transposed(m);
    CHECK(determinant(trans) == doctest::Approx(35.0f).epsilon(1e-4));
}

// ---- Inverse -------------------------------------------------------------

TEST_CASE("mat4: inverse of identity is identity") {
    mat4 inv = inverse(mat4());
    CHECK(inv == mat4());
}

TEST_CASE("mat4: M * inverse(M) = identity") {
    mat4 m(1, 2, 0, 0, 3, 4, 0, 0, 0, 0, 2, 0, 0, 0, 0, 1);
    mat4 inv = inverse(m);
    mat4 r = m * inv;
    CHECK(r == mat4());
}

TEST_CASE("mat4: M * inverse(M) = identity") {
    mat4 m(1, 2, 0, 0, 3, 4, 0, 0, 0, 0, 2, 0, 0, 0, 0, 1);
    mat4 inv = inverse(m);
    vec4 point(1.0, 2.0, 3.0, 1.0);
    vec4 new_point = (point * m) * inv;
    CHECK(point == new_point);
}

TEST_CASE("mat4: invert in-place: M * invert(M) = identity") {
    mat4 original(1, 2, 0, 0, 3, 4, 0, 0, 0, 0, 2, 0, 0, 0, 0, 1);
    mat4 m = original;
    invert(m);
    mat4 r = original * m;
    CHECK(r.r0c0 == doctest::Approx(1).epsilon(1e-5));
    CHECK(r.r0c1 == doctest::Approx(0).epsilon(1e-5));
    CHECK(r.r1c0 == doctest::Approx(0).epsilon(1e-5));
    CHECK(r.r1c1 == doctest::Approx(1).epsilon(1e-5));
    CHECK(r.r2c2 == doctest::Approx(1).epsilon(1e-5));
    CHECK(r.r3c3 == doctest::Approx(1).epsilon(1e-5));
}

// ---- lookAt --------------------------------------------------------------

// Camera at (0,0,5) looking at origin with up=(0,1,0):
//   - The camera position itself maps to (0,0,0) in view space.
//   - The world origin maps to (0,0,-5) in view space (5 units in front,
//     along the -Z forward axis in a right-handed system).
TEST_CASE("mat4: lookAt maps camera position to origin") {
    vec3 pos(0, 0, 5);
    vec3 target(0, 0, 0);
    vec3 up(0, 1, 0);
    vec4 r = vec4(0, 0, 0, 1.0) * lookAt(pos, target, up);
    CHECK(r.x == doctest::Approx(0).epsilon(1e-5));
    CHECK(r.y == doctest::Approx(0).epsilon(1e-5));
    CHECK(r.z == doctest::Approx(-5).epsilon(1e-5));
}

TEST_CASE("mat4: lookAt maps a point directly in front of camera along -Z") {
    // Camera at origin looking along -Z (target at (0,0,-1)), up=+Y.
    // A world point at (0,0,-3) should be at (0,0,-3) in view space
    // (already on the camera axis, 3 units ahead).
    vec3 pos(0, 0, 0);
    vec3 target(0, 0, -1);
    vec3 up(0, 1, 0);
    vec4 r = vec4(0, 0, -3, 0) * lookAt(pos, target, up);
    CHECK(r.x == doctest::Approx(0).epsilon(1e-5));
    CHECK(r.y == doctest::Approx(0).epsilon(1e-5));
    CHECK(r.z == doctest::Approx(-3).epsilon(1e-5));
}

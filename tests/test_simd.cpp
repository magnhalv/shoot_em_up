#include "doctest.h"

#include <math/simd.hpp>

TEST_CASE("SIMD") {
    f32 x = 0.0f;
    f32 y = 0.0f;
    __m256 a_vpos4 = _mm256_setr_ps(
        (f32)x, (f32)y,
        (f32)x+1, (f32)y,
        (f32)x+2, (f32)y,
        (f32)x+3, (f32)y
    );

    __m256 a_v8 = _mm256_setr_ps( 0.0f, 1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f);
    __m256 b_v8 = _mm256_setr_ps( 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f);
 // __m256 b_v8 = _mm256_setr_ps( 0.0f, 9.0f, 20.0f, 33.0f, 48.0f, 65.0f, 84.0f, 105.0f);

    {
      __m256 result1_v8 = _mm256_shuffle_ps(a_v8, b_v8, 0b11111111);
      f32 result1[8];
      _mm256_store_ps(result1, result1_v8);

      REQUIRE_EQ(result1[0], 3.0);
      REQUIRE_EQ(result1[1], 3.0);
      REQUIRE_EQ(result1[2], 11.0);
      REQUIRE_EQ(result1[3], 11.0);
      REQUIRE_EQ(result1[4], 7.0);
      REQUIRE_EQ(result1[5], 7.0);
      REQUIRE_EQ(result1[6], 15.0);
      REQUIRE_EQ(result1[7], 15.0);
    }

    {
      __m256 result1_v8 = _mm256_shuffle_ps(a_v8, b_v8, SHUFFLE_MASK(0, 3, 3, 3));
      f32 result1[8];
      _mm256_store_ps(result1, result1_v8);

      REQUIRE_EQ(result1[0], 0.0);
      REQUIRE_EQ(result1[1], 3.0);
      REQUIRE_EQ(result1[2], 11.0);
      REQUIRE_EQ(result1[3], 11.0);
      REQUIRE_EQ(result1[4], 4.0);
      REQUIRE_EQ(result1[5], 7.0);
      REQUIRE_EQ(result1[6], 15.0);
      REQUIRE_EQ(result1[7], 15.0);
    }
    {
      __m256 ax = _mm256_setr_ps( 0.0f, 1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f);
      __m256 ay = _mm256_setr_ps( 8.0f, 9.0f,  10.0f,  11.0f,  12.0f,  13.0f,  14.0f,  15.0f);


      __m256 bx = _mm256_setr_ps( 16.0f, 17.0f,  18.0f,  19.0f,  20.0f,  21.0f,  22.0f,  23.0f);
      __m256 by = _mm256_setr_ps( 24.0f, 25.0f,  26.0f,  27.0f,  28.0f,  29.0f,  30.0f,  31.0f);

        __m256 r_v8 = dot(ax, ay, bx, by);
        f32 r[8];
        _mm256_store_ps(r, r_v8);
        REQUIRE_EQ(r[0], 192.0f);
        REQUIRE_EQ(r[1], 242.0f);
        REQUIRE_EQ(r[2], 296.0f);
        REQUIRE_EQ(r[3], 354.0f);
        REQUIRE_EQ(r[4], 416.0f);
        REQUIRE_EQ(r[5], 482.0f);
        REQUIRE_EQ(r[6], 552.0f);
        REQUIRE_EQ(r[7], 626.0f);
    }

    

}

#pragma once

#include <platform/types.h>

// Reverse of _MM_SHUFFLE_, as I find that more intuitive
#define SHUFFLE_MASK(a,b,c,d) \
    (((d) << 6) | ((c) << 4) | ((b) << 2) | (a))


auto inline dot(__m256 ax, __m256 ay, __m256 bx, __m256 by) -> __m256  {
  __m256 x = _mm256_mul_ps(ax, bx);
  __m256 y = _mm256_mul_ps(ay, by);
  return _mm256_add_ps(x, y);
}

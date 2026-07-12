#pragma once

#include <stdio.h>
#include <stdlib.h>

#include "types.hpp"

// Defined in platform.hpp; kept separate to avoid a circular include with core/array.hpp.
void platform_assert_print_stack_trace();

#define Assert(expr)                                                              \
    if (!(expr)) {                                                                \
        platform_assert_print_stack_trace();                                      \
        printf("Assertion failed: %s, file %s(%d)\n", #expr, __FILE__, __LINE__); \
        abort();                                                                  \
    }

#define AssertLessThan_i32(a, b)                                                                            \
    i32 _a = (a);                                                                                            \
    i32 _b = (b);                                                                                            \
    if (!(_a < _b)) {                                                                                          \
        platform_assert_print_stack_trace();                                                                \
        printf("Assert failed: %s < %s, %d !< %d, file %s, line %d\n", #a, #b, _a, _b, __FILE__, __LINE__); \
        abort();                                                                                            \
    }
#define AssertEqual_i32(a, b)                                                                                    \
    do {                                                                                                         \
        i32 _a = (a);                                                                                            \
        i32 _b = (b);                                                                                            \
        if (_a != _b) {                                                                                          \
            printf("Assert failed: %s == %s, %d != %d, file %s, line %d\n", #a, #b, _a, _b, __FILE__, __LINE__); \
            abort();                                                                                             \
        }                                                                                                        \
    } while (0)

#define AssertEqual_i64(a, b)                                                                                        \
    do {                                                                                                             \
        i64 _a = (a);                                                                                                \
        i64 _b = (b);                                                                                                \
        if (_a != _b) {                                                                                              \
            printf("Assert failed: %s == %s, %lld != %lld, file %s, line %d\n", #a, #b, _a, _b, __FILE__, __LINE__); \
            abort();                                                                                                 \
        }                                                                                                            \
    } while (0)

#define AssertEqual_f32(a, b)                                                                                                   \
    do {                                                                                                                        \
        f32 _a = (a);                                                                                                           \
        f32 _b = (b);                                                                                                           \
        f32 _eps = F32_EPSILON;                                                                                                 \
        f32 _diff = _a - _b;                                                                                                    \
        if (_diff < 0.0f)                                                                                                       \
            _diff = -_diff;                                                                                                     \
        if (_diff > _eps) {                                                                                                     \
            printf("Assert failed: %s == %s, %f != %f (eps %f), file %s, line %d\n", #a, #b, _a, _b, _eps, __FILE__, __LINE__); \
            abort();                                                                                                            \
        }                                                                                                                       \
    } while (0)

#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase \
    default: {             \
        InvalidCodePath;   \
    } break

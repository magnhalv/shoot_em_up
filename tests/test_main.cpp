#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

#include <core/lib.hpp>

int main(int argc, char** argv) {
    initialize_core_lib();
    return doctest::Context(argc, argv).run();
}

#include <core/lib.cpp>
#include <math/mat2.cpp>
#include <math/mat3.cpp>
#include <math/mat4.cpp>
#include <math/vec2.cpp>
#include <math/vec3.cpp>

#include "memory_arena_test.cpp"
#include "structs/test_swap_back_list.cpp"
#include "test_mat2.cpp"
#include "test_mat3.cpp"
#include "test_mat4.cpp"
#include "test_render_line_bresenham.cpp"
#include "test_renderer.cpp"
#include "test_simd.cpp"
#include "test_sort.cpp"
#include "test_string8.cpp"

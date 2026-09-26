#include "doctest.h"
#include "util.hpp"

#include <core/array.hpp>

TEST_CASE_FIXTURE(SingleArenaFixture, "test_array_span") {

    Array<i32> array = Array<i32>::create(6, arena);
    array[0] = 1;
    array[1] = 2;
    array[2] = 3;
    array[3] = 4;
    array[4] = 5;
    array[5] = 6;

    Array<i32> span_array = span(array, 0, 0);
    REQUIRE_EQ(span_array.count(), 0);

    Array<i32> span2 = span(array, 3, array.count());
    REQUIRE_EQ(span_array.count(), 0);
}

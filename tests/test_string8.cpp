#include "doctest.h"

#include <core/string8.hpp>

#include "util.h"

TEST_CASE_FIXTURE(SingleArenaFixture, "string8: concat") {
    string8 a = "test";
    string8 b = "_string";

    string8 c = string8_concat(a, b, &arena);

    CHECK_EQ(c[0], 't');
    CHECK_EQ(c[1], 'e');
    CHECK_EQ(c[2], 's');
    CHECK_EQ(c[3], 't');
    CHECK_EQ(c[4], '_');
    CHECK_EQ(c[5], 's');
    CHECK_EQ(c[6], 't');
    CHECK_EQ(c[7], 'r');
    CHECK_EQ(c[8], 'i');
    CHECK_EQ(c[9], 'n');
    CHECK_EQ(c[10], 'g');
}

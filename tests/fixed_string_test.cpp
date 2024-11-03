#include <fixed_string.h>

#include "util.h"

TEST_CASE_FIXTURE(SingleArenaFixture, "basic case") {
    auto empty = FStr::create("", arena);
    REQUIRE_EQ(split(empty, ' ', arena).size(), 0);

    auto single = FStr::create("test", arena);
    auto single_result = split(single, ' ', arena);
    REQUIRE_EQ(single_result.size(), 1);
    REQUIRE_EQ(single_result[0], "test");

    auto two = FStr::create("test foo", arena);
    auto two_result = split(two, ' ', arena);
    REQUIRE_EQ(two_result.size(), 2);
    REQUIRE_EQ(two_result[0], "test");
    REQUIRE_EQ(two_result[1], "foo");

    auto two_with_start_and_end = FStr::create(" test foo ", arena);
    REQUIRE_EQ(split(two_with_start_and_end, ' ', arena).size(), 2);

    auto two_with_excess = FStr::create("  test   foo   ", arena);
    REQUIRE_EQ(split(two_with_excess, ' ', arena).size(), 2);
}
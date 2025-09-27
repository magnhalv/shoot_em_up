#include <engine/fixed_string.h>
#include <engine/structs/swap_back_list.h>

#include "../util.h"

TEST_CASE_FIXTURE(SingleArenaFixture, "Removing an element sets that index to the last element.") {
    SwapBackList<i32> list;

    list.init(arena, 6);
    list.push(4);
    list.push(3);
    list.push(2);
    list.push(1);

    REQUIRE_EQ(list.size(), 4);
    REQUIRE_EQ(list[0], 4);
    REQUIRE_EQ(list[3], 1);
    list.remove(0);
    REQUIRE_EQ(list.size(), 3);
    REQUIRE_EQ(list[0], 1);
}

TEST_CASE_FIXTURE(SingleArenaFixture, "Removing last element only reduces the size.") {
    SwapBackList<i32> list;

    list.init(arena, 6);
    list.push(4);
    list.push(3);
    list.push(2);
    list.push(1);

    REQUIRE_EQ(list.size(), 4);
    list.remove(list.size() - 1);

    REQUIRE_EQ(list.size(), 3);
    REQUIRE_EQ(list[0], 4);
    REQUIRE_EQ(list[1], 3);
    REQUIRE_EQ(list[2], 2);
}

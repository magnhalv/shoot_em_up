#include "doctest.h"

#include <core/sort.hpp>

#include "util.h"

TEST_CASE_FIXTURE(SingleArenaFixture, "merge_sort: empty range leaves data untouched") {
    i32 list[1] = { 123 }; // untouched sentinel

    merge_sort(list, 0, &arena);

    CHECK_EQ(list[0], 123);
}

TEST_CASE_FIXTURE(SingleArenaFixture, "merge_sort: single element remains unchanged") {
    i32 list[1] = { 42 };

    merge_sort(list, 1, &arena);

    CHECK_EQ(list[0], 42);
}

TEST_CASE_FIXTURE(SingleArenaFixture, "merge_sort: two elements already sorted remain sorted") {
    i32 list[2] = { -1, 7 };

    merge_sort(list, 2, &arena);

    CHECK_EQ(list[0], -1);
    CHECK_EQ(list[1], 7);
}

TEST_CASE_FIXTURE(SingleArenaFixture, "merge_sort: two elements reversed get sorted") {
    i32 list[2] = { 7, -1 };

    merge_sort(list, 2, &arena);

    CHECK_EQ(list[0], -1);
    CHECK_EQ(list[1], 7);
}

TEST_CASE_FIXTURE(SingleArenaFixture, "merge_sort: three elements get sorted") {
    i32 list[3] = { 5, 7, -1 };

    merge_sort(list, 3, &arena);

    CHECK_EQ(list[0], -1);
    CHECK_EQ(list[1], 5);
    CHECK_EQ(list[2], 7);
}

TEST_CASE_FIXTURE(SingleArenaFixture, "merge_sort: all-equal values remain all equal") {
    i32 list[10] = { 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 };

    merge_sort(list, 10, &arena);

    for (i32 i = 0; i < 10; i++)
        CHECK_EQ(list[i], 5);
}

TEST_CASE_FIXTURE(SingleArenaFixture, "merge_sort: already sorted list including negatives remains sorted") {
    i32 list[10] = { -9, -3, -1, 0, 2, 4, 5, 7, 7, 10 };

    merge_sort(list, 10, &arena);

    CHECK_EQ(list[0], -9);
    CHECK_EQ(list[1], -3);
    CHECK_EQ(list[2], -1);
    CHECK_EQ(list[3], 0);
    CHECK_EQ(list[4], 2);
    CHECK_EQ(list[5], 4);
    CHECK_EQ(list[6], 5);
    CHECK_EQ(list[7], 7);
    CHECK_EQ(list[8], 7);
    CHECK_EQ(list[9], 10);
}

TEST_CASE_FIXTURE(SingleArenaFixture, "merge_sort: reverse-sorted list gets sorted") {
    i32 list[10] = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };

    merge_sort(list, 10, &arena);

    CHECK_EQ(list[0], 0);
    CHECK_EQ(list[1], 1);
    CHECK_EQ(list[2], 2);
    CHECK_EQ(list[3], 3);
    CHECK_EQ(list[4], 4);
    CHECK_EQ(list[5], 5);
    CHECK_EQ(list[6], 6);
    CHECK_EQ(list[7], 7);
    CHECK_EQ(list[8], 8);
    CHECK_EQ(list[9], 9);
}

TEST_CASE_FIXTURE(SingleArenaFixture, "merge_sort: many scattered duplicates get grouped correctly") {
    i32 list[10] = { 2, 1, 2, 0, 2, 1, 0, 2, 1, 0 };

    merge_sort(list, 10, &arena);

    CHECK_EQ(list[0], 0);
    CHECK_EQ(list[1], 0);
    CHECK_EQ(list[2], 0);
    CHECK_EQ(list[3], 1);
    CHECK_EQ(list[4], 1);
    CHECK_EQ(list[5], 1);
    CHECK_EQ(list[6], 2);
    CHECK_EQ(list[7], 2);
    CHECK_EQ(list[8], 2);
    CHECK_EQ(list[9], 2);
}

TEST_CASE_FIXTURE(SingleArenaFixture, "merge_sort: handles INT32 min/max and duplicates correctly") {
    i32 list[10] = { 2147483647, -2147483647 - 1, 0, 1, -1, 2147483647, -2147483647 - 1, 2, -2, 0 };

    merge_sort(list, 10, &arena);

    CHECK_EQ(list[0], -2147483647 - 1);
    CHECK_EQ(list[1], -2147483647 - 1);
    CHECK_EQ(list[2], -2);
    CHECK_EQ(list[3], -1);
    CHECK_EQ(list[4], 0);
    CHECK_EQ(list[5], 0);
    CHECK_EQ(list[6], 1);
    CHECK_EQ(list[7], 2);
    CHECK_EQ(list[8], 2147483647);
    CHECK_EQ(list[9], 2147483647);
}

TEST_CASE_FIXTURE(SingleArenaFixture, "merge_sort: odd count with duplicates gets sorted") {
    i32 list[9] = { 3, 1, 4, 1, 5, 9, 2, 6, 5 };

    merge_sort(list, 9, &arena);

    CHECK_EQ(list[0], 1);
    CHECK_EQ(list[1], 1);
    CHECK_EQ(list[2], 2);
    CHECK_EQ(list[3], 3);
    CHECK_EQ(list[4], 4);
    CHECK_EQ(list[5], 5);
    CHECK_EQ(list[6], 5);
    CHECK_EQ(list[7], 6);
    CHECK_EQ(list[8], 9);
}

TEST_CASE_FIXTURE(SingleArenaFixture, "merge_sort: crossing-halves pattern sorts correctly") {
    i32 list[10] = { 0, 2, 4, 6, 8, 1, 3, 5, 7, 9 };

    merge_sort(list, 10, &arena);

    CHECK_EQ(list[0], 0);
    CHECK_EQ(list[1], 1);
    CHECK_EQ(list[2], 2);
    CHECK_EQ(list[3], 3);
    CHECK_EQ(list[4], 4);
    CHECK_EQ(list[5], 5);
    CHECK_EQ(list[6], 6);
    CHECK_EQ(list[7], 7);
    CHECK_EQ(list[8], 8);
    CHECK_EQ(list[9], 9);
}

TEST_CASE_FIXTURE(SingleArenaFixture, "merge_sort_indices: ") {
    i32 list[4] = { 3, 2, 7, 1 };

    i32* indices = merge_sort_indices(list, 4, &arena);

    CHECK_EQ(indices[0], 3);
    CHECK_EQ(indices[1], 1);
    CHECK_EQ(indices[2], 0);
    CHECK_EQ(indices[3], 2);
}

TEST_CASE_FIXTURE(SingleArenaFixture, "merge_sort_indices: ") {
    i32 list[4] = { 3, 2, 7, 1 };

    i32* indices = merge_sort_indices(list, 4, &arena);

    CHECK_EQ(indices[0], 3);
    CHECK_EQ(indices[1], 1);
    CHECK_EQ(indices[2], 0);
    CHECK_EQ(indices[3], 2);
}

TEST_CASE_FIXTURE(SingleArenaFixture, "merge_sort_indices: stable") {
    i32 list[4] = { 1, 1, 1, 1 };

    i32* indices = merge_sort_indices(list, 4, &arena);

    CHECK_EQ(indices[0], 0);
    CHECK_EQ(indices[1], 1);
    CHECK_EQ(indices[2], 2);
    CHECK_EQ(indices[3], 3);
}

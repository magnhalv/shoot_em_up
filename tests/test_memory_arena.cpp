#include "core/base_thread_context.hpp"
#include "doctest.h"

#include <core/logger.hpp>
#include <core/memory.hpp>
#include <core/memory_arena.hpp>

#include "util.hpp"

TEST_CASE_FIXTURE(SingleArenaFixture, "filling the arena") {
    // Two, because we have a sentinel at the beginning, also make space for padding byte;
    arena.allocate(512, { .alignment = 1, .flags = ArenaPushFlag_ClearToZero });
    arena.allocate(512, { .alignment = 1, .flags = ArenaPushFlag_ClearToZero });
    REQUIRE(arena.capacity == 1024);
    REQUIRE(arena.size == 1024);
}

TEST_CASE_FIXTURE(SingleArenaFixture, "check_alignment") {
    {
        u8* array = PushArray(&arena, 1, u8);
        bool is_aligned = ((uintptr_t)array & 3) == 0;
        REQUIRE(is_aligned);
    }
    {
        u8* array = PushArray(&arena, 1, u8);
        bool is_aligned = ((uintptr_t)array & 3) == 0;
        REQUIRE(is_aligned);
    }
    {
        u8* array = PushArray(&arena, 1, u8, { .alignment = 1, .flags = ArenaPushFlag_ClearToZero });
        u8* array2 = PushArray(&arena, 1, u8, { .alignment = 16, .flags = ArenaPushFlag_ClearToZero });
        REQUIRE(is_aligned(array2, 16));
    }
}

TEST_CASE_FIXTURE(SingleArenaFixture, "temp_arenas") {
    auto array = Array<u8>::create(4, arena);
    REQUIRE_EQ(arena.size, 4);
    array[0] = 'd';
    array[1] = 'e';
    array[2] = 'a';
    array[3] = 'd';

    {
        Temp temp = temp_begin(&arena);
        REQUIRE_EQ(arena.temp_count, 1);
        auto temp_arr = Array<u8>::create(5, temp.arena);
        REQUIRE_EQ(arena.size, 9);
        temp_arr[0] = 'r';
        temp_arr[1] = 'e';
        temp_arr[2] = 'a';
        temp_arr[3] = 'd';
        temp_arr[4] = 'y';
        temp_end(temp);
    }
    {
        Temp temp = temp_begin(&arena);
        REQUIRE_EQ(arena.temp_count, 1);
        auto temp_arr = Array<u8>::create(4, temp.arena);
        REQUIRE_EQ(arena.size, 8);
        temp_arr[0] = 'b';
        temp_arr[1] = 'e';
        temp_arr[2] = 'e';
        temp_arr[3] = 'f';
        temp_end(temp);
    }

    REQUIRE_EQ(arena.size, 4);
    REQUIRE_EQ(arena.temp_count, 0);

    // 'ready' was overwritten by the second temp array, but not the last entry
    REQUIRE_EQ(*(array.data() + 4), 'b');
    REQUIRE_EQ(*(array.data() + 5), 'e');
    REQUIRE_EQ(*(array.data() + 6), 'e');
    REQUIRE_EQ(*(array.data() + 7), 'f');
    REQUIRE_EQ(*(array.data() + 8), 'y');
}

TEST_CASE_FIXTURE(TwoArenaFixture, "scratch_arenas") {
    ThreadContext context;
    for (u32 i = 0; i < arenas.count(); i++) {
        context.arenas[i] = &arenas[i];
    }
    set_tctx_selected(&context);

    Temp temp = scratch_begin();

    scratch_end(temp);
}

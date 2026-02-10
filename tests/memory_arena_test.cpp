#include "doctest.h"

#include <core/logger.h>
#include <core/memory_arena.h>

#include "util.h"

TEST_CASE_FIXTURE(SingleArenaFixture, "filling the arena") {
    // Two, because we have a sentinel at the beginning, also make space for padding byte;
    arena.allocate(512 - 2 * sizeof(MemorySentinel) - 1, { .alignment = 1, .flags = ArenaPushFlag_ClearToZero });
    arena.allocate(512 - sizeof(MemorySentinel) - 1, { .alignment = 1, .flags = ArenaPushFlag_ClearToZero });
    REQUIRE(arena.m_capacity == 1024);
    REQUIRE(arena.m_size == 1024);
}

TEST_CASE_FIXTURE(SingleArenaFixture, "allocate more than size (due to arena guard)") {
    CHECK_CRASH(arena.allocate(default_size), "Failed to allocate 1044 bytes. Only 1008 remaining.");
}

TEST_CASE_FIXTURE(SingleArenaFixture, "integrity should work with no allocations made") {
    arena.check_integrity(); // should succeed
}

TEST_CASE_FIXTURE(SingleArenaFixture, "failed integrity: not initialized") {
    MemoryArena not_initialized;
    CHECK_CRASH(not_initialized.check_integrity(), "MemoryArena: is not initialized. Always initialize arenas before use!");
}

TEST_CASE_FIXTURE(SingleArenaFixture, "failed integrity simple") {
    arena.allocate(512);
    memset(arena.m_memory, 0, 1024);
    CHECK_CRASH(arena.check_integrity(), "MemoryArena: integrity check failed at guard index 0");
}

TEST_CASE_FIXTURE(SingleArenaFixture, "failed integrity array") {
    u8* array = static_cast<u8*>(arena.allocate(512));
    for (auto i = 0; i < 513; i++) {
        array[i] = 5;
    }
    CHECK_CRASH(arena.check_integrity(), "MemoryArena: integrity check failed at guard index 1");
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
        bool is_aligned = ((uintptr_t)array2 & 16) == 0;
        REQUIRE(is_aligned);
        printf("%p\n", (void*)array);
        printf("%p\n", (void*)array2);
    }
}

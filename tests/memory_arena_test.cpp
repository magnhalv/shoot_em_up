#include <cstring>

#include "doctest.h"

#include <logger.h>
#include <memory_arena.h>

#include "util.h"

TEST_CASE_FIXTURE(SingleArenaFixture, "filling the arena") {
  // Two, because we have a guard at the beginning.
  arena.allocate(512 - 2 * sizeof(ArenaGuard));
  arena.allocate(512 - sizeof(ArenaGuard));
  REQUIRE(arena.size == 1024);
  REQUIRE(arena.used == 1024);
}

TEST_CASE_FIXTURE(SingleArenaFixture, "allocate more than size (due to arena guard)") {
  CHECK_CRASH(arena.allocate(default_size), "Failed to allocate 1024 bytes. Only 1008 remaining.");
}

TEST_CASE_FIXTURE(SingleArenaFixture, "integrity should work with no allocations made") {
  arena.check_integrity(); // should succeed
}

TEST_CASE_FIXTURE(SingleArenaFixture, "failed integrity: not initialized") {
  MemoryArena not_initialized;
  CHECK_CRASH(not_initialized.check_integrity(), "MemoryArena: is not initialized. Always initialize arenas before use!");
}

TEST_CASE_FIXTURE(SingleArenaFixture, "failed integrity simple") {
  // Two, because we have a guard at the beginning.
  arena.allocate(512);
  memset(arena.memory, 0, 1024);
  CHECK_CRASH(arena.check_integrity(), "MemoryArena: integrity check failed at guard index 0");
}

TEST_CASE_FIXTURE(SingleArenaFixture, "failed integrity array") {
  u8* array = static_cast<u8*>(arena.allocate(512));
  for (auto i = 0; i < 513; i++) {
    array[i] = 5;
  }
  CHECK_CRASH(arena.check_integrity(), "MemoryArena: integrity check failed at guard index 1");
}

TEST_CASE_FIXTURE(SingleArenaFixture, "failed integrity array") {
  u8* array = static_cast<u8*>(arena.allocate(256));
  for (auto i = 0; i < 256; i++) {
    array[i] = 5;
  }
  // Should work
  arena.check_integrity();
  arena.extend(static_cast<void*>(array), 256);
  // We've extended the array to be 256 + 256 bytes
  for (auto i = 0; i < 512; i++) {
    array[i] = 5;
  }
  arena.check_integrity();
}

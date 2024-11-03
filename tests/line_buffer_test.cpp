#include "doctest.h"

#include "util.h"
#include <options.hpp>

TEST_CASE_FIXTURE(SingleArenaFixture, "LineBuffer: Add lines and extend") {
  LineBuffer buffer(10, &arena, 512);
  buffer.push_line("A line");
  buffer.push_line("B line");
  buffer.extend_last(" extended with C");
  buffer.push_line("D line");

  REQUIRE_EQ(buffer._lines[0], "A line");
  REQUIRE_EQ(buffer._lines[1], "B line extended with C");
  REQUIRE_EQ(buffer._lines[2], "D line");
}

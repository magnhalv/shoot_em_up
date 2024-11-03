#include <growable_string.h>

#include "util.h"

TEST_CASE_FIXTURE(SingleArenaFixture, "basic case") {
  auto basic = GStr::create("test", 10, arena);
  REQUIRE_EQ(basic, "test");

  basic.push(" foo");
  REQUIRE_EQ(basic, "test foo");

  basic.pop(5);
  REQUIRE_EQ(basic, "tes");

  basic.pop(5);
  REQUIRE_EQ(basic, "");
}

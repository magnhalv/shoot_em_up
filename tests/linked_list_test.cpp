#include <fixed_string.h>
#include <linked_list.h>

#include "util.h"

TEST_CASE_FIXTURE(SingleArenaFixture, "basic case with fixed strings") {
  LinkedList<FStr> list;
  FStr test_strings[2] = { FStr::create("test", arena), FStr::create("test2", arena) };
  list.insert(test_strings[0], arena);
  list.insert(test_strings[1], arena);

  REQUIRE_EQ(list.size(), 2);

  int index = 0;
  for (auto entry : list) {
    REQUIRE_EQ(entry, test_strings[index]);
    index++;
  }
}


#include "doctest.h"

#include "util.h"
#include <array.h>
#include <cli/echo.h>
#include <fixed_string.h>

TEST_CASE_FIXTURE(TransientFixture, "echo: too many arguments") {
  Array<FStr> args;
  args.init(local, 2);
  args[0] = FStr::create("Test", local);
  args[1] = FStr::create("Test2", local);
  LinkedListBuffer llb;
  llb.init(512, local);
  hm::cli::echo::handle(args, llb);
  REQUIRE_EQ(llb.list[0], "Echo only accepts a single argument.");
}

TEST_CASE_FIXTURE(TransientFixture, "echo: happy path") {
  Array<FStr> args;
  args.init(local, 1);
  args[0] = FStr::create("Test", local);
  LinkedListBuffer llb;
  llb.init(512, local);
  hm::cli::echo::handle(args, llb);
  REQUIRE_EQ(llb.list[0], "Test");
}

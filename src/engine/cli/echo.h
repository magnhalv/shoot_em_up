#pragma once

#include "../list.h"
#include "cli_app.h"

namespace hm::cli::echo {
auto inline handle(Array<FStr>& args, LinkedListBuffer& buf) -> void {
  if (args.size() != 1) {
    buf.add("Echo only accepts a single argument.");
    return;
  } else {
    buf.add(args[0].data());
  }
}

auto inline handle_autocomplete(Array<FStr>& args, LinkedListBuffer& buf, MemoryArena& arena) -> List<FStr> {
  List<FStr> list;
  list.init(arena, 0);
  return list;
}

auto inline reg(List<CliApp>& apps, MemoryArena& arena) -> void {
  CliApp echo{ .name = FStr::create("echo", arena), .handle = &handle, .autocomplete = &handle_autocomplete };
  apps.push(echo);
}

} // namespace hm::cli::echo

#pragma once

#include "../fixed_string.h"
#include "../linked_list_buffer.h"
#include "../list.h"

typedef void (*cli_command_handle)(Array<FStr>& args, LinkedListBuffer& buf);
typedef List<FStr> (*cli_command_autocomplete)(Array<FStr>& args, LinkedListBuffer& buf, MemoryArena& arena);

struct CliApp {
  FStr name;
  cli_command_handle handle;
  cli_command_autocomplete autocomplete;
};

#pragma once

#include "../engine.h"
#include "../options.hpp"
#include "cli_app.h"
#include "math/mat4.h"
#include <cstdio>

enum class CliOptionType { INT, BOOL, STRING };

struct CliOption {
  FStr long_name;
  i32 short_name;
  CliOptionType type;
  void* value;
  FStr description;
};

struct CliCommand {
  FStr long_name;
  i32 short_name;
  Array<CliOption> options;
};

struct CliAppTemp {
  FStr name;
  Array<CliCommand> commands;
};

namespace hm::cli::options {

auto inline get_app(MemoryArena& arena) -> CliAppTemp {
  CliAppTemp app;
  app.name = FStr::create("options", arena);

  app.commands.init(arena, 2);

  auto& antialias = app.commands[0];
  antialias.long_name = FStr::create("antialias", arena);
  antialias.short_name = 'a';
  antialias.options.init(arena, 2);
  antialias.options[0] = CliOption{
    .long_name = FStr::create("enable", arena), //
    .short_name = 'e',                          //
    .type = CliOptionType::BOOL,                //
    .value = nullptr,
    .description = FStr::create("enable antialiasing", arena) //
  };

  antialias.options[1] = CliOption{
    .long_name = FStr::create("disable", arena), //
    .short_name = 'd',                           //
    .type = CliOptionType::BOOL,                 //
    .value = nullptr,
    .description = FStr::create("disable antialiasing", arena) //
  };

  auto& grid = app.commands[1];
  grid.long_name = FStr::create("grid", arena);
  grid.short_name = 'g';
  grid.options.init(arena, 2);
  grid.options[0] = CliOption{
    .long_name = FStr::create("enable", arena), //
    .short_name = 'e',                          //
    .type = CliOptionType::BOOL,                //
    .value = nullptr,
    .description = FStr::create("enable grid", arena) //
  };

  grid.options[1] = CliOption{
    .long_name = FStr::create("disable", arena), //
    .short_name = 'd',                           //
    .type = CliOptionType::BOOL,                 //
    .value = nullptr,
    .description = FStr::create("disable grid", arena) //
  };

  return app;
}

auto inline parse(Array<FStr>& args, CliAppTemp app, MemoryArena& arena) -> CliCommand* {
  printf("%s\n", args[0].data());
  HM_ASSERT(args[0] == app.name);

  CliCommand* command = nullptr;
  for (auto& c : app.commands) {
    auto is_command = c.short_name == args[1].data()[0] || args[1] == c.long_name;
    if (is_command) {
      command = &c;
      break;
    }
  }

  if (command == nullptr) {
    // TODO: Handle not found
    return nullptr;
  }

  auto i = 2;
  while (i < args.size()) {

    CliOption* option = nullptr;
    for (auto& o : command->options) {
      auto is_option = o.short_name == args[1].data()[0] || args[1] == o.long_name;
      if (is_option) {
        option = &o;
        break;
      }
    }

    switch (option->type) {

    case CliOptionType::BOOL: {
      option->value = allocate<bool>(arena);
      *reinterpret_cast<bool*>(option->value) = true;
    } break;
    case CliOptionType::INT:
    case CliOptionType::STRING: break;
    }

    i++;
  }
  return command;
}

auto inline get_bool(const char* name, CliCommand& command) -> bool {
  for (auto o : command.options) {
    if (o.long_name == name) {
      return *reinterpret_cast<bool*>(o.value);
    }
  }
  return false;
}

auto inline handle_options(Array<FStr>& args, LinkedListBuffer& buf) -> void {
  printf("Create app\n");
  auto app = get_app(*g_transient);

  const char* help_message = "options [antialias, grid] on|off";

  printf("Parse app\n");
  auto command = parse(args, app, *g_transient);

  printf("Do stuff\n");
  if (command->long_name == "antialias") {
    if (get_bool("enable", *command)) {
      graphics_options->anti_aliasing = true;
    }
    if (get_bool("disable", *command)) {
      graphics_options->anti_aliasing = true;
    }
  }
  if (command->long_name == "grid") {
    if (get_bool("enable", *command)) {
      graphics_options->anti_aliasing = true;
    }
    if (get_bool("disable", *command)) {
      graphics_options->anti_aliasing = true;
    }
  } else {
    buf.add(help_message);
  }

  save_to_file(graphics_options);
}

auto inline handle_autocomplete(Array<FStr>& args, LinkedListBuffer& buf, MemoryArena& arena) -> List<FStr> {
  List<FStr> result;
  result.init(arena, 5);

  if (args.size() == 0) {
    buf.add("grid");
  }
  // TODO: Must manipulate command string
  return result;
}

auto inline reg(List<CliApp>& apps, MemoryArena& arena) -> void {
  CliApp options{ .name = FStr::create("options", arena), .handle = &handle_options, .autocomplete = &handle_autocomplete };
  apps.push(options);
}

} // namespace hm::cli::options

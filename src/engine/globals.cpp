#include "engine/globals.hpp"

Platform* g_platform = nullptr;
Options* g_graphics_options = nullptr;

auto load(Platform* in_platform) -> void {
  g_platform = in_platform;
}

auto load(Options* options) -> void {
  g_graphics_options = options;
}

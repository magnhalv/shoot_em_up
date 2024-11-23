#pragma once

#include <engine/options.hpp>
#include <platform/platform.h>

extern Platform* g_platform;
extern Options* g_graphics_options;

void load(Platform* platform);
void load(Options* platform);

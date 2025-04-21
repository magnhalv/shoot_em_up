#pragma once

#include <platform/platform.h>

#include <engine/options.hpp>

extern Platform* g_platform;
extern Options* g_graphics_options;

void load(Platform* platform);
void load(Options* platform);

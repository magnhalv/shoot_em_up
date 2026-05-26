#pragma once

#define NOMINMAX
#include <windows.h>

#include <renderers/renderer.hpp>

struct Win32RenderContext {
    HWND window;
    HDC hdc; // To I need this?
    HGLRC hglrc;
};

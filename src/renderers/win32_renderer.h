#pragma once

#include <windows.h>

#include <renderers/renderer.h>

struct Win32RenderContext {
    HWND window;
    HDC hdc; // To I need this?
    HGLRC hglrc;
};

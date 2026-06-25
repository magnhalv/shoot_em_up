#pragma once

#define NOMINMAX
#include <windows.h>

#include <renderers/renderer.hpp>

struct Win32RenderContext {
    HWND window;
    HDC hdc; // To I need this?
    HGLRC hglrc;
};

static const char* win32_renderer_exports[] = {
    "win32_renderer_init",
    "win32_renderer_add_texture",
    "win32_renderer_render",
    "win32_renderer_begin_frame",
    "win32_renderer_end_frame",
    "win32_renderer_delete_context",
    "win32_renderer_create_framebuffer",
    "win32_renderer_apply_framebuffer",
    "win32_renderer_get_color",
};

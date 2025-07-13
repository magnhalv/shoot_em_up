#include <windows.h>

#include "renderer.hpp"

#define RENDERER_INIT(name) void name(HWND window)
typedef RENDERER_INIT(renderer_init_fn);

struct RendererPlatform {
    renderer_init_fn* init;

    HMODULE handle;
    bool is_valid;
    FILETIME last_loaded_dll_write_time = { 0, 0 };

    RenderApi api;
};

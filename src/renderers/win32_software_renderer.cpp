#include <platform/platform.h>

#include <engine/hm_assert.h>
#include <math/math.h>

#include <renderers/renderer.h>
#include <renderers/win32_renderer.h>

const u32 MaxNumTextures = 5;

struct WindowDimension {
    i32 width;
    i32 height;
};

struct OffscreenBuffer {
    BITMAPINFO Info;
    void* memory;
    i32 memorySize;
    i32 width;
    i32 height;
    i32 bytes_per_pixel;
    i32 pitch;
};

// TODO: This should probably go to an arena
struct SWRendererState {
    OffscreenBuffer global_offscreen_buffer;
};

static SWRendererState state = { 0 };

static WindowDimension get_window_dimension(HWND window) {
    WindowDimension result;
    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.width = clientRect.right - clientRect.left;
    result.height = clientRect.bottom - clientRect.top;
    return result;
}

static void render_buffer_in_window(HDC deviceContext, int window_width, int window_height, OffscreenBuffer buffer,
    int x, int y, int width, int height) {
    int offset = 0;
    StretchDIBits(                   //
        deviceContext,               //
        0, 0,                        //
        buffer.width, buffer.height, //
        0, 0,                        //
        buffer.width, buffer.height, //
        buffer.memory, &buffer.Info, //
        DIB_RGB_COLORS, SRCCOPY      //
    );
}

static void resize_dib_section(OffscreenBuffer* buffer, int width, int height) {
    // TODO: Bulletproof this
    // Maybe don't free first, free after, then free first if that fails.
    if (buffer->memory) {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;

    buffer->Info.bmiHeader.biSize = sizeof(buffer->Info.bmiHeader);
    buffer->Info.bmiHeader.biWidth = buffer->width;
    buffer->Info.bmiHeader.biHeight = -buffer->height;
    buffer->Info.bmiHeader.biPlanes = 1;
    buffer->Info.bmiHeader.biBitCount = 32;
    buffer->Info.bmiHeader.biCompression = BI_RGB;

    buffer->bytes_per_pixel = 4;
    buffer->memorySize = buffer->bytes_per_pixel * (buffer->width * buffer->height);
    buffer->memory = VirtualAlloc(0, buffer->memorySize, MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = buffer->width * buffer->bytes_per_pixel;
}

i32 round_real32_to_int32(f32 real) {
    return (i32)roundf(real);
}

static void draw_rectangle(OffscreenBuffer* buffer, vec2 v_min, vec2 v_max, f32 r, f32 g, f32 b) {
    i32 min_x = round_real32_to_int32(v_min.x);
    i32 max_x = round_real32_to_int32(v_max.x);
    i32 min_y = round_real32_to_int32(v_min.y);
    i32 max_y = round_real32_to_int32(v_max.y);
    if (min_x < 0) {
        min_x = 0;
    }
    if (max_x > buffer->width) {
        max_x = buffer->width;
    }

    if (min_y < 0) {
        min_y = 0;
    }
    if (max_y > buffer->height) {
        max_y = buffer->height;
    }

    // Flip coordinate system
    min_y = buffer->height - min_y;
    max_y = buffer->height - max_y;
    auto temp = min_y;
    min_y = max_y;
    max_y = temp;

    u32 color = (round_real32_to_int32(r * 255.0f) << 16) | (round_real32_to_int32(g * 255.0f) << 8) |
        (round_real32_to_int32(b * 255.0f));

    u8* row = ((u8*)buffer->memory + (min_y * buffer->pitch) + (min_x * buffer->bytes_per_pixel));
    for (int y = min_y; y < max_y; y++) {
        u32* pixel = (u32*)row;
        for (int x = min_x; x < max_x; x++) {
            *pixel++ = color;
        }
        row += buffer->pitch;
    }
}

static auto clear(i32 client_width, i32 client_height, vec4 color) {
    vec2 min = vec2(0, 0);
    vec2 max = vec2(client_width, client_height);
    draw_rectangle(                     //
        &state.global_offscreen_buffer, //
        min, max,                       //
        color.r, color.b, color.g       //
    );
}

static auto draw_quad(Quadrilateral quad, vec2 local_origin, vec2 offset, vec2 x_axis, vec2 y_axis, vec4 color,
    i32 screen_width, i32 screen_height) {
    vec2 bl = quad.bl - local_origin;
    vec2 tl = quad.tl - local_origin;
    vec2 tr = quad.tr - local_origin;
    vec2 br = quad.br - local_origin;

    bl = bl.x * x_axis + bl.y * y_axis;
    tl = tl.x * x_axis + tl.y * y_axis;
    tr = tr.x * x_axis + tr.y * y_axis;
    br = br.x * x_axis + br.y * y_axis;

    bl = bl + local_origin;
    tl = tl + local_origin;
    tr = tr + local_origin;
    br = br + local_origin;

    bl = bl + offset;
    tl = tl + offset;
    tr = tr + offset;
    br = br + offset;

    draw_rectangle(                     //
        &state.global_offscreen_buffer, //
        bl, tr,                         //
        1.0, 0, 0                       //
    );
}

static auto draw_bitmap(Quadrilateral quad, vec2 local_origin, vec2 offset, vec2 x_axis, vec2 y_axis, vec4 color,
    u32 bitmap_handle, i32 screen_width, i32 screen_height) {
    printf("Draw bitmap\n");
    vec2 bl = quad.bl - local_origin;
    vec2 tr = quad.tr - local_origin;

    bl = bl.x * x_axis + bl.y * y_axis;
    tr = tr.x * x_axis + tr.y * y_axis;

    bl = bl + local_origin;
    tr = tr + local_origin;

    bl = bl + offset;
    tr = tr + offset;

    f32 min_y = hm::min(bl.y, tr.y);
    f32 max_y = hm::max(bl.y, tr.y);

    f32 min_x = hm::min(bl.x, tr.x);
    f32 max_x = hm::max(bl.x, tr.x);

    vec2 min{ min_x, min_y };
    vec2 max{ max_x, max_y };

    printf("--------\n");

    draw_rectangle(                     //
        &state.global_offscreen_buffer, //
        min, max,                       //
        1.0, 0, 0                       //
    );
}

extern "C" __declspec(dllexport) RENDERER_INIT(win32_renderer_init) {
    printf("Init software renderer...\n");

    // TODO: Get dimensions as input
    resize_dib_section(&state.global_offscreen_buffer, 960, 540);

    printf("Software renderer ready to go.\n");
}

extern "C" __declspec(dllexport) RENDERER_ADD_TEXTURE(win32_renderer_add_texture) {
    return 0;
}

extern "C" __declspec(dllexport) RENDERER_RENDER(win32_renderer_render) {
    for (u32 base_address = 0; base_address < group->push_buffer_size;) {
        RenderGroupEntryHeader* header = (RenderGroupEntryHeader*)(group->push_buffer + base_address);
        base_address += sizeof(RenderGroupEntryHeader);

        void* data = (u8*)header + sizeof(*header);
        switch (header->type) {
        case RenderCommands_RenderEntryClear: {
            RenderEntryClear* entry = (RenderEntryClear*)data;
            clear(group->screen_width, group->screen_height, entry->color);
            base_address += sizeof(*entry);
        } break;
        case RenderCommands_RenderEntryQuadrilateral: {
            auto* entry = (RenderEntryQuadrilateral*)data;
            draw_quad(entry->quad, entry->local_origin, entry->offset, entry->basis.x, entry->basis.y, entry->color,
                group->screen_width, group->screen_height);

            base_address += sizeof(*entry);
        } break;
        case RenderCommands_RenderEntryBitmap: {
            auto* entry = (RenderEntryBitmap*)data;
            draw_bitmap(entry->quad, entry->local_origin, entry->offset, entry->basis.x, entry->basis.y, entry->color,
                entry->bitmap_handle, group->screen_width, group->screen_height);

            base_address += sizeof(*entry);
        } break;
        default: InvalidCodePath;
        }
    }
}

extern "C" __declspec(dllexport) RENDERER_END_FRAME(win32_renderer_end_frame) {
    Win32RenderContext* win32_context = (Win32RenderContext*)context;
    // draw_rectangle(&global_offscreen_buffer, vec2(5, 5), vec2(100, 100), 1.0, 0.0, 0.0);
    HDC device_context = GetDC(win32_context->window);
    i32 width = state.global_offscreen_buffer.width;
    i32 height = state.global_offscreen_buffer.height;
    render_buffer_in_window(           //
        device_context,                //
        width, height,                 //
        state.global_offscreen_buffer, //
        0, 0,                          //
        width, height                  //
    );
    ReleaseDC(win32_context->window, device_context);
    printf("---- END FRAME ---- \n");
}

extern "C" __declspec(dllexport) RENDERER_DELETE_CONTEXT(win32_renderer_delete_context) {
}

// TODO: Not sure if I need this
// case WM_PAINT: {
//     PAINTSTRUCT paint;
//     HDC deviceContext = BeginPaint(window, &paint);
//     int x = paint.rcPaint.left;
//     int y = paint.rcPaint.top;
//     int width = paint.rcPaint.right - x;
//     int height = paint.rcPaint.bottom - y;
//     local_persist DWORD color = WHITENESS;
//     PatBlt(deviceContext, x, y, width, height, color);
//
//     Win32_WindowDimension dimension = win32_get_window_dimension(window);
//     PatBlt(deviceContext, 0, 0, dimension.width, dimension.height, BLACKNESS);
//     win32_DisplayBufferInWindows(deviceContext, //
//         dimension.width, dimension.height,      //
//         global_offscreen_buffer,                //
//         x, y,                                   //
//         width, height);
//
//     if (color == WHITENESS) {
//         color = BLACKNESS;
//     }
//     else {
//         color = WHITENESS;
//     }
//
//     EndPaint(window, &paint);
// } break;

#include <platform/platform.h>

#include <engine/hm_assert.h>

#include <renderers/renderer.h>
#include <renderers/win32_software_renderer.h>

const u32 MaxNumTextures = 5;

// TODO: This should probably go to an arena
typedef struct {

} SWRendererState;

static SWRendererState state = {};

static auto clear(i32 client_width, i32 client_height, vec4 color) {
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
}

static auto draw_bitmap(Quadrilateral quad, vec2 local_origin, vec2 offset, vec2 x_axis, vec2 y_axis, vec4 color,
    u32 bitmap_handle, i32 screen_width, i32 screen_height) {
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
}

extern "C" __declspec(dllexport) RENDERER_INIT(win32_renderer_init) {
    printf("Init software renderer...\n");
    printf("Software renderer ready to go.\n");

    // win32_ResizeDIBSection(&global_offscreen_buffer, client_width, client_height);
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
    // draw_rectangle(&global_offscreen_buffer, vec2(5, 5), vec2(100, 100), 1.0, 0.0, 0.0);
    // {
    //     HDC device_context = GetDC(window);
    //     win32_DisplayBufferInWindows(    //
    //         device_context,              //
    //         client_width, client_height, //
    //         global_offscreen_buffer,     //
    //         0, 0,                        //
    //         client_width, client_height  //
    //     );
    //     ReleaseDC(window, device_context);
    // }
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

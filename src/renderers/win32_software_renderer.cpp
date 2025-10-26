#include "math/mat3.h"
#include <cstdio>
#include <platform/platform.h>

#include <math/math.h>

#include <core/logger.h>
#include <core/memory.h>
#include <core/memory_arena.h>
// TODO: Move to core
#include <engine/hm_assert.h>

#include <renderers/renderer.h>
#include <renderers/win32_renderer.h>

struct WindowDimension {
    i32 width;
    i32 height;
};

struct OffscreenBuffer {
    BITMAPINFO Info;
    void* memory;
    i32 memory_size;
    i32 width;
    i32 height;
    i32 bytes_per_pixel;
    i32 pitch;
};

struct Win32Texture {
    void* data;
    i32 width;
    i32 height;
    i32 size;
    i32 pitch;
    i32 bytes_per_pixel;
};

// TODO: This should probably go to an arena
struct SWRendererState {
    OffscreenBuffer global_offscreen_buffer;
    MemoryArena permanent;
    MemoryArena transient;

    Win32Texture textures[MaxTextureId];
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
    buffer->memory_size = buffer->bytes_per_pixel * (buffer->width * buffer->height);
    buffer->memory = VirtualAlloc(0, buffer->memory_size, MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = buffer->width * buffer->bytes_per_pixel;
}

static void draw_rectangle(OffscreenBuffer* buffer, vec2 v_min, vec2 v_max, f32 r, f32 g, f32 b) {
    i32 min_x = round_f32_to_i32(v_min.x);
    i32 max_x = round_f32_to_i32(v_max.x);
    i32 min_y = round_f32_to_i32(v_min.y);
    i32 max_y = round_f32_to_i32(v_max.y);
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

    u32 color = (round_f32_to_i32(r * 255.0f) << 16) | (round_f32_to_i32(g * 255.0f) << 8) | (round_f32_to_i32(b * 255.0f));

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

static auto draw_quad(Quadrilateral quad, vec2 offset, vec2 scale, f32 rotation, vec4 color, i32 screen_width, i32 screen_height) {
    /*vec2 bl = quad.bl - local_origin;*/
    /*vec2 tl = quad.tl - local_origin;*/
    /*vec2 tr = quad.tr - local_origin;*/
    /*vec2 br = quad.br - local_origin;*/
    /**/
    /*bl = bl.x * x_axis + bl.y * y_axis;*/
    /*tl = tl.x * x_axis + tl.y * y_axis;*/
    /*tr = tr.x * x_axis + tr.y * y_axis;*/
    /*br = br.x * x_axis + br.y * y_axis;*/
    /**/
    /*bl = bl + local_origin;*/
    /*tl = tl + local_origin;*/
    /*tr = tr + local_origin;*/
    /*br = br + local_origin;*/
    /**/
    /*bl = bl + offset;*/
    /*tl = tl + offset;*/
    /*tr = tr + offset;*/
    /*br = br + offset;*/
    /**/
    /*draw_rectangle(                     //*/
    /*    &state.global_offscreen_buffer, //*/
    /*    bl, tr,                         //*/
    /*    1.0, 0, 0                       //*/
    /*);*/
}

static inline auto get_texel(Win32Texture* texture, i32 x, i32 y) -> u32 {
    x = clamp(x, 0, texture->width - 1);
    y = clamp(y, 0, texture->width - 1);
    return *((u8*)texture->data + (y * texture->pitch) + (x * texture->bytes_per_pixel));
}

static auto draw_bitmap(Quadrilateral quad, vec2 offset, vec2 scale, f32 rotation, vec4 color, BitmapId bitmap_id,
    i32 screen_width, i32 screen_height) {
    vec3 bl = vec2_to_vec3(quad.bl);
    vec3 tl = vec2_to_vec3(quad.tl);
    vec3 tr = vec2_to_vec3(quad.tr);
    vec3 br = vec2_to_vec3(quad.br);

    auto model_width = br.x - bl.x;
    auto model_height = tr.y - br.y;
    vec3 translation = vec2_to_vec3(offset);

    mat3 rot_mat = mat3_rotate(rotation);
    mat3 scale_mat = mat3_scale(scale);

    mat3 model = rot_mat * scale_mat;
    mat3 inv_model = inverse(model);

    bl = model * bl;
    tl = model * tl;
    tr = model * tr;
    br = model * br;

    bl = bl + translation;
    tl = tl + translation;
    tr = tr + translation;
    br = br + translation;

    i32 min_x = round_f32_to_i32(hm::min(bl.x, tl.x, tr.x, br.x));
    i32 max_x = round_f32_to_i32(hm::max(bl.x, tl.x, tr.x, br.x));
    i32 min_y = round_f32_to_i32(hm::min(bl.y, tl.y, tr.y, br.y));
    i32 max_y = round_f32_to_i32(hm::max(bl.y, tl.y, tr.y, br.y));

    vec3 edge1 = tl - bl;
    vec3 edge2 = tr - tl;
    vec3 edge3 = br - tr;
    vec3 edge4 = bl - br;

    OffscreenBuffer* buffer = &state.global_offscreen_buffer;

    Win32Texture* texture = &state.textures[bitmap_id.value];
    for (int y = min_y; y < max_y; y++) {
        for (int x = min_x; x < max_x; x++) {

            u32 color = 0xFFF00FFF;

            u8* dest = ((u8*)buffer->memory + (y * buffer->pitch) + (x * buffer->bytes_per_pixel));
            u32* pixel = (u32*)dest;

            vec3 screen_point{ (f32)x, (f32)y, 0.0 };
            if (screen_point.x < 0 || screen_point.x >= buffer->width) {
                continue;
            }
            if (screen_point.y < 0 || screen_point.y >= buffer->height) {
                continue;
            }

            f32 dot1 = dot(screen_point - bl, edge1);
            f32 dot2 = dot(screen_point - tl, edge2);
            f32 dot3 = dot(screen_point - tr, edge3);
            f32 dot4 = dot(screen_point - br, edge4);

            if (dot1 > 0 && dot2 > 0 && dot3 > 0 && dot4 > 0) {

                vec3 texel_point = inv_model * (screen_point - translation);
                texel_point = texel_point + (vec3(model_width, model_height, 0) * 0.5);

                f32 u = (f32)texel_point.x / ((f32)model_width - 1);
                f32 v = (f32)texel_point.y / ((f32)model_height - 1);

                // This is texel00
                i32 texel_x = round_f32_to_i32(u * (texture->width - 1));
                i32 texel_y = round_f32_to_i32(v * (texture->height - 1));

                f32 u_frac = f32_get_fraction(u);
                f32 v_frac = f32_get_fraction(v);

                // Bilinear filtering
                u32 texel00 = *((u32*)state.textures[bitmap_id.value].data + (texel_y * texture->width) + texel_x);
                u32 texel10 = *((u32*)state.textures[bitmap_id.value].data + (texel_y * texture->width) + texel_x + 1);
                u32 texel01 = *((u32*)state.textures[bitmap_id.value].data + (texel_y * texture->width) + texel_x);
                u32 texel11 = *((u32*)state.textures[bitmap_id.value].data + (texel_y * texture->width) + texel_x);

                f32 color = texel00 * (1.0 - u_frac) * (1.0 - v_frac) //
                    + texel10 * (u_frac) * (1.0 - v_frac)             //
                    + texel01 * (1.0 - u_frac) *
                        (v_frac) //

                        texel_x = clamp(texel_x, 0, texture->width - 1);
                texel_y = clamp(texel_y, 0, texture->height - 1);

                u8 red = *texel >> 0;
                u8 green = *texel >> 8;
                u8 blue = *texel >> 16;
                u8 alpha = *texel >> 24;
                u32 color = alpha << 24 | red << 16 | green << 8 | blue;
                *pixel = color;
            }
        }
    }

    /*auto width = quad.br.x - quad.bl.x;*/
    /*auto height = quad.tl.y - quad.bl.y;*/
    /*auto dim = vec2(width, height);*/
    /**/
    /*OffscreenBuffer* buffer = &state.global_offscreen_buffer;*/
    /**/
    /*Win32Texture* texture = &state.textures[bitmap_id.value];*/
    /*u64 points_inside = 0;*/
    /*for (auto y = 0; y < height; y++) {*/
    /*    for (auto x = 0; x < width; x++) {*/
    /**/
    /*        vec2 point = vec2(x, y) - local_origin;*/
    /*        point = point.x * x_axis + point.y * y_axis;*/
    /*        point = point + local_origin;*/
    /*        point = point + offset;*/
    /**/
    /*        f32 u = (f32)x / ((f32)width - 1);*/
    /*        f32 v = (f32)y / ((f32)height - 1);*/
    /**/
    /*        i32 texel_x = round_f32_to_i32(u * (texture->width - 1));*/
    /*        i32 texel_y = round_f32_to_i32(v * (texture->height - 1));*/
    /**/
    /*        texel_x = clamp(texel_x, 0, texture->width - 1);*/
    /*        texel_y = clamp(texel_y, 0, texture->height - 1);*/
    /**/
    /*        u32* texel = (u32*)texture->data + (texel_y * texture->width) + (texel_x);*/
    /*        if ((point.x >= 0 && point.x < buffer->width) && (point.y >= 0 && point.y < buffer->height)) {*/
    /*            u8* dest = ((u8*)buffer->memory + (round_f32_to_i32(point.y) * buffer->pitch) +*/
    /*                (round_f32_to_i32(point.x) * buffer->bytes_per_pixel));*/
    /*            u32* pixel = (u32*)dest;*/
    /**/
    /*            u8 a = *texel >> 24;*/
    /*            u8 b = *texel >> 16;*/
    /*            u8 g = *texel >> 8;*/
    /*            u8 r = *texel >> 0;*/
    /**/
    /*            u8 red = *texel >> 24;*/
    /*            u8 green = *texel >> 16;*/
    /*            u8 blue = *texel >> 8;*/
    /*            u8 alpha = *texel >> 0;*/
    /*            u32 color = a << 24 | r << 16 | g << 8 | b;*/
    /*            *pixel = color;*/
    /*            points_inside++;*/
    /*        }*/
    /*    }*/
    /*}*/
}

extern "C" __declspec(dllexport) RENDERER_INIT(win32_renderer_init) {
    log_info("Init software renderer...\n");

    // TODO: We should check for need of resizing on every draw call.
    resize_dib_section(&state.global_offscreen_buffer, 960, 540);

    HM_ASSERT(memory != nullptr);
    HM_ASSERT(memory->data != nullptr);
    state.permanent.init(memory->data, memory->size);
    state.transient = *state.permanent.allocate_arena(MegaBytes(10));

    log_info("Software renderer ready to go.\n");
}

extern "C" __declspec(dllexport) RENDERER_DELETE_CONTEXT(win32_renderer_delete_context) {
}

extern "C" __declspec(dllexport) RENDERER_ADD_TEXTURE(win32_renderer_add_texture) {
    if (bitmap_id.value >= MaxTextureId) {
        // TODO: Return empty texture
        log_error("Max texture id is %d, %d provided.", MaxTextureId, bitmap_id.value);
        HM_ASSERT(false);
        return false;
    }

    Win32Texture* texture = &state.textures[bitmap_id.value];
    if (texture->data == nullptr) {
        texture->height = height;
        texture->width = width;
        texture->bytes_per_pixel = 4;
        texture->size = width * height * texture->bytes_per_pixel;
        texture->pitch = width * texture->bytes_per_pixel;
        texture->data = state.permanent.allocate(texture->size);

        copy_memory(data, texture->data, texture->size);
        log_info("Texture added");
        return true;
    }
    else {
        return false;
    }
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
            draw_quad(entry->quad, entry->offset, entry->scale, entry->rotation, entry->color, group->screen_width,
                group->screen_height);

            base_address += sizeof(*entry);
        } break;
        case RenderCommands_RenderEntryBitmap: {
            auto* entry = (RenderEntryBitmap*)data;
            draw_bitmap(entry->quad, entry->offset, entry->scale, entry->rotation, entry->color, entry->bitmap_handle,
                group->screen_width, group->screen_height);

            base_address += sizeof(*entry);
        } break;
        default: InvalidCodePath;
        }
    }
}

extern "C" __declspec(dllexport) RENDERER_BEGIN_FRAME(win32_renderer_begin_frame) {
    state.transient.clear();
}

extern "C" __declspec(dllexport) RENDERER_END_FRAME(win32_renderer_end_frame) {
    Win32RenderContext* win32_context = (Win32RenderContext*)context;
    HDC device_context = GetDC(win32_context->window);

    i32 width = state.global_offscreen_buffer.width;
    i32 height = state.global_offscreen_buffer.height;
    i32 bytes_per_pixel = state.global_offscreen_buffer.bytes_per_pixel;

    // Flip the y-axis.
    u8* buffer = allocate<u8>(state.transient, state.global_offscreen_buffer.memory_size);
    for (i32 y = 0; y < height; y++) {
        u8* src_row = (u8*)state.global_offscreen_buffer.memory + (y * width * bytes_per_pixel);
        u8* dest_row = buffer + ((height - y - 1) * width * bytes_per_pixel);
        copy_memory(src_row, dest_row, width * bytes_per_pixel);
    }

    int offset = 0;
    auto result = StretchDIBits(             //
        device_context,                      //
        0, 0,                                //
        width, height, 0, 0,                 //
        width, height,                       //
        buffer,                              //
        &state.global_offscreen_buffer.Info, //
        DIB_RGB_COLORS, SRCCOPY              //
    );
    if (result == 0 || result == GDI_ERROR) {
        log_error("StretchDIBits failed\n");
    }

    ReleaseDC(win32_context->window, device_context);
}

// TODO: I need this to handle redraw calls from windows, e.g. if you move the window, or move a window above it
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

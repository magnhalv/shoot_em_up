#pragma once

#include <platform/types.h>

#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>

#include <engine/hugin_file_formats.h>

// Move somewhere else
//

enum RendererType : i32 {
    RendererType_Software, //
    RendererType_OpenGL    //
};

typedef struct {
    vec2 bl;
    vec2 tl;
    vec2 tr;
    vec2 br;
} Quadrilateral;

struct RenderEntityBasis {
    vec2 offset;
};

enum RenderGroupEntryType {
    RenderCommands_RenderEntryClear,
    RenderCommands_RenderEntryQuadrilateral,
    RenderCommands_RenderEntryBitmap
};

struct RenderGroupEntryHeader {
    RenderGroupEntryType type;
};

struct RenderEntryClear {
    vec4 color; // r,g,b,a
};

struct RenderEntryQuadrilateral {
    RenderEntityBasis render_basis;
    Quadrilateral quad;
    vec4 color;
    vec2 offset;
    f32 rotation;
    vec2 scale;
};

typedef struct {
    RenderEntityBasis render_basis;
    Quadrilateral quad;
    Quadrilateral uv;
    vec4 color;
    vec2 offset;
    f32 rotation;
    vec2 scale;
    i32 texture_id;
    ivec2 uv_min;
    ivec2 uv_max;
} RenderEntryBitmap;

struct RenderGroup {
    f32 meters_to_pixels;
    i32 screen_width;
    i32 screen_height;

    u64 max_push_buffer_size;
    u64 push_buffer_size;
    u8* push_buffer;
};

#define PushRenderElement(group, type) (type*)push_render_element_(group, sizeof(type), RenderCommands_##type)
auto inline push_render_element_(RenderGroup* render_group, u32 size, RenderGroupEntryType type) {
    void* result = 0;

    size += sizeof(RenderGroupEntryHeader);
    if ((render_group->push_buffer_size + size) < render_group->max_push_buffer_size) {
        RenderGroupEntryHeader* header = (RenderGroupEntryHeader*)(render_group->push_buffer + render_group->push_buffer_size);
        header->type = type;
        result = (u8*)(header) + sizeof(*header);
        render_group->push_buffer_size += size;
    }
    else {
        // TODO: return a null struct perhaps?
        InvalidCodePath;
    }
    return result;
}

const i32 MaxTextureId = 1024;

#define RENDERER_API __cdecl

#define RENDERER_INIT(name) void name(void* context, MemoryBlock* memory)
typedef RENDERER_INIT(renderer_init_fn);

#define RENDERER_ADD_TEXTURE(name) bool name(i32 texture_id, void* data, i32 width, i32 height, i32 bytes_per_pixel)
typedef RENDERER_ADD_TEXTURE(renderer_add_texture_fn);

#define RENDERER_RENDER(name) void name(RenderGroup* group, i32 client_width, i32 client_height)
typedef RENDERER_RENDER(renderer_render_fn);

#define RENDERER_BEGIN_FRAME(name) void name(void* context)
typedef RENDERER_BEGIN_FRAME(renderer_begin_frame_fn);

#define RENDERER_END_FRAME(name) void name(void* context)
typedef RENDERER_END_FRAME(renderer_end_frame_fn);

#define RENDERER_DELETE_CONTEXT(name) void name(void* context)
typedef RENDERER_DELETE_CONTEXT(renderer_delete_context_fn);

struct RendererApi {
    renderer_init_fn* init;
    renderer_add_texture_fn* add_texture;
    renderer_render_fn* render;
    renderer_begin_frame_fn* begin_frame;
    renderer_end_frame_fn* end_frame;
    renderer_delete_context_fn* delete_context;
};

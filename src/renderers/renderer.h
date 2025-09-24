#pragma once

#include <math/vec2.h>
#include <math/vec3.h>
#include <math/vec4.h>

// Move somewhere else

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

struct CoordSystem {
    vec2 x;
    vec2 y;
};

struct RenderEntryQuadrilateral {
    RenderEntityBasis render_basis;
    Quadrilateral quad;
    vec4 color;
    vec2 local_origin;
    vec2 offset;
    CoordSystem basis;
};

typedef struct {
    RenderEntityBasis render_basis;
    Quadrilateral quad;
    Quadrilateral uv;
    vec4 color;
    vec2 local_origin;
    vec2 offset;
    CoordSystem basis;
    u32 bitmap_handle;
} RenderEntryBitmap;

struct RenderGroup {
    f32 meters_to_pixels;
    i32 screen_width;
    i32 screen_height;

    u32 max_push_buffer_size;
    u32 push_buffer_size;
    u8* push_buffer;
};

#define RENDERER_API __cdecl

#define RENDERER_INIT(name) void name(void* context)
typedef RENDERER_INIT(renderer_init_fn);

#define RENDERER_ADD_TEXTURE(name) u32 name(void* data, i32 width, i32 height)
typedef RENDERER_ADD_TEXTURE(renderer_add_texture_fn);

#define RENDERER_RENDER(name) void name(RenderGroup* group, i32 client_width, i32 client_height)
typedef RENDERER_RENDER(renderer_render_fn);

#define RENDERER_END_FRAME(name) void name()
typedef RENDERER_END_FRAME(renderer_end_frame_fn);

#define RENDERER_DELETE_CONTEXT(name) void name(void* context)
typedef RENDERER_DELETE_CONTEXT(renderer_delete_context_fn);

struct RendererApi {
    renderer_init_fn* init;
    renderer_add_texture_fn* add_texture;
    renderer_render_fn* render;
    renderer_end_frame_fn* end_frame;
    renderer_delete_context_fn* delete_context;
};

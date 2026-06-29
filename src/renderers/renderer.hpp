#pragma once

#include <platform/platform.hpp>
#include <platform/types.hpp>

#include <math/transform.hpp>
#include <math/vec2.hpp>
#include <math/vec3.hpp>
#include <math/vec4.hpp>

#include <core/array.hpp>
#include <core/list.hpp>

#include <engine/hugin_file_formats.hpp>

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

enum RenderGroupEntryType {                      //
    RenderCommands_RenderEntryClear,             //
    RenderCommands_RenderEntryClearCheckPattern, //
    RenderCommands_RenderEntryBitmap,            //
    RenderCommands_RenderEntryLine,              //
    RenderCommands_RenderEntryCircle,            //
    RenderCommands_RenderEntryFilledCircle,      //
    RenderCommands_RenderEntryTriangle,          //
    RenderCommands_RenderEntryFilledTriangle,    //
    RenderCommands_RenderEntryShadedTriangle,    //
    RenderCommands_RenderEntryTriMesh,           //
    RenderCommands_RenderEntryTriMeshWireframe   //
};

struct RenderGroupEntryHeader {
    RenderGroupEntryType type;
};

struct RenderEntryClear {
    vec4 color; // r,g,b,a
};

struct RenderEntryClearCheckPattern {
    vec4 color1; // r,g,b,a
    vec4 color2; // r,g,b,a
};

struct RenderEntryQuadrilateral {
    RenderEntityBasis render_basis;
    Quadrilateral quad;
    vec4 color;
    vec2 offset;
    f32 rotation;
    vec2 scale;
};

struct RenderEntryBitmap {
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
    f32 border_thickness;
    vec4 border_color;
};

struct RenderEntryLine {
    vec3 start;
    vec3 end;
    vec4 color;
};

struct RenderEntryCircle {
    vec2 P;
    f32 radius;
    vec4 color;
};

struct RenderEntryFilledCircle {
    vec2 P;
    f32 radius;
    vec4 color;
};

struct RenderEntryTriangle {
    union {
        struct {
            vec3 P0;
            vec3 P1;
            vec3 P2;
        };
        vec3 vertices[3];
    };
    vec4 color;
};

struct RenderEntryFilledTriangle {
    union {
        struct {
            vec3 P0;
            vec3 P1;
            vec3 P2;
        };
        vec3 vertices[3];
    };
    vec4 color;
};

struct RenderEntryShadedTriangle {
    union {
        struct {
            vec3 P0;
            vec3 P1;
            vec3 P2;
        };
        vec2 vertices[3];
    };
    f32 h0;
    f32 h1;
    f32 h2;
    vec4 color;
};

struct TriMesh {
    Array<vec4> vertices;
    Array<ivec3> triangles;
    Array<vec3> normals;
    Array<vec4> colors;
};

struct RenderEntryTriMesh {
    vec4 camera_position;
    mat4 world_to_view;
    mat4 view_to_clip;
    TriMesh model;
    Array<Transform> instances;
};

struct RenderEntryPolygonInstances {
    Array<vec3> vertices;
    Array<ivec3> triangles;
    Array<vec4> colors;
};

struct RenderGroup {
    List<u64> sort_entries_offset;
    List<i32> sort_keys;

    u64 max_push_buffer_size;
    u64 push_buffer_size;
    u8* push_buffer;
};

// INTERNAL

struct Framebuffer {
    void* memory;
    Array<f32> z_buffer;
    i32 memory_size;
    i32 width;
    i32 height;
    i32 bytes_per_pixel;
    i32 pitch;

    auto inline set_pixel(i32 x, i32 y, u32 color) -> void {
        Assert(x >= 0 && x < width);
        Assert(y >= 0 && y < height);
        Assert(sizeof(color) == sizeof(u32));
        u32* dest = (u32*)((u8*)memory + (y * pitch) + (x * bytes_per_pixel));
        *dest = color;
    }

    auto inline get_pixel(i32 x, i32 y) -> u32* {
        Assert(x >= 0 && x < width);
        Assert(y >= 0 && y < height);
        return (u32*)((u8*)memory + (y * pitch) + (x * bytes_per_pixel));
    }
};

// INTERNAL END

#define PushRenderElement(group, type, sort_key) \
    (type*)push_render_element_(group, sizeof(type), RenderCommands_##type, sort_key)
auto inline push_render_element_(RenderGroup* render_group, u32 size, RenderGroupEntryType type, i32 sort_key) {
    void* result = 0;

    size += sizeof(RenderGroupEntryHeader);
    if ((render_group->push_buffer_size + size) < render_group->max_push_buffer_size) {
        RenderGroupEntryHeader* header = (RenderGroupEntryHeader*)(render_group->push_buffer + render_group->push_buffer_size);
        header->type = type;
        result = (u8*)(header) + sizeof(*header);
        render_group->sort_entries_offset.push(render_group->push_buffer_size);
        render_group->sort_keys.push(sort_key);

        render_group->push_buffer_size += size;
    }
    else {
        // TODO: return a null struct perhaps?
        InvalidCodePath;
    }

    return result;
}

const i32 MaxTextureId = 1024;

struct FrameBufferHandle {
    i32 v;
};

struct PackedColor {
    u32 v;
};

struct Color {
    vec4 v;
};

#define RENDERER_API __cdecl

#define RENDERER_INIT(name) void name(void* context, PlatformApi* platform_api, MemoryBlock* memory)
typedef RENDERER_INIT(renderer_init_fn);

#define RENDERER_ADD_TEXTURE(name) bool name(i32 texture_id, void* data, i32 width, i32 height, i32 bytes_per_pixel)
typedef RENDERER_ADD_TEXTURE(renderer_add_texture_fn);

#define RENDERER_RENDER(name) void name(PlatformWorkQueue* render_queue, RenderGroup* group, FrameBufferHandle handle)
typedef RENDERER_RENDER(renderer_render_fn);

#define RENDERER_BEGIN_FRAME(name) void name(void* context)
typedef RENDERER_BEGIN_FRAME(renderer_begin_frame_fn);

#define RENDERER_END_FRAME(name) void name(void* context)
typedef RENDERER_END_FRAME(renderer_end_frame_fn);

#define RENDERER_DELETE_CONTEXT(name) void name(void* context)
typedef RENDERER_DELETE_CONTEXT(renderer_delete_context_fn);

// Framebuffer
#define RENDERER_CREATE_FRAMEBUFFER(name) FrameBufferHandle name(i32 width, i32 height)
typedef RENDERER_CREATE_FRAMEBUFFER(renderer_create_framebuffer_fn);

#define RENDERER_APPLY_FRAMEBUFFER(name) \
    void name(FrameBufferHandle handle, i32 width, i32 height, i32 offset_x, i32 offset_y)
typedef RENDERER_APPLY_FRAMEBUFFER(renderer_apply_framebuffer_fn);

#define RENDERER_GET_COLOR(name) Color name(FrameBufferHandle handle, i32 offset_x, i32 offset_y)
typedef RENDERER_GET_COLOR(renderer_get_color_fn);
// Framebuffer end

struct RendererApi {
    renderer_init_fn* init;
    renderer_add_texture_fn* add_texture;
    renderer_render_fn* render;
    renderer_begin_frame_fn* begin_frame;
    renderer_end_frame_fn* end_frame;
    renderer_delete_context_fn* delete_context;

    renderer_create_framebuffer_fn* create_framebuffer;
    renderer_apply_framebuffer_fn* apply_framebuffer;
    renderer_get_color_fn* get_color;
};

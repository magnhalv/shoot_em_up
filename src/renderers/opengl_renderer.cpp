#include <platform/platform.h>

#include <engine/hm_assert.h>

#include "gl/gl.h"
#include "gl/gl_shader.h"

#include "renderer.h"

const u32 MaxNumTextures = 5;

typedef struct {
    u32 quad_vao;
    u32 quad_vbo;
    GLShaderProgram quad_shader;

    u32 texture_vao;
    u32 texture_vbo;
    u32 texture_ebo;
    GLShaderProgram texture_shader;

    u32 textures[5];
    u32 num_textures;

} GlRendererState;

static GlRendererState state = {};

static auto clear(i32 client_width, i32 client_height, vec4 color) {
    gl->viewport(0, 0, client_width, client_height);
    gl->clear_color(color.x, color.y, color.z, color.w);
    gl->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Remove depth buffer?
}

static auto to_gl_x(i32 screen_x_cord, i32 screen_width) -> f32 {
    return screen_x_cord / (screen_width / 2.0) - 1.0f;
}

static auto to_gl_y(i32 screen_y_cord, i32 screen_height) -> f32 {
    return screen_y_cord / (screen_height / 2.0) - 1.0f;
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

    f32 quad_verticies[12] = {
        to_gl_x(bl.x, screen_width),
        to_gl_y(bl.y, screen_height),
        to_gl_x(tl.x, screen_width),
        to_gl_y(tl.y, screen_height),
        to_gl_x(tr.x, screen_width),
        to_gl_y(tr.y, screen_height),

        to_gl_x(bl.x, screen_width),
        to_gl_y(bl.y, screen_height),
        to_gl_x(tr.x, screen_width),
        to_gl_y(tr.y, screen_height),
        to_gl_x(br.x, screen_width),
        to_gl_y(br.y, screen_height),
    };

    HM_ASSERT(state.quad_vao != 0);
    gl->bind_vertex_array(state.quad_vao);
    gl->named_buffer_sub_data(state.quad_vbo, 0, sizeof(quad_verticies), quad_verticies);

    state.quad_shader.bind();
    state.quad_shader.set_uniform("color", color);

    gl->draw_arrays(GL_TRIANGLES, 0, 6);

    state.quad_shader.unbind();
    gl->bind_vertex_array(0);
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

    f32 vertices[] = {
        // clang-format off
        // positions                                                    // colors           // texture coords
      to_gl_x(tr.x, screen_width), to_gl_y(tr.y, screen_height), 0.0f,  color.r, color.g, color.b, color.a,   1.0f, 1.0f,   // top right
        
      to_gl_x(br.x, screen_width), to_gl_y(br.y, screen_height), 0.0f,  color.r, color.g, color.b, color.a,  1.0f, 0.0f,  // bottom right
        
      to_gl_x(bl.x, screen_width), to_gl_y(bl.y, screen_height), 0.0f,  color.r, color.g, color.b, color.a,   0.0f, 0.0f, // bottom left

      to_gl_x(tl.x, screen_width), to_gl_y(tl.y, screen_height), 0.0f,   color.r, color.g, color.b, color.a,  0.0f, 1.0f   // top left
        // clang-format on
    };

    u32* texture = &state.textures[bitmap_handle];
    HM_ASSERT(state.texture_vao != 0);
    HM_ASSERT(*texture != 0);

    gl->enable(GL_BLEND);
    gl->blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl->bind_vertex_array(state.texture_vao);
    gl->named_buffer_sub_data(state.texture_vbo, 0, sizeof(vertices), vertices);
    gl->bind_texture(GL_TEXTURE_2D, *texture);
    state.texture_shader.bind();
    gl->draw_elements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    gl->bind_vertex_array(0);

    state.texture_shader.unbind();
    gl->bind_vertex_array(0);
    gl->disable(GL_BLEND);
}

extern "C" __declspec(dllexport) RENDERER_INIT(win32_renderer_init) {
    // Quad VAO
    {
        f32 quad_verticies[12] = { -1, -1, -1, 1, 1, 1, -1, -1, 1, 1, 1, -1 };
        gl->create_vertex_arrays(1, &state.quad_vao);
        gl->bind_vertex_array(state.quad_vao);
        gl->create_buffers(1, &state.quad_vbo);

        gl->named_buffer_storage(state.quad_vbo, sizeof(quad_verticies), quad_verticies, GL_DYNAMIC_STORAGE_BIT);
        auto binding_index = 0;
        auto stride = 2 * sizeof(f32);
        auto offset = 0;
        gl->vertex_array_vertex_buffer(state.quad_vao, binding_index, state.quad_vbo, offset, stride);
        auto attrib_index = 0;

        gl->enable_vertex_array_attrib(state.quad_vao, attrib_index);
        gl->vertex_array_attrib_format(state.quad_vao, attrib_index, 2, GL_FLOAT, GL_FALSE, offset);
        gl->vertex_array_attrib_binding(state.quad_vao, attrib_index, attrib_index);
        gl->bind_vertex_array(0);

        state.quad_shader.initialize(R"(.\shaders\basic_2d.vert)", R"(.\shaders\single_color.frag)");
    }

    // Texture VAO
    {
        u32* vao = &state.texture_vao;
        u32* vbo = &state.texture_vbo;
        u32* ebo = &state.texture_ebo;
        f32 vertices[] = {
            // clang-format off
        // positions       // colors                 // texture coords
        1.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
        1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f,   1.0f, 0.0f,  // bottom right
        0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f,   0.0f, 0.0f, // bottom left
        0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 0.0f, 0.0f,   0.0f, 1.0f   // top left
            // clang-format on
        };
        const u32 vbo_offset = 0;
        const u32 num_pos = 3;
        const u32 num_colors = 4;
        const u32 num_uv = 2;
        const u32 vbo_stride = (num_pos + num_colors + num_uv) * sizeof(f32);

        u32 indices[] = {
            0, 1, 3, // first triangle
            1, 2, 3  // second triangle
        };

        gl->create_vertex_arrays(1, vao);
        gl->create_buffers(1, vbo);
        gl->create_buffers(1, ebo);

        gl->bind_vertex_array(*vao);

        gl->named_buffer_storage(*vbo, sizeof(vertices), vertices, GL_DYNAMIC_STORAGE_BIT);
        gl->named_buffer_storage(*ebo, sizeof(indices), indices, GL_DYNAMIC_STORAGE_BIT);
        gl->vertex_array_element_buffer(*vao, *ebo);
        const u32 VBO_bi = 0;
        gl->vertex_array_vertex_buffer(*vao, VBO_bi, *vbo, vbo_offset, vbo_stride);

        u32 vbo_binding_index = 0;
        u32 attrib_index = 0;
        // position attribute
        gl->vertex_array_attrib_format(*vao, attrib_index, num_pos, GL_FLOAT, GL_FALSE, 0);
        gl->vertex_array_attrib_binding(*vao, attrib_index, vbo_binding_index);
        gl->enable_vertex_array_attrib(*vao, attrib_index);
        attrib_index++;

        // color attribute
        gl->vertex_array_attrib_format(*vao, attrib_index, num_colors, GL_FLOAT, GL_FALSE, num_pos * sizeof(f32));
        gl->vertex_array_attrib_binding(*vao, attrib_index, vbo_binding_index);
        gl->enable_vertex_array_attrib(*vao, attrib_index);
        attrib_index++;

        // texture coord attribute
        gl->vertex_array_attrib_format(*vao, attrib_index, num_uv, GL_FLOAT, GL_FALSE, (num_pos + num_colors) * sizeof(f32));
        gl->vertex_array_attrib_binding(*vao, attrib_index, vbo_binding_index);
        gl->enable_vertex_array_attrib(*vao, attrib_index);
        attrib_index++;

        state.texture_shader.initialize(R"(.\shaders\texture_2d.vert)", R"(.\shaders\texture_2d.frag)");
    }
}

extern "C" __declspec(dllexport) RENDERER_ADD_TEXTURE(win32_renderer_add_texture) {
    if (state.num_textures >= MaxNumTextures) {
        InvalidCodePath;
    }

    auto* texture = &state.textures[state.num_textures];
    auto handle = state.num_textures;
    state.num_textures++;

    gl->gen_textures(1, texture);
    gl->bind_texture(GL_TEXTURE_2D, *texture);
    // set the texture wrapping parameters
    // set texture wrapping to GL_REPEAT (default wrapping method)
    gl->tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    gl->tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    gl->tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl->tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    gl->tex_image_2d(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    gl->generate_mip_map(GL_TEXTURE_2D);

    gl->bind_texture(GL_TEXTURE_2D, 0);
    return handle;
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

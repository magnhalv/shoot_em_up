#include <platform/platform.h>

#include "renderer.h"
#include "engine/gl/gl_vao.h"
#include "engine/hm_assert.h"

enum VaoIds {
  ColoredQuad = 0,
  BitmapQuad = 1, 
};

typedef struct {
  GLShaderProgram shaders[5];
  Bitmap bitmaps[5];
  GLVao vaos[2];
} GlRendererState;

internal GlRendererState state;

auto renderer_init() -> void {
    f32 quad_verticies[12] = {
      -1, -1,
      -1, 1,
      1, 1,
      -1, -1,
      1, 1,
      1, -1
    };
    GLVao *vao = &state.vaos[ColoredQuad];
    vao->init();
    vao->bind();
    vao->add_buffer(0, sizeof(quad_verticies), 2 * sizeof(f32));
    vao->add_buffer_desc(0, 0, 2, 0, 2 * sizeof(f32));
    vao->upload_buffer_desc();
    vao->upload_buffer_data(0, quad_verticies, 0, sizeof(quad_verticies));
    vao->unbind();


    GLShaderProgram *program = &state.shaders[ColoredQuad];
    program->initialize(R"(.\assets\shaders\basic_2d.vert)", R"(.\assets\shaders\single_color.frag)");

    GLVao *vao = &state.vaos[BitmapQuad];

    f32 vertices[] = {*/
        // positions         // colors           // texture coords
        1.0f, 1.0f, 0.0f,    0.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
        1.0f, 0.0f, 0.0f,   0.0f, 0.0f, 0.0f,   1.0f, 0.0f,  // bottom right
        0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 0.0f,   0.0f, 0.0f, // bottom left
        0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 0.0f,   0.0f, 1.0f   // top left
    };
    u32 indices[] = {
      0, 1, 3, // first triangle
      1, 2, 3  // second triangle
    };

    vao->init();
    vao->bind();
    gl->create_vertex_arrays(1, &sprite.vao);
    gl->create_buffers(1, &sprite.vbo);
    gl->create_buffers(1, &sprite.ebo);

  /*  gl->bind_vertex_array(sprite.vao);*/
  /**/
  /*  gl->named_buffer_storage(sprite.vbo, sizeof(vertices), vertices, GL_DYNAMIC_STORAGE_BIT);*/
  /*  gl->named_buffer_storage(sprite.ebo, sizeof(indices), indices, GL_DYNAMIC_STORAGE_BIT);*/
  /*  gl->vertex_array_element_buffer(sprite.vao, sprite.ebo);*/
  /*  u32 VBO_bi = 0;*/
  /*  gl->vertex_array_vertex_buffer(sprite.vao, VBO_bi, sprite.vbo, 0, 8 * sizeof(f32)); // 8 * sizeof(f32) is the stride*/
  /**/
  /*  u32 vbo_binding_index = 0;*/
  /*  u32 attrib_index = 0;*/
  /*  // position attribute*/
  /*  gl->vertex_array_attrib_format(sprite.vao, attrib_index, 3, GL_FLOAT, GL_FALSE, 0);*/
  /*  gl->vertex_array_attrib_binding(sprite.vao, attrib_index, vbo_binding_index);*/
  /*  gl->enable_vertex_array_attrib(sprite.vao, attrib_index);*/
  /*  attrib_index++;*/
  /**/
  /*  // color attribute*/
  /*  gl->vertex_array_attrib_format(sprite.vao, attrib_index, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32));*/
  /*  gl->vertex_array_attrib_binding(sprite.vao, attrib_index, vbo_binding_index);*/
  /*  gl->enable_vertex_array_attrib(sprite.vao, attrib_index);*/
  /*  attrib_index++;*/
  /**/
  /*  // texture coord attribute*/
  /*  gl->vertex_array_attrib_format(sprite.vao, attrib_index, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(f32));*/
  /*  gl->vertex_array_attrib_binding(sprite.vao, attrib_index, vbo_binding_index);*/
  /*  gl->enable_vertex_array_attrib(sprite.vao, attrib_index);*/
  /*  attrib_index++;*/
  /**/
  /*// load and create a texture*/
  /*// -------------------------*/
  /*// texture 1*/
  /*// ---------*/
  /*gl->gen_textures(1, &sprite.tex1);*/
  /*gl->bind_texture(GL_TEXTURE_2D, sprite.tex1);*/
  /*// set the texture wrapping parameters*/
  /*// set texture wrapping to GL_REPEAT (default wrapping method)*/
  /*gl->tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);*/
  /*gl->tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);*/
  /*// set texture filtering parameters*/
  /*gl->tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);*/
  /*gl->tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);*/
  /*// load image, create texture and generate mipmaps*/
  /*i32 width, height, nrChannels;*/
  /*stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.*/
  /*unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);*/
  /*if (data) {*/
  /*  if (nrChannels == 4) {*/
  /*    gl->tex_image_2d(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);*/
  /*  } else if (nrChannels == 3) {*/
  /*    gl->tex_image_2d(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);*/
  /*  } else {*/
  /*    crash_and_burn("Unsupported number of channels when loading texture: : %s", path);*/
  /*  }*/
  /*  gl->generate_mip_map(GL_TEXTURE_2D);*/
  /*} else {*/
  /*  crash_and_burn("Failed to load texture: %s", path);*/
  /*}*/
  /*stbi_image_free(data);*/
  /**/
}

auto clear(i32 client_width, i32 client_height, vec4 color) {
  gl->viewport(0, 0, client_width, client_height);
  gl->clear_color(color.x, color.y, color.z, color.w);
  gl->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Remove depth buffer?
}


auto to_gl_x(i32 screen_x_cord, i32 screen_width) -> f32 {
  return screen_x_cord / (screen_width/2.0) - 1.0f;
}

auto to_gl_y(i32 screen_y_cord, i32 screen_height) -> f32 {
  return screen_y_cord / (screen_height/2.0) - 1.0f;
}

auto draw_quad(Quadrilateral quad, vec2 local_origin, vec2 offset, vec2 x_axis, vec2 y_axis, vec4 color, i32 screen_width, i32 screen_height) {
    vec2 bl = quad.bl - local_origin;
    vec2 tl = quad.tl - local_origin;
    vec2 tr = quad.tr - local_origin;
    vec2 br = quad.br - local_origin;

    bl = bl.x*x_axis + bl.y*y_axis;
    tl = tl.x*x_axis + tl.y*y_axis;
    tr = tr.x*x_axis + tr.y*y_axis;
    br = br.x*x_axis + br.y*y_axis;

    bl = bl + local_origin;
    tl = tl + local_origin;
    tr = tr + local_origin;
    br = br + local_origin;

    bl = bl + offset;
    tl = tl + offset;
    tr = tr + offset;
    br = br + offset;

    f32 quad_verticies[12] = {
      to_gl_x(bl.x, screen_width), to_gl_y(bl.y, screen_height),
      to_gl_x(tl.x, screen_width), to_gl_y(tl.y, screen_height),
      to_gl_x(tr.x, screen_width), to_gl_y(tr.y, screen_height),

      to_gl_x(bl.x, screen_width), to_gl_y(bl.y, screen_height),
      to_gl_x(tr.x, screen_width), to_gl_y(tr.y, screen_height),
      to_gl_x(br.x, screen_width), to_gl_y(br.y, screen_height),
    };
    
    GLVao *vao = &state.vaos[ColoredQuad];
    HM_ASSERT(vao->handle != 0);
    vao->bind();
    vao->upload_buffer_data(0, quad_verticies, 0, sizeof(quad_verticies));

    
    GLShaderProgram *program = &state.shaders[ColoredQuad];
    program->bind();
    program->set_uniform("color", color);

    gl->draw_arrays(GL_TRIANGLES, 0, 6);
  
    program->unbind();
    vao->unbind();
}

auto rotate(vec2 p, f32 theta) -> vec2 {
 return { p.x*cos(theta) - p.y*sin(theta),
          p.y * cos(theta) + p.x * sin(theta)};
}

auto render(RenderGroup *group, i32 client_width, i32 client_height) -> void {

  for (u32 base_address = 0; base_address < group->push_buffer_size;) {
    RenderGroupEntryHeader *header = (RenderGroupEntryHeader*)(group->push_buffer + base_address);
    base_address += sizeof(RenderGroupEntryHeader);

    void *data = (u8*)header + sizeof(*header);
    switch (header->type) {
    case RenderCommands_RenderEntryClear: 
      {
        RenderEntryClear *entry = (RenderEntryClear*)data;
        clear(group->screen_width, group->screen_height, entry->color);
        base_address += sizeof(*entry);
      } 
      break;
    case RenderCommands_RenderEntryQuadrilateral: 
      {
        auto *entry = (RenderEntryQuadrilateral*)data;
        draw_quad(entry->quad, 
                  entry->local_origin, 
                  entry->offset, 
                  entry->basis.x, entry->basis.y, 
                  entry->color, 
                  group->screen_width, group->screen_height);

        base_address += sizeof(*entry);
      } 
      break;
    default: 
      InvalidCodePath;
    }
  } 
}

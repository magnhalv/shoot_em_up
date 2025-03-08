#include <platform/platform.h>

#include "renderer.h"


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

    float quad_verticies[] = {
      to_gl_x(bl.x, screen_width), to_gl_y(bl.y, screen_height),
      to_gl_x(tl.x, screen_width), to_gl_y(tl.y, screen_height),
      to_gl_x(tr.x, screen_width), to_gl_y(tr.y, screen_height),

      to_gl_x(bl.x, screen_width), to_gl_y(bl.y, screen_height),
      to_gl_x(tr.x, screen_width), to_gl_y(tr.y, screen_height),
      to_gl_x(br.x, screen_width), to_gl_y(br.y, screen_height),
    };
    
    GLVao vao;
    vao.init();
    vao.bind();
    vao.add_buffer(0, sizeof(quad_verticies), 2 * sizeof(f32));
    vao.add_buffer_desc(0, 0, 2, 0, 2 * sizeof(f32));
    vao.upload_buffer_desc();
    vao.upload_buffer_data(0, quad_verticies, 0, sizeof(quad_verticies));

    GLShaderProgram program;
    program.initialize(R"(.\assets\shaders\basic_2d.vert)", R"(.\assets\shaders\single_color.frag)");
    program.useProgram();
    program.set_uniform("color", color);

    gl->draw_arrays(GL_TRIANGLES, 0, 6);
    program.free();
    vao.destroy();

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

#include "sprite.hpp"

#include <libs/stb/stb_image.h>

#include "gl/gl.h"
#include "logger.h"

auto load_sprite(const char* path, GLShaderProgram* program) -> Sprite {
  Sprite sprite = {};
  sprite.program = program;
  f32 vertices[] = {
    // clang-format off
      // positions         // colors           // texture coords
      1.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
      1.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,  // bottom right
      0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
      0.0f, 1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f   // top left
    // clang-format on
  };
  u32 indices[] = {
    0, 1, 3, // first triangle
    1, 2, 3  // second triangle
  };
  gl->create_vertex_arrays(1, &sprite.vao);
  gl->create_buffers(1, &sprite.vbo);
  gl->create_buffers(1, &sprite.ebo);

  gl->bind_vertex_array(sprite.vao);

  gl->named_buffer_storage(sprite.vbo, sizeof(vertices), vertices, GL_DYNAMIC_STORAGE_BIT);
  gl->named_buffer_storage(sprite.ebo, sizeof(indices), indices, GL_DYNAMIC_STORAGE_BIT);
  gl->vertex_array_element_buffer(sprite.vao, sprite.ebo);
  u32 VBO_bi = 0;
  gl->vertex_array_vertex_buffer(sprite.vao, VBO_bi, sprite.vbo, 0, 8 * sizeof(f32)); // 8 * sizeof(f32) is the stride

  u32 vbo_binding_index = 0;
  u32 attrib_index = 0;
  // position attribute
  gl->vertex_array_attrib_format(sprite.vao, attrib_index, 3, GL_FLOAT, GL_FALSE, 0);
  gl->vertex_array_attrib_binding(sprite.vao, attrib_index, vbo_binding_index);
  gl->enable_vertex_array_attrib(sprite.vao, attrib_index);
  attrib_index++;

  // color attribute
  gl->vertex_array_attrib_format(sprite.vao, attrib_index, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(f32));
  gl->vertex_array_attrib_binding(sprite.vao, attrib_index, vbo_binding_index);
  gl->enable_vertex_array_attrib(sprite.vao, attrib_index);
  attrib_index++;

  // texture coord attribute
  gl->vertex_array_attrib_format(sprite.vao, attrib_index, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(f32));
  gl->vertex_array_attrib_binding(sprite.vao, attrib_index, vbo_binding_index);
  gl->enable_vertex_array_attrib(sprite.vao, attrib_index);
  attrib_index++;

  // load and create a texture
  // -------------------------
  // texture 1
  // ---------
  gl->gen_textures(1, &sprite.tex1);
  gl->bind_texture(GL_TEXTURE_2D, sprite.tex1);
  // set the texture wrapping parameters
  // set texture wrapping to GL_REPEAT (default wrapping method)
  gl->tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  gl->tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // set texture filtering parameters
  gl->tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl->tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // load image, create texture and generate mipmaps
  i32 width, height, nrChannels;
  stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
  unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
  if (data) {
    if (nrChannels == 4) {
      gl->tex_image_2d(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    } else if (nrChannels == 3) {
      gl->tex_image_2d(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    } else {
      crash_and_burn("Unsupported number of channels when loading texture: : %s", path);
    }
    gl->generate_mip_map(GL_TEXTURE_2D);
  } else {
    crash_and_burn("Failed to load texture: %s", path);
  }
  stbi_image_free(data);

  Transform tranform = Transform();
  tranform.scale.x = width;
  tranform.scale.y = height;
  sprite.transform = tranform;
  return sprite;
}

auto render_sprite(Sprite& sprite, mat4 projection) -> void {
  sprite.program->useProgram();
  sprite.program->set_uniform("projection", projection);
  mat4 model = identity();
  sprite.program->set_uniform("model", sprite.transform.to_mat4());
  gl->bind_vertex_array(sprite.vao);
  gl->bind_texture(GL_TEXTURE_2D, sprite.tex1);
  gl->draw_elements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  gl->bind_vertex_array(0);
}

#include "text_renderer.h"
#include "gl/gl.h"

#include <cstring>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "logger.h"
#include "memory_arena.h"
#include "stb_image_write.h"

auto font_str_dim(const char* str, f32 scale, Font& font) -> vec2 {
  auto length = strlen(str);
  auto& characters = font.characters;

  auto x_length = 0.0f;
  auto y_length = 0.0f;
  for (auto i = 0; i < length; i++) {
    char c = str[i];
    if (c == '\0') {
      continue;
    }
    assert(c > 0 && c < characters.size());
    Character ch = characters[c];

    y_length = fmax(ch.size.y * scale, y_length);

    // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
    x_length += (ch.advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
  }
  return vec2(x_length, y_length);
}

auto write_texture_to_png(const char* file_path, u32 texture_id, i32 width, i32 height) {
  auto total = width * height;
  auto bytes = allocate<u8>(*g_transient, total);
  gl->bind_texture(GL_TEXTURE_2D, texture_id);
  gl->get_tex_image(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, bytes);

  auto pixels = allocate<u32>(*g_transient, total);
  for (auto i = 0; i < total; i++) {
    auto b = bytes[i];
    pixels[i] = (b << 24) | (b << 16) | (b << 8) | 8;
  }

  gl->bind_texture(GL_TEXTURE_2D, 0);
  stbi_write_png(file_path, width, height, 4, pixels, width * sizeof(u32));
}

auto font_load(const char* path, MemoryArena& permanent_arena) -> Font* {
  auto* font = allocate<Font>(permanent_arena);

  FT_Library ft;
  if (FT_Init_FreeType(&ft)) {
    crash_and_burn("Failed to initialize FreeType library.");
  }

  FT_Face face;
  if (FT_New_Face(ft, path, 0, &face)) {
    crash_and_burn("Failed to load Ubuntu font.");
  }
  FT_Set_Pixel_Sizes(face, 0, 48);

  gl->pixel_store_i(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

  font->characters.init(permanent_arena, 128);
  i32 i = 0;

  const auto atlas_height = 512;
  const auto atlas_width = 512;

  // generate texture
  gl->gen_textures(1, &font->texture_atlas);
  gl->bind_texture(GL_TEXTURE_2D, font->texture_atlas);
  gl->tex_image_2d(GL_TEXTURE_2D, 0, GL_RED, atlas_width, atlas_height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
  // set texture options
  gl->tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  gl->tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  gl->tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  gl->tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  auto max_height_in_row = 0;
  auto x = 5;
  auto y = 5;
  const auto padding_x = 5;
  const auto padding_y = 5;

  u8 data[5 * 5] = {
    255, 255, 255, 255, 255, //
    255, 255, 255, 255, 255, //
    255, 255, 255, 255, 255, //
    255, 255, 255, 255, 255, //
    255, 255, 255, 255, 255, //
  };
  gl->tex_sub_image_2d(GL_TEXTURE_2D, 0, 0, 0, 5, 5, GL_RED, GL_UNSIGNED_BYTE, data);
  for (u8 c = 0; c < 128; c++) {
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
      crash_and_burn("FreeType: Failed to load glyph.");
    }
    auto width = face->glyph->bitmap.width;
    auto height = face->glyph->bitmap.rows;

    if ((x + width) > atlas_width) {
      x = 0;
      y += max_height_in_row + padding_y;
    }

    assert(y < atlas_height);
    max_height_in_row = height > max_height_in_row ? height : max_height_in_row;

    gl->tex_sub_image_2d(GL_TEXTURE_2D, 0, x, y, width, height, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
    f32 u_start = static_cast<f32>(x) / atlas_width;
    f32 v_start = static_cast<f32>(y) / atlas_height;

    f32 u_end = static_cast<f32>(x + width) / atlas_width;
    f32 v_end = static_cast<f32>(y + height) / atlas_height;
    // now store character for later use
    font->characters[i++] = {
      ivec2(width, height),
      ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
      vec2(u_start, v_start),
      vec2(u_end, v_end),
      static_cast<u32>(face->glyph->advance.x),
    };

    x += width + padding_x;
  }
  // write_texture_to_png("texture.png", font->texture_atlas, atlas_width, atlas_height);
  FT_Done_Face(face);
  FT_Done_FreeType(ft);

  return font;
}

auto TextRenderer::init(GLShaderProgram* program) -> void {
  _program = program;
  _vao.init();
  _vao.add_buffer(0, sizeof(f32) * 6 * 4, sizeof(f32) * 4);
  _vao.add_buffer_desc(0,
      GLBufferElementDescription{ .binding_index = 0, .size = 4, .offset_in_bytes = 0, .stride_in_bytes = 4 * sizeof(f32) });
  _vao.upload_buffer_desc();
}

auto TextRenderer::render(const char* text, const Font& font, f32 x, f32 y, f32 scale, const mat4& ortho_projection) -> void {
  assert(_program != nullptr);

  _vao.bind();
  _program->useProgram();
  _program->set_uniform("projection", ortho_projection);
  _program->set_uniform("text_color", vec3(0.8, 0.8, 0.8));
  gl->active_texture(GL_TEXTURE0);

  gl->enable(GL_BLEND);
  gl->disable(GL_DEPTH_TEST);
  gl->blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  gl->bind_texture(GL_TEXTURE_2D, font.texture_atlas);

  auto length = strlen(text);
  auto& characters = font.characters;
  for (auto i = 0; i < length; i++) {
    char c = text[i];
    if (c == '\0') {
      continue;
    }
    assert(c > 0 && c < characters.size());
    Character ch = characters[c];

    f32 x_pos = x + ch.bearing.x * scale;
    f32 y_pos = y - (ch.size.y - ch.bearing.y) * scale;

    f32 w = ch.size.x * scale;
    f32 h = ch.size.y * scale;

    f32 vertices[6][4] = {
      { x_pos, y_pos + h, ch.uv_start.x, ch.uv_start.y },
      { x_pos, y_pos, ch.uv_start.x, ch.uv_end.y },
      { x_pos + w, y_pos, ch.uv_end.x, ch.uv_end.y },

      { x_pos, y_pos + h, ch.uv_start.x, ch.uv_start.y },
      { x_pos + w, y_pos, ch.uv_end.x, ch.uv_end.y },
      { x_pos + w, y_pos + h, ch.uv_end.x, ch.uv_start.y },
    };
    // render glyph texture over quad
    // update content of VBO memory

    _vao.upload_buffer_data(0, vertices, 0, sizeof(vertices));
    // render quad
    gl->draw_arrays(GL_TRIANGLES, 0, 6);
    // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
    x += (ch.advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
  }
  gl->bind_vertex_array(0);
  gl->bind_texture(GL_TEXTURE_2D, 0);
  gl->disable(GL_BLEND);
  gl->disable(GL_DEPTH_TEST);
}

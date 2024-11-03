#include "gui.hpp"
#include "math/vec2.h"
#include "memory_arena.h"
#include "platform/types.h"
#include "text_renderer.h"

namespace im {

UIState m_state;
Font* _font;
mat4* _ortho;
MemoryArena* gui_permanent;
MemoryArena* gui_transient;

Array<RenderLayer> layers;
i32 g_old_layer_idx = -1;
i32 g_current_layer_idx = 0;
const i32 TOP_LAYER = 5; // TODO: Replace this with different contexts, or something

struct Window {
  i32 id;
  ivec2 position;
  ivec2 content_start;
  ivec2 content_end;
  ivec2 next_draw_point;
  ivec2 size;
};

Window active_window;

const f32 ui_scale = 0.6f;
const i32 NUM_LAYERS = 8;

auto is_in_rect(ivec2 pos, ivec2 start, ivec2 end) {
  return pos.x >= start.x && pos.x < end.x && pos.y >= end.y && pos.y < start.y;
}

auto initialize_imgui(Font* font, MemoryArena* permanent) -> void {
  _font = font;
  gui_permanent = permanent->allocate_arena(MegaBytes(1));
  gui_transient = permanent->allocate_arena(MegaBytes(1));

  layers.init(*gui_permanent, NUM_LAYERS);
  for (auto& layer : layers) {
    layer.vertices.init(*permanent, 4*1024);
    layer.indices.init(*permanent, 4*1024);
  }
  g_current_layer_idx = 0;
  m_state.hot_item = None_Hot_Items_id;
}

auto get_font_max_height() -> f32 {
  return font_str_dim("A", 1.0, *_font).y;
}

auto new_frame(i32 mouse_x, i32 mouse_y, bool mouse_down, mat4* ortho) -> void {
  m_state.mouse_pos = ivec2(mouse_x, mouse_y);
  m_state.mouse_down = mouse_down;

  g_current_layer_idx = 0;
  for (auto& layer : layers) {
    layer.vertices.empty();
    layer.indices.empty();
  }

  active_window.id = 0;

  gui_transient->clear();
  _ortho = ortho;
}

auto end_frame() -> void {
}

auto get_state() -> UIState {
  return m_state;
}
auto get_render_layers() -> Array<RenderLayer> {
  return layers;
}

auto set_to_top_layer() -> void {
  g_old_layer_idx = g_current_layer_idx;
  g_current_layer_idx = TOP_LAYER;
}

auto reset_layer() -> void {
  assert(g_current_layer_idx >= 0);
  assert(g_current_layer_idx <= NUM_LAYERS);
  g_current_layer_idx = g_old_layer_idx;
  g_old_layer_idx = -1;
}
const vec4 background_color = vec4(0.133, 0.239, 0.365, 1.0);
const vec4 active_color = vec4(0.233, 0.339, 0.465, 1.0);
const vec4 hot_color = vec4(0.033, 0.139, 0.265, 1.0);

// renders from left bottom to right top
auto render_text(const char* text, const Font& font, f32 x, f32 y, f32 scale, vec4 color, const mat4& ortho_projection) -> void {
  auto length = strlen(text);
  auto& characters = font.characters;
  auto* layer = &layers[g_current_layer_idx];
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
    // TODO: Cut off if they are outside of window
    auto index = layer->vertices.size();
    layer->vertices.push({ .position = vec2(x_pos, y_pos + h), .uv = vec2(ch.uv_start.x, ch.uv_start.y), .color = color }); // 1
    layer->vertices.push({ .position = vec2(x_pos, y_pos), .uv = vec2(ch.uv_start.x, ch.uv_end.y), .color = color }); // 2
    layer->vertices.push({ .position = vec2(x_pos + w, y_pos), .uv = vec2(ch.uv_end.x, ch.uv_end.y), .color = color }); // 3
    layer->vertices.push({ .position = vec2(x_pos + w, y_pos + h), .uv = vec2(ch.uv_end.x, ch.uv_start.y), .color = color }); // 4

    layer->indices.push(index);
    layer->indices.push(index + 1);
    layer->indices.push(index + 2);

    layer->indices.push(index);
    layer->indices.push(index + 2);
    layer->indices.push(index + 3);
    // render glyph texture over quad
    // update content of VBO memory

    // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
    x += (ch.advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
  }
}

auto text(const char* text, i32 x, i32 y, vec4& color, f32 scale) -> void {
  render_text(text, *_font, x, y, scale, color, *_ortho);
}

auto draw_rectangle(ivec2 start_in, ivec2 end_in, const vec4& color) {
  vec2 start = ivec2_to_vec2(start_in);
  vec2 end = ivec2_to_vec2(end_in);
  auto uv = vec2(0.0, 0.0);
  auto* current_layer = &layers[g_current_layer_idx];
  auto first_index = current_layer->vertices.size();
  current_layer->vertices.push({ .position = start, .uv = uv, .color = color });
  current_layer->vertices.push({ .position = vec2(start.x, end.y), .uv = uv, .color = color });
  current_layer->vertices.push({ .position = vec2(end.x, start.y), .uv = uv, .color = color });
  current_layer->vertices.push({ .position = end, .uv = uv, .color = color });

  current_layer->indices.push(first_index);
  current_layer->indices.push(first_index + 1);
  current_layer->indices.push(first_index + 2);

  current_layer->indices.push(first_index + 1);
  current_layer->indices.push(first_index + 3);
  current_layer->indices.push(first_index + 2);
}

auto rectangle(i32 id, i32 start_x, i32 start_y, i32 end_x, i32 end_y, const vec4& color) -> bool {

  ivec2 start = ivec2(start_x, start_y);
  ivec2 end{ end_x, end_y };

  if (is_in_rect(m_state.mouse_pos, start, end)) {

    m_state.hot_item = id;

    if (m_state.mouse_down) {
      m_state.active_item = id;
    }
  } else if (m_state.hot_item == id) {
    m_state.hot_item = None_Hot_Items_id;
  }

  bool was_clicked = !m_state.mouse_down && m_state.active_item == id;
  if (was_clicked) {
    m_state.active_item = None_Hot_Items_id;
  }

  if (m_state.active_item == id) {
    // color = active_color;
  } else if (m_state.hot_item == id) {
    // color = hot_color;
  }

  draw_rectangle(start, end, color);

  return was_clicked;
}

auto draw_rectangle(i32 x, i32 y, i32 width, i32 height, vec4 color) {
  draw_rectangle(ivec2(x, y), ivec2(x + width, y - height), color);
}

auto button(i32 id, const char* text, i32 x, i32 y) -> bool {
  ivec2 pos = ivec2(x, y);
  if (active_window.id != 0) {
    pos = active_window.next_draw_point;
  }

  const i32 padding = 15;
  const i32 margin = 15;

  auto text_dim = font_str_dim(text, ui_scale, *_font);
  ivec2 size = ivec2(text_dim.x + padding * 2, -(text_dim.y + padding * 2));
  ivec2 start = pos;
  ivec2 end = pos + size;

  auto color = background_color;
  if (is_in_rect(m_state.mouse_pos, start, end)) {
    m_state.hot_item = id;

    if (m_state.mouse_down) {
      m_state.active_item = id;
    }
  } else if (m_state.hot_item == id) {
    m_state.hot_item = None_Hot_Items_id;
  }

  bool was_clicked = !m_state.mouse_down && m_state.active_item == id;
  if (was_clicked) {
    m_state.active_item = None_Hot_Items_id;
  }

  if (m_state.active_item == id) {
    color = active_color;
  } else if (m_state.hot_item == id) {
    color = hot_color;
  }

  draw_rectangle(start, end, color);

  active_window.next_draw_point.y += (size.y - margin);
  active_window.content_end.x = fmax(end.x, active_window.content_end.x);
  active_window.content_end.y = fmin(end.y, active_window.content_end.y);

  vec4 text_color = vec4(0.7, 0.7, 0.7, 1.0);
  render_text(text, *_font, start.x + padding, start.y - padding - text_dim.y, ui_scale, text_color, *_ortho);

  return was_clicked;
}

auto window_begin(i32 id, const char* title, i32 x, i32 y) -> void {
  const i32 padding_x = 15;
  const i32 padding_y = 15;
  active_window.id = id;
  active_window.position.x = x;
  active_window.position.y = y;
  active_window.content_start = ivec2(x + padding_x, y - padding_y);
  active_window.next_draw_point = active_window.content_start;
  active_window.content_end = active_window.content_start;

  g_current_layer_idx++;
  assert(g_current_layer_idx < NUM_LAYERS);
}

auto window_end() -> void {
  const i32 padding_x = 15;
  const i32 padding_y = 15;
  assert(g_current_layer_idx > 0);
  g_current_layer_idx--;
  active_window.content_end.x += padding_x;
  active_window.content_end.y -= padding_y;
  draw_rectangle(active_window.position, active_window.content_end, vec4(0.1, 0.1, 0.1, 0.8));

  if (m_state.hot_item == None_Hot_Items_id && is_in_rect(m_state.mouse_pos, active_window.content_start, active_window.content_end)) {
    m_state.hot_item = active_window.id;
  }
  active_window.id = 0;
}
} // namespace im

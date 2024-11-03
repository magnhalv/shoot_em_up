#ifndef HOT_RELOAD_OPENGL_CLI_H
#define HOT_RELOAD_OPENGL_CLI_H

#include <platform/types.h>
#include <platform/user_input.h>

#include "../growable_string.h"
#include "../linked_list_buffer.h"
#include "../list.h"
#include "../text_renderer.h"

#include "cli_app.h"
#include "math/vec4.h"

struct CliSizes {
  static constexpr f32 padding_x = 5;
  static constexpr f32 line_margin = 5;

  f32 line_height;
  f32 response_area_height;
  f32 max_line_width;
  i32 max_num_lines;

  f32 width;
  f32 height;

  f32 scale;

  auto update(f32 _width, f32 _height) {
    max_line_width = _width - 2 * padding_x;
    // Margin from command line
    width = _width;
    height = _height;
    response_area_height = height - line_height - line_margin * 2;
    max_num_lines = static_cast<i32>(ceilf(response_area_height / line_height));
    scale = 0.4;
  }
};

struct Cli {
  static const i32 Max_Height = 200;
  static constexpr f32 Duration = 0.2f;

  auto handle_input(UserInput* input) -> void;
  auto init(MemoryArena* arena) -> void;
  auto update(i32 client_width, i32 client_height, f32 dt) -> void;
  /// Returns true if toggle enables cli
  auto toggle() -> bool;

  auto execute_command(FStr& command) -> void;

  f32 t;
  f32 m_current_y;
  f32 _progress;
  f32 m_active;
  f32 m_opened_height;

  MemoryArena* m_permanent_arena;

  GStr m_command_buffer;
  LinkedListBuffer m_response_buffer;

  List<CliApp> m_apps;
  CliSizes _sizes;
  TextRenderer* _text_renderer;
  Font* _font;

  // colors
  vec4 m_background;
  vec4 m_text_color;
};

#endif // HOT_RELOAD_OPENGL_CLI_H

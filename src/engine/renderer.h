#pragma once

#include <engine/engine.h>

struct RenderBasis {
  vec3 point;
};

struct RenderEntityBasis {
  RenderBasis *basic;
  vec2 offset;
};

enum RenderGroupEntryType {
  RenderCommands_RenderEntryClear,
  RenderCommands_RenderEntryDrawRectangle,
};

struct RenderGroupEntryHeader {
  RenderGroupEntryType type;
};

struct RenderEntryClear {
  vec4 color; // r,g,b,a
};

struct RenderEntryRectangle {
  RenderEntityBasis basis;
  f32 r, g, b, a;
  vec2 dim;
};

struct RenderGroup {
 
  RenderBasis *default_basis;
  f32 meters_to_pixels;

  u32 max_push_buffer_size;
  u32 push_buffer_size;
  u8 *push_buffer;
};


auto render(RenderGroup *group, i32 client_width, i32 client_height) -> void;





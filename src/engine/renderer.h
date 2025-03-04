#pragma once

#include <engine/engine.h>


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
  CoordSystem basis;
};

struct RenderGroup {
  f32 meters_to_pixels;

  u32 max_push_buffer_size;
  u32 push_buffer_size;
  u8 *push_buffer;
};


auto render(RenderGroup *group, i32 client_width, i32 client_height) -> void;





#pragma once

#include "glad/gl.h"
#include <platform/types.h>

struct GLBufferElementDescription {
  u32 binding_index;
  i32 size;
  i32 offset_in_bytes;
  i32 stride_in_bytes;
};

struct GLBuffer {
  const static u32 Max_Num_Descriptions = 5;

  u32 handle;
  u32 binding_index;
  GLsizeiptr size_in_bytes;
  GLsizeiptr offset_in_bytes;
  GLsizeiptr stride_in_bytes;
  GLbitfield flags;
  GLBufferElementDescription description[Max_Num_Descriptions];
  i32 num_descriptions;
};

struct GLElementBuffer {
  u32 handle;
  GLsizeiptr size;
};

struct GLVao {
  static const i32 Max_Buffers = 5;

  u32 handle;
  GLBuffer buffers[Max_Buffers];
  GLElementBuffer el_buffer;
  u32 num_buffers;

  auto init() -> void;

  auto destroy() -> void;

  auto bind() const -> void;

  auto add_buffer(i32 binding_index, GLsizeiptr size, GLsizeiptr stride_in_bytes, GLsizeiptr offset_in_bytes = 0,
      GLbitfield flags = 0) -> void;
  // TODO: Move to buffer, and make offset last argument with default = 0;
  auto upload_buffer_data(i32 buffer_idx, void* data, GLsizeiptr offset, GLsizeiptr size) -> void;
  auto upload_element_buffer_data(i32* data, i32 size) -> void;
  auto add_buffer_desc(u32 buffer_idx, u32 location, i32 size, i32 offset, i32 stride) -> void;
  auto add_buffer_desc(u32 buffer_idx, GLBufferElementDescription desc) -> void;
  auto upload_buffer_desc() -> void;
  auto set_element_buffer(i32 size) -> void;
};

#include "gl_vao.h"
#include "gl.h"

auto GLVao::init() -> void {
  num_buffers = 0;
  handle = 0;
  el_buffer.handle = 0;
  gl->create_vertex_arrays(1, &handle);
}

auto GLVao::destroy() -> void {
  gl->delete_vertex_array(1, &handle);
  gl->delete_buffers(1, &el_buffer.handle);
  gl->bind_vertex_array(0);
}

auto GLVao::bind() const -> void {
  gl->bind_vertex_array(handle);
}

auto GLVao::add_buffer(i32 binding_index, GLsizeiptr size, GLsizeiptr stride_in_bytes, GLsizeiptr offset_in_bytes,
    GLbitfield flags) -> void {
  assert(num_buffers < Max_Buffers);
  auto& buf = buffers[num_buffers++];
  buf.handle = 0;
  buf.binding_index = binding_index;
  buf.size_in_bytes = size;
  buf.stride_in_bytes = stride_in_bytes;
  buf.offset_in_bytes = offset_in_bytes;
  buf.flags = flags;
  buf.num_descriptions = 0;
}

auto GLVao::upload_buffer_data(i32 buffer_idx, void* data, GLsizeiptr offset_in_bytes, GLsizeiptr size_in_bytes) -> void {
  auto& buf = buffers[buffer_idx];
  assert((size_in_bytes + offset_in_bytes) <= buf.size_in_bytes);
  gl->named_buffer_sub_data(buf.handle, offset_in_bytes, size_in_bytes, data);
}

auto GLVao::add_buffer_desc(u32 buffer_idx, u32 binding_index, i32 size, i32 offset_in_bytes, i32 stride_in_bytes) -> void {
  assert(buffer_idx < num_buffers && buffer_idx >= 0);
  auto& buffer = buffers[buffer_idx];
  assert(buffer.num_descriptions < GLBuffer::Max_Num_Descriptions);
  auto& desc = buffer.description[buffer.num_descriptions++];
  desc.binding_index = binding_index;
  desc.size = size;
  desc.offset_in_bytes = offset_in_bytes;
  desc.stride_in_bytes = stride_in_bytes;
}

auto GLVao::add_buffer_desc(u32 buffer_idx, GLBufferElementDescription desc) -> void {
  assert(buffer_idx < num_buffers && buffer_idx >= 0);
  auto& buffer = buffers[buffer_idx];
  assert(buffer.num_descriptions < GLBuffer::Max_Num_Descriptions);
  buffer.description[buffer.num_descriptions++] = desc;
}

auto GLVao::upload_element_buffer_data(i32* data, i32 size) -> void {
  gl->named_buffer_sub_data(el_buffer.handle, 0, size, data);
}
auto GLVao::set_element_buffer(i32 size) -> void {
  el_buffer.size = size;
  gl->create_buffers(1, &el_buffer.handle);
  gl->named_buffer_storage(el_buffer.handle, el_buffer.size, nullptr, GL_DYNAMIC_STORAGE_BIT);
  gl->vertex_array_element_buffer(handle, el_buffer.handle);
}

auto GLVao::upload_buffer_desc() -> void {
  for (i32 i = 0; i < num_buffers; i++) {
    auto& buffer = buffers[i];
    gl->create_buffers(1, &buffer.handle);
    // TODO: Dont make every buffer dynamic
    gl->named_buffer_storage(buffer.handle, buffer.size_in_bytes, nullptr, GL_DYNAMIC_STORAGE_BIT);
    // Attach the buffer to the VAO
    gl->vertex_array_vertex_buffer(handle, buffer.binding_index, buffer.handle, buffer.offset_in_bytes, buffer.stride_in_bytes);
    for (i32 d = 0; d < buffer.num_descriptions; d++) {
      auto& desc = buffer.description[d];
      gl->enable_vertex_array_attrib(handle, desc.binding_index);
      gl->vertex_array_attrib_format(handle, desc.binding_index, desc.size, GL_FLOAT, GL_FALSE, desc.offset_in_bytes);
      gl->vertex_array_attrib_binding(handle, desc.binding_index, buffer.binding_index);
    }
  }
}

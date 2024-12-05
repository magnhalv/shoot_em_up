
#include <math/transform.h>
#include <math/vec2.h>

#include "gl/gl_shader.h"

// TODO: The sprites can probably share the vao, vbo, ebo etc.
struct Sprite {
  u32 vao = 0;
  u32 vbo = 0;
  u32 ebo = 0;
  u32 tex1 = 0;
  // TODO: Move this
  GLShaderProgram* program = nullptr;

  i32 width;
  i32 height;
};

auto load_sprite(const char* path, GLShaderProgram* program) -> Sprite;
auto render_sprite(const Sprite& sprite, const Transform& transform, const mat4& projection) -> void;

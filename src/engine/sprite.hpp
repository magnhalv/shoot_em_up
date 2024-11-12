
#include <math/transform.h>
#include <math/vec2.h>

#include "gl/gl_shader.h"

// TODO: The sprites can probably share the vao, vbo, ebo etc.
struct Sprite {
  u32 vao = 0;
  u32 vbo = 0;
  u32 ebo = 0;
  u32 tex1 = 0;
  GLShaderProgram* program = nullptr;

  Transform transform = {};
  vec2 speed;
};

auto load_sprite(const char* path, GLShaderProgram* program) -> Sprite;
auto render_sprite(Sprite& sprite, mat4 projection) -> void;

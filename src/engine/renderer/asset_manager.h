#ifndef HOT_RELOAD_OPENGL_ASSET_MANAGER_H
#define HOT_RELOAD_OPENGL_ASSET_MANAGER_H

#include <platform/types.h>

#include <engine/gl/gl_shader.h>

const i32 Max_Num_Shader_Programs = 7;

struct AssetManager {
  u64 num_shader_programs;
  GLShaderProgram shader_programs[Max_Num_Shader_Programs];

  auto update_if_changed() -> void {
    for (auto i = 0; i < num_shader_programs; i++) {
      shader_programs[i].relink_if_changed();
    }
  }
};

auto asset_manager_set_memory(void* memory) -> void;

extern AssetManager* asset_manager;

#endif // HOT_RELOAD_OPENGL_ASSET_MANAGER_H

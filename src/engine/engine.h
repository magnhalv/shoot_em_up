#pragma once

#include <math/mat4.h>
#include <math/vec4.h>
#include <platform/types.h>

#include "camera.h"
#include "cli/cli.h"
#include "framebuffer.h"
#include "gl/gl_shader.h"
#include "gl/gl_vao.h"
#include "memory_arena.h"
#include "options.hpp"
#include "text_renderer.h"

/// Processed mouse input
struct Pointer {
  f32 x = 0;
  f32 y = 0;

  auto update_pos(const MouseRaw& raw, i32 client_width, i32 client_height) -> void {
    // TODO: Sensitivity must be moved somewhere else
    const f32 sensitivity = 2.0;
    const f32 dx = static_cast<f32>(raw.dx) * sensitivity;
    const f32 dy = static_cast<f32>(raw.dy) * sensitivity;

    x = std::min(std::max(dx + x, 0.0f), static_cast<f32>(client_width));
    y = std::min(std::max(dy + y, 0.0f), static_cast<f32>(client_height));
  }
};

struct Window {
  f32 width;
  f32 height;
  mat4 ortho;
  mat4 perspective;
};

extern Options* graphics_options;

struct PerFrameData {
  mat4 projection;
  mat4 view;
  mat4 model;
};

struct TimeInfo {
  f32 dt; // in seconds
  u64 dt_ms;
  u64 t_ms;

  // Performance stuff. Improve when needed
  i32 num_frames;
  u64 second_counter;
  i32 fps;
};

enum class InputMode { Game = 0, Gui };

struct Entity {
  int id;
};

struct EngineState {
  Pointer pointer;
  bool is_initialized = false;
  Camera camera;
  MemoryArena transient;
  MemoryArena permanent;

  GLGlobalUniformBufferContainer uniform_buffer_container;
  Framebuffer framebuffer;
  MultiSampleFramebuffer ms_framebuffer;
  GLVao quad_vao{};

  Cli cli;
  bool is_cli_active;

  Options graphics_options;
  TimeInfo time;

  TextRenderer text_renderer;
  Font* font;
};

extern "C" __declspec(dllexport) void update_and_render(EngineMemory* memory, EngineInput* app_input);
extern "C" __declspec(dllexport) void load(GLFunctions* in_gl, Platform* in_platform, EngineMemory* in_memory);

#include <cmath>
#include <cstddef>
#include <cstdio>

#include <cstring>
#include <glad/gl.h>

#include <math/math.h>
#include <math/util.hpp>
#include <math/vec3.h>

#include "engine.h"
#include "engine/hm_assert.h"
#include "gl/gl.h"
#include "gl/gl_vao.h"
#include "globals.hpp"
#include "gui.hpp"
#include "logger.h"
#include "math/mat4.h"
#include "math/transform.h"
#include "memory_arena.h"
#include "platform/platform.h"
#include "text_renderer.h"
#include <engine/renderer/asset_manager.h>
#include <math/transform.h>

inline auto to_ndc(i32 pixels, i32 range) -> f32 {
  auto x = range / 2;
  return (static_cast<f32>(pixels) / static_cast<f32>(x)) - 1;
}

inline f32 new_x(f32 x, f32 y, f32 degree) {
  return cos(degree) * x - sin(degree) * y;
}

inline f32 new_y(f32 x, f32 y, f32 degree) {
  return sin(degree) * x + cos(degree) * y;
}

auto get_sound_samples(EngineMemory* memory, i32 num_samples) -> SoundBuffer {
  auto* engine = (EngineState*)memory->permanent;
  auto& audio = engine->audio;
  HM_ASSERT(num_samples >= 0);
  SoundBuffer buffer;
  buffer.tone_hz = 220;
  buffer.samples_per_second = 48000;
  buffer.sample_size_in_bytes = SoundSampleSize;
  buffer.num_samples = num_samples * buffer.sample_size_in_bytes;
  buffer.samples = allocate<u16>(*g_transient, buffer.num_samples);

  // Fill the buffer with a sine wave.
  uint32_t target_buf_idx = 0;
  for (auto& playing_sound : audio.playing_sounds) {
    if (playing_sound.sound == nullptr) {
      continue;
    }

    auto src_buffer = playing_sound.sound->samples;
    while (target_buf_idx < buffer.num_samples) {
      buffer.samples[target_buf_idx++] = src_buffer[playing_sound.curr_sample++];

      if (playing_sound.curr_sample >= playing_sound.sound->samples.size()) {
        playing_sound.sound = nullptr;
        playing_sound.curr_sample = 0;
        break;
      }
    }
  }

  // Fill remaining with zeros
  while (target_buf_idx < buffer.num_samples) {
    buffer.samples[target_buf_idx++] = 0;
  }

  return buffer;
}

// TODO: Handle change of screen width and height
void update_and_render(EngineMemory* memory, EngineInput* app_input) {
  auto* state = (EngineState*)memory->permanent;
  const f32 ratio = static_cast<f32>(app_input->client_width) / static_cast<f32>(app_input->client_height);

  // TODO: Move to renderer
  auto& quad_program = asset_manager->shader_programs[0];
  auto& font_program = asset_manager->shader_programs[1];
  auto& imgui_program = asset_manager->shader_programs[2];
  auto& single_color_program = asset_manager->shader_programs[3];
  auto& sprite_program = asset_manager->shader_programs[4];
  asset_manager->num_shader_programs = 5;

  // region Initialize
  [[unlikely]] if (!state->is_initialized) {
    log_info("Initializing...");

    state->permanent.init(
        static_cast<u8*>(memory->permanent) + sizeof(EngineState), Permanent_Memory_Block_Size - sizeof(EngineState));

    state->camera.init(-90.0f, -27.0f, vec3(0.0f, 5.0f, 10.0f));

    state->pointer.x = 200;
    state->pointer.y = 200;

    // region Compile shaders
    quad_program.initialize(R"(.\assets\shaders\quad.vert)", R"(.\assets\shaders\quad.frag)");
    font_program.initialize(R"(.\assets\shaders\font.vert)", R"(.\assets\shaders\font.frag)");
    imgui_program.initialize(R"(.\assets\shaders\imgui.vert)", R"(.\assets\shaders\imgui.frag)");
    single_color_program.initialize(R"(.\assets\shaders\basic_2d.vert)", R"(.\assets\shaders\single_color.frag)");
    sprite_program.initialize(R"(.\assets\shaders\sprite.vert)", R"(.\assets\shaders\sprite.frag)");

    state->framebuffer.init(app_input->client_width, app_input->client_height);
    state->ms_framebuffer.init(app_input->client_width, app_input->client_height);

    // endregion

    float quad_verticies[] = {
      // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
      // clang-format off
      // positions     // texCoords
      -1.0f, 1.0f,     0.0f, 1.0f,  
      -1.0f, -1.0f,    0.0f, 0.0f, 
      1.0f, -1.0f,     1.0f, 0.0f,  
      -1.0f, 1.0f,     0.0f, 1.0f,  
      1.0f, -1.0f,     1.0f, 0.0f,  
      1.0f, 1.0f,      1.0f, 1.0f,
      // clang-format on
    };

    state->quad_vao.init();
    state->quad_vao.bind();
    state->quad_vao.add_buffer(0, sizeof(quad_verticies), 4 * sizeof(f32));
    state->quad_vao.add_buffer_desc(0, 0, 2, 0, 4 * sizeof(f32));
    state->quad_vao.add_buffer_desc(0, 1, 2, 2 * sizeof(f32), 4 * sizeof(f32));
    state->quad_vao.upload_buffer_desc();
    state->quad_vao.upload_buffer_data(0, quad_verticies, 0, sizeof(quad_verticies));

    state->camera.update_cursor(0, 0);

    auto cli_memory_arena = state->permanent.allocate_arena(MegaBytes(1));
    state->font = font_load("assets/fonts/ubuntu/Ubuntu-Regular.ttf", state->permanent);
    state->text_renderer.init(&font_program);
    state->cli.init(cli_memory_arena);

    im::initialize_imgui(state->font, &state->permanent);

    state->player_sprite = load_sprite("assets/sprites/player_1.png", &sprite_program);
    state->player.transform = Transform();
    state->player.transform.scale.x = state->player_sprite.width;
    state->player.transform.scale.y = state->player_sprite.height;
    state->player.transform.scale.z = 1;

    state->projectile_sprite = load_sprite("assets/sprites/projectile_1.png", &sprite_program);
    for (auto& p : state->projectiles) {
      p.transform = Transform();
      p.transform.scale.x = state->projectile_sprite.width;
      p.transform.scale.y = state->projectile_sprite.height;
      p.transform.scale.z = 1;
    }

    init_audio_system(state->audio, state->permanent);

    state->is_initialized = true;
  }
  // TODO: Gotta set this on hot reload
  clear_transient();

  // endregion

#if ENGINE_DEBUG
  state->permanent.check_integrity();
  asset_manager->update_if_changed();
#endif

  auto& time = state->time;
  auto is_new_second = static_cast<i32>(time.t) < static_cast<i32>(time.t + app_input->dt);
  if (is_new_second) {
    time.fps = time.num_frames_this_second;
    time.num_frames_this_second = 0;
  }

  time.dt = app_input->dt;
  time.t += time.dt;
  time.num_frames_this_second++;

  auto ortho_projection = create_ortho(0, app_input->client_width, 0, app_input->client_height, 0.0f, 100.0f);

  Pointer* pointer = &state->pointer;
  const MouseRaw* mouse = &app_input->input.mouse_raw;

  // Update GUI
  im::new_frame(pointer->x, pointer->y, app_input->input.mouse_raw.left.ended_down, &ortho_projection);
  // Something something gui  im::new_frame(-1, -1, false, &ortho_projection);

  // Update CLI
  {
    // TODO: Need to rewrite CLI to use imgui
    if (app_input->input.oem_5.is_pressed_this_frame()) {
      state->is_cli_active = state->cli.toggle();
    }
    state->cli.handle_input(&app_input->input);
    state->cli.update(app_input->client_width, app_input->client_height, time.dt);
  }

  {
    im::window_begin(1, "My window", app_input->client_width - 200, app_input->client_height - 100);
    if (im::button(GEN_GUI_ID, "Gate")) {
      printf("My button");
    }

    im::window_end();

    auto debug_color = vec4(1.0, 1.0, 0, 1.0);
    char fps_text[50];
    sprintf(fps_text, "FPS: %d", time.fps);
    auto fps_text_dim = font_str_dim(fps_text, 0.3, *state->font);
    im::text(fps_text, app_input->client_width - fps_text_dim.x - 25, app_input->client_height - fps_text_dim.y - 25,
        debug_color, 0.3);
  }

  //////////////////////////////////
  ///////  Update gameplay   ///////
  //////////////////////////////////

  {
    auto& input = app_input->input;
    auto& p_pos = state->player.transform.position;

    if (input.space.is_pressed_this_frame()) {
      play_sound(SoundType::Laser, state->audio);
      const auto& pt = state->player.transform;
      for (auto& p : state->projectiles) {
        if (!p.is_active) {
          p.transform.position.x = pt.position.x + pt.scale.x / 2;
          p.transform.position.y = pt.position.y + pt.scale.y / 2;
          p.is_active = true;
          break;
        }
      }
    }

    const auto max_speed = 300.0;
    const f32 acc = 60.0f;
    vec2 accelaration = vec2(acc, acc);

    vec2 direction = vec2();

    auto speed = 1.0;
    if (input.w.ended_down) {
      direction.y = 1.0;
    }
    if (input.s.ended_down) {
      direction.y = -1.0;
    }
    if (input.a.ended_down) {
      direction.x = -1.0;
    }
    if (input.d.ended_down) {
      direction.x = 1.0;
    }
    normalize(direction);
    accelaration = accelaration * direction;

    auto& player = state->player;
    if (accelaration.x != 0.0) {
      const auto new_speed_x = player.speed.x + accelaration.x;
      player.speed.x = hm::min(hm::max(new_speed_x, -max_speed), max_speed);
    } else {
      if (player.speed.x < 0.0f) {
        player.speed.x = fmax(player.speed.x + acc, 0.0f);
      } else if (player.speed.x > 0.0f) {
        player.speed.x = fmin(player.speed.x - acc, 0.0f);
      }
    }

    if (accelaration.y != 0.0) {
      const auto new_speed_y = player.speed.y + accelaration.y;
      player.speed.y = hm::min(hm::max(new_speed_y, -max_speed), max_speed);
    } else {
      if (player.speed.y < 0.0f) {
        player.speed.y = fmax(player.speed.y + acc, 0.0f);
      } else if (player.speed.y > 0.0f) {
        player.speed.y = fmin(player.speed.y - acc, 0.0f);
      }
    }

    player.transform.position.x += (player.speed.x * time.dt);
    if (player.transform.position.x < 0) {
      player.transform.position.x = 0;
    }
    if ((player.transform.position.x + player.transform.scale.x) > app_input->client_width) {
      player.transform.position.x = app_input->client_width - player.transform.scale.x;
    }

    player.transform.position.y += (player.speed.y * time.dt);
    if (player.transform.position.y < 0) {
      player.transform.position.y = 0;
    }
    if ((player.transform.position.y + player.transform.scale.y) > app_input->client_height) {
      player.transform.position.y = app_input->client_height - player.transform.scale.y;
    }

    const auto projectile_speed = 1200.0;
    for (auto& p : state->projectiles) {
      if (p.is_active) {
        p.transform.position.y += projectile_speed * time.dt;
      }
    }
  }

  //////////////////////////////////
  ///////       Render       ///////
  //////////////////////////////////

  if (g_graphics_options->anti_aliasing) {
    gl->bind_framebuffer(GL_FRAMEBUFFER, state->ms_framebuffer.fbo);
  } else {
    gl->bind_framebuffer(GL_FRAMEBUFFER, state->framebuffer.fbo);
  }
  // region Render setup
  gl->enable(GL_DEPTH_TEST);
  gl->clear_color(0.1f, 0.1f, 0.1f, 0.2f);
  gl->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  gl->viewport(0, 0, app_input->client_width, app_input->client_height);

  // render sprite
  render_sprite(state->player_sprite, state->player.transform, ortho_projection);
  for (const auto& p : state->projectiles) {
    if (p.is_active) {
      render_sprite(state->projectile_sprite, p.transform, ortho_projection);
    }
  }

  // RenderGUI
  {
    gl->enable(GL_BLEND);
    gl->disable(GL_DEPTH_TEST);
    gl->blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl->bind_texture(GL_TEXTURE_2D, state->font->texture_atlas);
    single_color_program.useProgram();

    imgui_program.useProgram();
    imgui_program.set_uniform("projection", ortho_projection);

    auto layers = im::get_render_layers();
    GLVao vao;
    vao.init();
    vao.bind();

    // TODO: Remove 1024, im::gui needs to decide this
    // Allocate buffers
    vao.set_element_buffer(sizeof(i32) * 4 * 1024);
    auto total_size = 4 * 1024 * sizeof(im::DrawVert);
    auto stride = sizeof(im::DrawVert);
    vao.add_buffer(0, total_size, stride);
    vao.add_buffer_desc(0, 0, 2, offsetof(im::DrawVert, position), stride);
    vao.add_buffer_desc(0, 1, 2, offsetof(im::DrawVert, uv), stride);
    vao.add_buffer_desc(0, 2, 4, offsetof(im::DrawVert, color), stride);
    vao.upload_buffer_desc();

    auto i = 0;
    for (auto& layer : layers) {
      vao.upload_element_buffer_data(layer.indices.data(), sizeof(i32) * layer.indices.size());
      vao.upload_buffer_data(0, layer.vertices.data(), 0, sizeof(im::DrawVert) * layer.vertices.size());
      gl->draw_elements(GL_TRIANGLES, layer.indices.size(), GL_UNSIGNED_INT, 0);
      i++;
    }

    vao.destroy();
    gl->enable(GL_DEPTH_TEST);
    gl->disable(GL_BLEND);
  }

  // region Draw pointer
  {
    single_color_program.useProgram();
    single_color_program.set_uniform("color", vec4(0.7f, 0.7f, 0.7f, 0.7f));
    single_color_program.set_uniform("projection", ortho_projection);
    // TODO Fix this, worst implementation ever. Move it to GUI library
    f32 x = state->pointer.x;
    f32 y = state->pointer.y;
    f32 cursor_vertices[6] = {
      x, y,                                                         //
      x + new_x(-10.0f, -20.0f, 45.0f), y + new_y(-10, -20, 45.0f), //
      x + new_x(10.0f, -20.0f, 45.0f), y + new_y(10, -20, 45.0f),   //
    };
    GLVao cursor_vao{};
    cursor_vao.init();
    cursor_vao.bind();
    cursor_vao.add_buffer(0, sizeof(cursor_vertices), sizeof(vec2));
    cursor_vao.add_buffer_desc(0, 0, 2, 0, sizeof(vec2));
    cursor_vao.upload_buffer_desc();
    cursor_vao.upload_buffer_data(0, cursor_vertices, 0, sizeof(cursor_vertices));

    gl->draw_arrays(GL_TRIANGLES, 0, 3);
    cursor_vao.destroy();
  }
  // endregion

  // region Draw end quad
  {
    if (g_graphics_options->anti_aliasing) {
      const i32 w = app_input->client_width;
      const i32 h = app_input->client_height;
      gl->bind_framebuffer(GL_READ_FRAMEBUFFER, state->ms_framebuffer.fbo);
      gl->bind_framebuffer(GL_DRAW_FRAMEBUFFER, state->framebuffer.fbo);
      gl->framebuffer_blit(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    state->quad_vao.bind();
    quad_program.useProgram();
    gl->bind_framebuffer(GL_FRAMEBUFFER, 0);
    // TODO: Is this needed?
    gl->clear_color(1.0f, 1.0f, 1.0f, 1.0f);
    gl->clear(GL_COLOR_BUFFER_BIT);
    gl->disable(GL_DEPTH_TEST);

    gl->texture_bind(GL_TEXTURE_2D, state->framebuffer.texture);
    gl->draw_arrays(GL_TRIANGLES, 0, 6);

    gl->texture_bind(GL_TEXTURE_2D, 0);
  }
  // endregion

  // endregion

  gl->use_program(0);
  gl->bind_vertex_array(0);

  GLenum err;
  bool found_error = false;
  while ((err = gl->get_error()) != GL_NO_ERROR) {
    printf("OpenGL Error %d\n", err);
    found_error = true;
  }
  if (found_error) {
    exit(1);
  }
}

void load(GLFunctions* in_gl, Platform* in_platform, EngineMemory* in_memory) {
  load_gl(in_gl);
  load(in_platform);

  assert(sizeof(EngineState) < Permanent_Memory_Block_Size);
  auto* state = (EngineState*)in_memory->permanent;
  state->transient.init(in_memory->transient, Transient_Memory_Block_Size);
  set_transient_arena(&state->transient);

  assert(sizeof(AssetManager) <= Assets_Memory_Block_Size);
  asset_manager_set_memory(in_memory->asset);

  load(&state->graphics_options);
}

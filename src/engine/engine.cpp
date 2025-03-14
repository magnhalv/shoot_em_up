#include <cmath>
#include <cstddef>
#include <cstdio>

#include <glad/gl.h>

#include <math/math.h>
#include <math/util.hpp>
#include <math/vec3.h>

#include "engine.h"
#include "engine/audio.hpp"
#include "engine/gameplay.h"
#include "engine/hm_assert.h"
#include <engine/renderer.h>
#include "engine/structs/swap_back_list.h"
#include "gl/gl.h"
#include "gl/gl_vao.h"
#include "globals.hpp"
#include "gui.hpp"
#include "logger.h"
#include "math/mat4.h"
#include "math/quat.h"
#include "math/transform.h"
#include "memory_arena.h"
#include "platform/platform.h"
#include "platform/types.h"
#include "text_renderer.h"
#include <engine/renderer/asset_manager.h>
#include <math/transform.h>

typedef struct {
  vec2 p;
  vec2 dim;
} Rect;


auto rect_center(Rect *rect) -> vec2 {
  return rect->p + 0.5*rect->dim; 
}

// Only works without rotation
auto rect_to_quadrilateral(Rect *rect) -> Quadrilateral {
  Quadrilateral result = {};
  result.bl = vec2(0,0);
  result.tl = vec2(0, rect->dim.y);
  result.tr = rect->dim;
  result.br = vec2(rect->dim.x, 0);
  return result;
}

auto point_rotate(vec2 p, f32 theta) -> vec2 {
  vec2 x_axis = {cos(theta), -sin(theta)};
  vec2 y_axis = {sin(theta), cos(theta)};

  return {
    p.x * x_axis.x + p.y * x_axis.y, 
    p.x * y_axis.x + p.y * y_axis.y
  };
}

struct EnemyBehaviour {
  vec2 spawn_point;
  vec2 center_point;
};

inline auto to_ndc(i32 pixels, i32 range) -> f32 {
  auto x = range / 2;
  return (static_cast<f32>(pixels) / static_cast<f32>(x)) - 1;
}

inline f32 rotate_x(f32 x, f32 y, f32 degree) {
  return cos(degree) * x - sin(degree) * y;
}

inline f32 rotate_y(f32 x, f32 y, f32 degree) {
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
  // Shouldnt this be just num_samples??
  buffer.num_samples = num_samples;
  buffer.samples = allocate<i16>(*g_transient, buffer.num_samples);
  for (auto i = 0; i < buffer.num_samples; i++) {
    buffer.samples[i] = 0;
  }

  for (auto& playing_sound : audio.playing_sounds) {

    uint32_t target_buf_idx = 0;
    auto src_buffer = playing_sound.sound->samples;
    auto start_src_idx = playing_sound.curr_sample;
    auto end_src_idx = playing_sound.sound->samples.size();
    end_src_idx = hm::min(start_src_idx + buffer.num_samples, playing_sound.sound->samples.size());
    while (start_src_idx < end_src_idx) {
      // NOTE: We need to handle this better when we get more sounds
      buffer.samples[target_buf_idx++] += (src_buffer[start_src_idx++] / 4);
    }
    playing_sound.curr_sample = end_src_idx;
  }

  return buffer;
}

auto create_spaceship(Sprite* sprite) -> SpaceShip {
  SpaceShip result;
  result.transform = Transform();
  result.transform.scale.x = sprite->width;
  result.transform.scale.y = sprite->height;
  result.transform.scale.z = 1;
  result.sprite = sprite;
  return result;
}

auto sine_behaviour(f32 x) {
  return sin(x);
}

auto noise(f32 x) {
  return sin(2 * x) + sin(PI * x);
}

auto spawn_enemy(Sprite* sprite, SwapBackList<SpaceShip>& enemies, i32 max_width, i32 max_height) {
  SpaceShip enemy = create_spaceship(sprite);
  enemy.transform.position.x = 200;
  enemy.transform.position.y = max_height + sprite->height;
  enemy.transform.scale.x = sprite->width;
  enemy.transform.scale.y = sprite->height;
  enemy.original_x = 200;
  enemy.progress = 0;
  enemy.transform.rotation = fromTo(vec3(0, 1, 0), vec3(0, -1, 0));
  enemies.push(enemy);
}

#define PushRenderElement(group, type) (type*) push_render_element_(group, sizeof(type), RenderCommands_##type)
auto push_render_element_(RenderGroup *render_group, u32 size, RenderGroupEntryType type) {
  void *result = 0;

  size += sizeof(RenderGroupEntryHeader);
  if ((render_group->push_buffer_size + size) < render_group->max_push_buffer_size) {
    RenderGroupEntryHeader *header = (RenderGroupEntryHeader*)(render_group->push_buffer + render_group->push_buffer_size);
    header->type = type;
    result = (u8*)(header) + sizeof(*header);
    render_group->push_buffer_size += size;
  }
  else {
    printf("Invalid\n");
    // invalid code path
  }
  return result;
}

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

    renderer_init();
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
    state->player = create_spaceship(&state->player_sprite);

    state->projectile_sprite = load_sprite("assets/sprites/projectile_1.png", &sprite_program);
    state->player_projectiles.init(state->permanent, 100);

    state->enemy_sprite = load_sprite("assets/sprites/blue_01.png", &sprite_program);
    state->enemy_chargers.init(state->permanent, 30);

    SpaceShip enemy = create_spaceship(&state->enemy_sprite);
    enemy.transform.position.x = 200;
    enemy.transform.position.y = 600;
    state->enemy_chargers.push(enemy);

    state->explosion_sprites[0] = load_sprite("assets/sprites/explosion/explosion-1.png", &sprite_program);
    state->explosion_sprites[1] = load_sprite("assets/sprites/explosion/explosion-2.png", &sprite_program);
    state->explosion_sprites[2] = load_sprite("assets/sprites/explosion/explosion-3.png", &sprite_program);
    state->explosion_sprites[3] = load_sprite("assets/sprites/explosion/explosion-4.png", &sprite_program);
    state->explosion_sprites[4] = load_sprite("assets/sprites/explosion/explosion-5.png", &sprite_program);
    state->explosion_sprites[5] = load_sprite("assets/sprites/explosion/explosion-6.png", &sprite_program);
    state->explosion_sprites[6] = load_sprite("assets/sprites/explosion/explosion-7.png", &sprite_program);
    state->explosion_sprites[7] = load_sprite("assets/sprites/explosion/explosion-8.png", &sprite_program);
    state->explosions.init(state->permanent, 30);

    init_audio_system(state->audio, state->permanent);

    state->is_initialized = true;
    log_info("Initializing complete");
  }
  // TODO: Gotta set this on hot reload
  clear_transient();
  remove_finished_sounds(state->audio);

  // endregion

#if ENGINE_DEBUG
  state->permanent.check_integrity();
  asset_manager->update_if_changed();
#endif

  const vec2 screen_center = vec2(app_input->client_width / 2.0, app_input->client_height / 2.0);

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

  RenderGroup group {};
  group.push_buffer_size = 0;
  group.max_push_buffer_size = MegaBytes(4);
  group.push_buffer = allocate<u8>(*g_transient, group.max_push_buffer_size);
  group.screen_width = app_input->client_width;
  group.screen_height = app_input->client_height;

  auto* clear = PushRenderElement(&group, RenderEntryClear);
  clear->color = vec4(0.2, 0.4, 0.2, 0.2);

  
  Rect rect = {};
  rect.dim = screen_center;
  rect.p = screen_center - 0.5*rect.dim;

  Quadrilateral quad = rect_to_quadrilateral(&rect);

  auto* render_quad = PushRenderElement(&group, RenderEntryQuadrilateral);
  render_quad->color = vec4(1.0, 0.4, 0.2, 0.2);
  render_quad->quad = quad;
  render_quad->local_origin = 0.5*rect.dim;
  render_quad->offset = screen_center - 0.5*rect.dim;
  render_quad->offset = screen_center - 0.5*rect.dim;
  f32 theta = app_input->t;
  render_quad->basis.x = vec2(cos(theta), -sin(theta));
  render_quad->basis.y = vec2(sin(theta), cos(theta));
  render(&group, app_input->client_width, app_input->client_height);


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

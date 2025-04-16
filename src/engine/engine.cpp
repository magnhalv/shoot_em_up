#include <cmath>
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
#include "gl/gl.h"
#include "globals.hpp"
#include "gui.hpp"
#include "logger.h"
#include "math/vec2.h"
#include "memory_arena.h"
#include "platform/platform.h"
#include "platform/types.h"
#include "text_renderer.h"
#include <engine/renderer/asset_manager.h>
#include <math/transform.h>


// Only works without rotation
auto rect_to_quadrilateral(const vec2 &p, const vec2 &dim) -> Quadrilateral {
  Quadrilateral result = {};
  result.bl = vec2(0,0);
  result.tl = vec2(0, dim.y);
  result.tr = dim;
  result.br = vec2(dim.x, 0);
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

auto sine_behaviour(f32 x) {
  return sin(x);
}

auto noise(f32 x) {
  return sin(2 * x) + sin(PI * x);
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


  // region Initialize
  [[unlikely]] if (!state->is_initialized) {
    log_info("Initializing...");

    state->permanent.init(
        static_cast<u8*>(memory->permanent) + sizeof(EngineState), Permanent_Memory_Block_Size - sizeof(EngineState));


    state->pointer.x = 200;
    state->pointer.y = 200;

    renderer_init();

    // endregion
    auto cli_memory_arena = state->permanent.allocate_arena(MegaBytes(1));
    state->font = font_load("assets/fonts/ubuntu/Ubuntu-Regular.ttf", state->permanent);
    state->cli.init(cli_memory_arena);

    im::initialize_imgui(state->font, &state->permanent);

    state->player_bitmap = load_bitmap("assets/sprites/player_1.png", &state->permanent);
    state->player.sprite_handle = renderer_add_texture(state->player_bitmap);
    state->player.dim = vec2(state->player_bitmap->width, state->player_bitmap->height);
    state->player.p = vec2(100, 100);
    state->player.deg = 0.0f;

    state->projectile_bitmap = load_bitmap("assets/sprites/projectile_1.png", &state->permanent);
    state->projectile_bitmap_handle = renderer_add_texture(state->projectile_bitmap);
    state->player_projectiles.init(state->permanent, 100);

    //state->enemy_sprite = load_sprite("assets/sprites/blue_01.png", &sprite_program);


    
    //state->explosion_sprites[0] = load_sprite("assets/sprites/explosion/explosion-1.png", &sprite_program);
    //state->explosion_sprites[1] = load_sprite("assets/sprites/explosion/explosion-2.png", &sprite_program);
    //state->explosion_sprites[2] = load_sprite("assets/sprites/explosion/explosion-3.png", &sprite_program);
    //state->explosion_sprites[3] = load_sprite("assets/sprites/explosion/explosion-4.png", &sprite_program);
    //state->explosion_sprites[4] = load_sprite("assets/sprites/explosion/explosion-5.png", &sprite_program);
    //state->explosion_sprites[5] = load_sprite("assets/sprites/explosion/explosion-6.png", &sprite_program);
    //state->explosion_sprites[6] = load_sprite("assets/sprites/explosion/explosion-7.png", &sprite_program);
    //state->explosion_sprites[7] = load_sprite("assets/sprites/explosion/explosion-8.png", &sprite_program);
    //state->explosions.init(state->permanent, 30);

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

 {
    auto& input = app_input->input;
    auto& p_pos = state->player.p;

    state->enemy_timer += app_input->dt;
//    if (state->enemy_timer > 0.3) {
//      state->enemy_timer = 0;
//      spawn_enemy(&state->enemy_sprite, state->enemies, app_input->client_width, app_input->client_height);
//    }

    if (input.space.is_pressed_this_frame()) {
      play_sound(SoundType::Laser, state->audio);
      auto pos = state->player.p;
      pos.y = pos.y + state->player.dim.y;
      pos.x = pos.x + 0.5*state->player.dim.x - 0.5*state->projectile_bitmap->width;
      Projectile p{.p = pos };
      state->player_projectiles.push(p);
    }

    const auto max_speed = 300.0;
    const f32 acc = 60.0f;

    vec2 direction = {};

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
    vec2 accelaration = direction * acc;

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

    if (mag(player.speed) > max_speed) {
      player.speed = normalized(player.speed) * max_speed;
    }

    player.p = update_position(player.p, player.speed, player.dim, time.dt, app_input->client_width, app_input->client_height);

    
    const auto projectile_speed = 1200.0;
    for (auto& proj : state->player_projectiles) {
      proj.p.y += projectile_speed * time.dt;
    }

    /*
    for (auto e = 0; e < state->explosions.size(); e++) {
      auto& ex = state->explosions[e];
      ex.curr_frame++;
      if (ex.curr_frame > ex.frames_per_sprite) {
        ex.curr_frame = 0;
        ex.curr_sprite++;
      }
      if (ex.curr_sprite >= ex.num_sprites) {
        state->explosions.remove(e);
        e--;
      }
    }

    */
    /*
    // Update enemies
    for (auto& enemy : state->enemies) {
      enemy.progress += 0.04f;
      enemy.speed.y = -300.0f;
      enemy.speed.x = 250.0f * sine_behaviour(enemy.progress);
      enemy.transform = update_position(enemy.transform, enemy.speed, time.dt);
    }

    */
    // Update projectile
    for (auto i = 0; i < state->player_projectiles.size(); i++) {
      auto pos = state->player_projectiles[i].p;
      {
        vec2 bottom_left = vec2(0, 0);
        vec2 top_right = vec2(app_input->client_width, app_input->client_height);
        if (!hmath::in_rect(pos, bottom_left, top_right)) {
          state->player_projectiles.remove(i);
          i--;
          continue;
        }
      }
    }

    /*
      for (auto e = 0; e < state->enemies.size(); e++) {
        auto& enemy = state->enemies[e];
        vec2 bottom_left = enemy.transform.position.xy();
        vec2 top_right = bottom_left + enemy.transform.scale.xy();
        if (hmath::in_rect(pos, bottom_left, top_right)) {

          Explosion ex{};
          ex.transform = Transform();
          ex.transform.scale.x = state->explosion_sprites[0].width;
          ex.transform.scale.y = state->explosion_sprites[0].height;
          ex.transform.position.x = enemy.transform.position.x;
          ex.transform.position.y = enemy.transform.position.y;
          ex.num_sprites = 8;
          ex.frames_per_sprite = 5;
          state->explosions.push(ex);
          play_sound(SoundType::Explosion, state->audio);

          state->enemies.remove(e);
          state->player_projectiles.remove(i);
          i--;
          break;
        }
      }
    }
    */
  }

  ///////////////////////////
  //// Rendering ////////////
  ///////////////////////////
  RenderGroup group {};
  group.push_buffer_size = 0;
  group.max_push_buffer_size = MegaBytes(4);
  group.push_buffer = allocate<u8>(*g_transient, group.max_push_buffer_size);
  group.screen_width = app_input->client_width;
  group.screen_height = app_input->client_height;

  auto* clear = PushRenderElement(&group, RenderEntryClear);
  clear->color = vec4(0.0, 0.0, 0.0, 0.0);

  {
    const auto &player = state->player;
    auto* render_bm = PushRenderElement(&group, RenderEntryBitmap);
    render_bm->quad = rect_to_quadrilateral(player.p, player.dim);
    render_bm->local_origin = 0.5*player.dim;
    render_bm->offset = player.p;
    render_bm->basis.x = vec2(cos(player.deg), -sin(player.deg));
    render_bm->basis.y = vec2(sin(player.deg), cos(player.deg));
    render_bm->bitmap_handle = state->player.sprite_handle;
  }

  for (auto &proj : state->player_projectiles) {
    auto &sprite = state->projectile_bitmap;
    const vec2 dim = vec2(sprite->width, sprite->height);
    const f32 deg = 0.0f;
    auto &handle = state->projectile_bitmap_handle;

    auto* rendel_el = PushRenderElement(&group, RenderEntryBitmap);
    rendel_el->quad = rect_to_quadrilateral(proj.p, dim);
    rendel_el->local_origin = 0.5*dim;
    rendel_el->offset = proj.p;
    rendel_el->basis.x = vec2(cos(deg), -sin(deg));
    rendel_el->basis.y = vec2(sin(deg), cos(deg));
    rendel_el->bitmap_handle = handle;
  }

  render(&group, app_input->client_width, app_input->client_height);
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

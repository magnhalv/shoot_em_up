
#include <cmath>
#include <cstdio>

#include "engine/assets.h"
#include "platform/platform.h"

#include <math/math.h>
#include <math/util.hpp>
#include <math/vec3.h>

#include "engine.h"
#include "engine/audio.hpp"
#include "engine/gameplay.h"
#include "engine/hm_assert.h"
#include "globals.hpp"
#include "logger.h"
#include "math/vec2.h"
#include "memory_arena.h"
#include "platform/types.h"
#include <math/transform.h>

#include <renderers/renderer.hpp>

// Only works without rotation
auto rect_to_quadrilateral(const vec2& p, const vec2& dim) -> Quadrilateral {
    Quadrilateral result = {};
    result.bl = vec2(0, 0);
    result.tl = vec2(0, dim.y);
    result.tr = dim;
    result.br = vec2(dim.x, 0);
    return result;
}

auto point_rotate(vec2 p, f32 theta) -> vec2 {
    vec2 x_axis = { cos(theta), -sin(theta) };
    vec2 y_axis = { sin(theta), cos(theta) };

    return { p.x * x_axis.x + p.y * x_axis.y, p.x * y_axis.x + p.y * y_axis.y };
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
    auto* audio = &engine->audio;
    auto* game_assets = engine->assets;
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

    for (auto& ps : audio->playing_sounds) {
        if (ps.audio == nullptr) {
            LoadedAudio* audio = get_audio(game_assets, ps.id);
            if (audio) {
                ps.audio = audio;
            }
            else {
                load_audio(game_assets, ps.id);
            }
        }
    }

    for (auto& ps : audio->playing_sounds) {
        if (ps.audio != nullptr) {
            uint32_t target_buf_idx = 0;
            i16* src_buffer = (i16*)ps.audio->data;
            auto start_src_idx = ps.curr_sample;
            auto end_src_idx = ps.audio->sample_count;
            end_src_idx = hm::min(start_src_idx + buffer.num_samples, ps.audio->sample_count);
            while (start_src_idx < end_src_idx) {
                // NOTE: We need to handle this better when we get more sounds
                buffer.samples[target_buf_idx++] += (src_buffer[start_src_idx++] / 4);
            }

            /*i16 max = 0;*/
            /*for (auto i = 0; i < target_buf_idx; i++) {*/
            /*    if (max < buffer.samples[i]) {*/
            /*        max = buffer.samples[i];*/
            /*    }*/
            /*}*/
            /*printf("%d\n", max);*/
            ps.curr_sample = end_src_idx;
        }
    }

    return buffer;
}

auto sine_behaviour(f32 x) {
    return sin(x);
}

auto sine_movement(f32 base, f32 amp, f32 frequency, f32 time) {
    return base + amp * sin(frequency * time);
}
auto noise(f32 x) {
    return sin(2 * x) + sin(PI * x);
}

#define PushRenderElement(group, type) (type*)push_render_element_(group, sizeof(type), RenderCommands_##type)
auto push_render_element_(RenderGroup* render_group, u32 size, RenderGroupEntryType type) {
    void* result = 0;

    size += sizeof(RenderGroupEntryHeader);
    if ((render_group->push_buffer_size + size) < render_group->max_push_buffer_size) {
        RenderGroupEntryHeader* header = (RenderGroupEntryHeader*)(render_group->push_buffer + render_group->push_buffer_size);
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

void update_and_render(EngineMemory* memory, EngineInput* app_input, RenderGroup* group) {
    auto* state = (EngineState*)memory->permanent;
    const f32 ratio = static_cast<f32>(app_input->client_width) / static_cast<f32>(app_input->client_height);

    // region Initialize
    [[unlikely]] if (!state->is_initialized) {
        log_info("Initializing...");

        state->permanent.init(static_cast<u8*>(memory->permanent) + sizeof(EngineState),
            Permanent_Memory_Block_Size - sizeof(EngineState));

        state->task_system.queue = memory->work_queue;
        TaskSystem* task_system = &state->task_system;
        for (auto i = 0; i < array_length(task_system->tasks); i++) {
            TaskWithMemory* task = task_system->tasks + i;
            task->memory = state->permanent.allocate_arena(KiloBytes(1));
        }

        state->pointer.x = 200;
        state->pointer.y = 200;

        // endregion
        auto cli_memory_arena = state->permanent.allocate_arena(MegaBytes(1));
        // state->font = font_load("assets/fonts/ubuntu/Ubuntu-Regular.ttf", state->permanent);
        // state->cli.init(cli_memory_arena);

        // im::initialize_imgui(state->font, &state->permanent);

        state->assets = initialize_game_assets(&state->permanent);

        auto bitmap_id = get_first_bitmap_id(state->assets, Asset_PlayerSpaceShip);
        auto player = get_bitmap_meta(state->assets, bitmap_id);
        state->player.dim = vec2(player.dim[0], player.dim[1]);
        state->player.P = vec2(100, 100);
        state->player.direction = 0.0f;

        state->player_projectiles.init(state->permanent, 100);
        state->enemy_chargers.init(state->permanent, 100);

        // state->explosion_sprites[0] = load_sprite("assets/sprites/explosion/explosion-1.png", &sprite_program);
        // state->explosion_sprites[1] = load_sprite("assets/sprites/explosion/explosion-2.png", &sprite_program);
        // state->explosion_sprites[2] = load_sprite("assets/sprites/explosion/explosion-3.png", &sprite_program);
        // state->explosion_sprites[3] = load_sprite("assets/sprites/explosion/explosion-4.png", &sprite_program);
        // state->explosion_sprites[4] = load_sprite("assets/sprites/explosion/explosion-5.png", &sprite_program);
        // state->explosion_sprites[5] = load_sprite("assets/sprites/explosion/explosion-6.png", &sprite_program);
        // state->explosion_sprites[6] = load_sprite("assets/sprites/explosion/explosion-7.png", &sprite_program);
        // state->explosion_sprites[7] = load_sprite("assets/sprites/explosion/explosion-8.png", &sprite_program);
        // state->explosions.init(state->permanent, 30);

        init_audio_system(&state->audio, &state->permanent);

        state->is_initialized = true;
        log_info("Initializing complete");
    }

    // TODO: Gotta set this on hot reload
    clear_transient();
    remove_finished_sounds(&state->audio);

    // endregion

#if ENGINE_DEBUG
    state->permanent.check_integrity();
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

    // -------------- PLAYER INPUT ------------------
    {
        auto& input = app_input->input;
        auto& p_pos = state->player.P;

        state->enemy_timer += app_input->dt;
        if (state->enemy_timer > 0.3) {
            state->enemy_timer = 0;

            auto enemy_meta = get_first_bitmap_meta(state->assets, Asset_EnemySpaceShip);
            Entity enemy = {};
            enemy.P = vec2(100.0, app_input->client_height);
            enemy.dim.x = enemy_meta.dim[0];
            enemy.dim.y = enemy_meta.dim[1];
            enemy.direction = PI;

            state->enemy_chargers.push(enemy);
        }

        if (input.space.is_pressed_this_frame()) {

            AudioId audio_id = get_first_audio(state->assets, Asset_Laser);
            play_audio(&state->audio, audio_id);

            auto proj_meta = get_first_bitmap_meta(state->assets, Asset_Projectile);
            auto pos = state->player.P;
            pos.y = pos.y + state->player.dim.y;
            pos.x = pos.x + 0.5 * state->player.dim.x - 0.5 * proj_meta.dim[0];

            Entity p = {};
            p.P = pos;
            p.dim.x = proj_meta.dim[0];
            p.dim.y = proj_meta.dim[1];

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
        }
        else {
            if (player.speed.x < 0.0f) {
                player.speed.x = fmax(player.speed.x + acc, 0.0f);
            }
            else if (player.speed.x > 0.0f) {
                player.speed.x = fmin(player.speed.x - acc, 0.0f);
            }
        }

        if (accelaration.y != 0.0) {
            const auto new_speed_y = player.speed.y + accelaration.y;
            player.speed.y = hm::min(hm::max(new_speed_y, -max_speed), max_speed);
        }
        else {
            if (player.speed.y < 0.0f) {
                player.speed.y = fmax(player.speed.y + acc, 0.0f);
            }
            else if (player.speed.y > 0.0f) {
                player.speed.y = fmin(player.speed.y - acc, 0.0f);
            }
        }

        if (mag(player.speed) > max_speed) {
            player.speed = normalized(player.speed) * max_speed;
        }
    }

    // -------------- UPDATE GAME STATE -----------------
    {
        auto& player = state->player;
        player.P = update_position(player.P, player.speed, player.dim, time.dt, app_input->client_width, app_input->client_height);

        const auto projectile_speed = 1200.0;
        for (auto& proj : state->player_projectiles) {
            proj.P.y += projectile_speed * time.dt;
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

        // Update enemies
        for (auto i = 0; i < state->enemy_chargers.size();) {
            auto& enemy = state->enemy_chargers[i];
            enemy.P.y += -400.0f * time.dt;
            enemy.progress += time.dt;
            enemy.P.x = sine_movement(100.0, 100.0, 3.0, enemy.progress);

            if (enemy.P.y + enemy.dim.y <= 0.0) {
                state->enemy_chargers.remove(i);
            }
            else {
                i++;
            }
        }

        // Update projectile
        for (size_t i = 0; i < state->player_projectiles.size(); i++) {
            auto* projectile = &state->player_projectiles[i];
            vec2 bottom_left = vec2(0, 0);
            vec2 top_right = vec2(app_input->client_width, app_input->client_height);
            if (!hmath::in_rect(projectile->P, bottom_left, top_right)) {
                state->player_projectiles.remove_safe(&i);
            }
            else {
                hmath::Rect projectile_rect = hmath::create_rect(projectile->P, projectile->dim);

                for (size_t enemy_idx = 0; enemy_idx < state->enemy_chargers.size(); enemy_idx++) {
                    auto enemy = state->enemy_chargers[enemy_idx];
                    auto enemy_rect = hmath::create_rect(enemy.P, enemy.dim);

                    if (hmath::intersects(projectile_rect, enemy_rect)) {

                        auto id = get_first_bitmap_id(state->assets, Asset_EnemySpaceShip);
                        auto enemy_bitmap = get_bitmap(state->assets, id);

                        id = get_first_bitmap_id(state->assets, Asset_Projectile);
                        auto projectile_bitmap = get_bitmap(state->assets, id);

                        if (enemy_bitmap != nullptr && projectile_bitmap != nullptr) {
                        }
                        state->enemy_chargers.remove_safe(&enemy_idx);
                        state->player_projectiles.remove_safe(&i);
                        break;
                    }
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
    group->push_buffer_size = 0;
    group->max_push_buffer_size = MegaBytes(4);
    group->push_buffer = allocate<u8>(*g_transient, group->max_push_buffer_size);
    group->screen_width = app_input->client_width;
    group->screen_height = app_input->client_height;

    auto* clear = PushRenderElement(group, RenderEntryClear);
    clear->color = vec4(0.0, 0.0, 0.0, 0.0);

    {
        auto player_bitmap_id = get_first_bitmap_id(state->assets, Asset_PlayerSpaceShip);
        auto bitmap = get_bitmap(state->assets, player_bitmap_id);
        if (bitmap) {
            if (bitmap->texture_handle == 0) {
                bitmap->texture_handle = Platform->render_api->add_texture(bitmap->data, bitmap->width, bitmap->height);
            }

            const auto& player = state->player;
            auto* render_bm = PushRenderElement(group, RenderEntryBitmap);
            render_bm->quad = rect_to_quadrilateral(player.P, player.dim);
            render_bm->local_origin = 0.5 * player.dim;
            render_bm->offset = player.P;
            render_bm->basis.x = vec2(cos(player.direction), -sin(player.direction));
            render_bm->basis.y = vec2(sin(player.direction), cos(player.direction));
            render_bm->bitmap_handle = bitmap->texture_handle;
        }
    }

    for (auto& enemy : state->enemy_chargers) {

        auto bitmap_id = get_first_bitmap_id(state->assets, Asset_EnemySpaceShip);
        auto bitmap = get_bitmap(state->assets, bitmap_id);
        if (bitmap) {
            if (bitmap->texture_handle == 0) {
                bitmap->texture_handle = Platform->render_api->add_texture(bitmap->data, bitmap->width, bitmap->height);
            }
            auto* render_el = PushRenderElement(group, RenderEntryBitmap);
            render_el->quad = rect_to_quadrilateral(enemy.P, enemy.dim);
            render_el->local_origin = 0.5 * enemy.dim;
            render_el->offset = enemy.P;
            render_el->basis.x = vec2(cos(enemy.direction), -sin(enemy.direction));
            render_el->basis.y = vec2(sin(enemy.direction), cos(enemy.direction));
            render_el->bitmap_handle = bitmap->texture_handle;
            render_el->color = vec4(1.0, 0, 0, 0);
        }
    }

    for (auto& proj : state->player_projectiles) {
        auto bitmap_id = get_first_bitmap_id(state->assets, Asset_Projectile);
        auto bitmap = get_bitmap(state->assets, bitmap_id);
        if (bitmap) {
            if (bitmap->texture_handle == 0) {
                bitmap->texture_handle = Platform->render_api->add_texture(bitmap->data, bitmap->width, bitmap->height);
            }
            const vec2 dim = vec2(bitmap->width, bitmap->height);
            const f32 deg = 0.0f;

            auto* rendel_el = PushRenderElement(group, RenderEntryBitmap);
            rendel_el->quad = rect_to_quadrilateral(proj.P, dim);
            rendel_el->local_origin = 0.5 * dim;
            rendel_el->offset = proj.P;
            rendel_el->basis.x = vec2(cos(deg), -sin(deg));
            rendel_el->basis.y = vec2(sin(deg), cos(deg));
            rendel_el->bitmap_handle = bitmap->texture_handle;
        }
    }

    // render(&group, app_input->client_width, app_input->client_height);
}

void load(PlatformApi* platform, EngineMemory* in_memory) {
    load(platform);

    assert(sizeof(EngineState) < Permanent_Memory_Block_Size);
    auto* state = (EngineState*)in_memory->permanent;
    state->transient.init(in_memory->transient, Transient_Memory_Block_Size);
    set_transient_arena(&state->transient);
    load(&state->task_system);

    load(&state->graphics_options);
}

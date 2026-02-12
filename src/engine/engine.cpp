#include <cmath>
#include <cstdio>

#include <platform/platform.h>
#include <platform/types.h>

#include <math/mat3.h>
#include <math/math.h>
#include <math/transform.h>
#include <math/util.hpp>
#include <math/vec2.h>
#include <math/vec3.h>

#include <core/logger.h>

#include <renderers/renderer.h>

#include "assets.h"
#include "audio.hpp"
#include "core/string8.hpp"
#include "engine.h"
#include "gameplay.h"
#include "globals.hpp"
#include "gui/imgui.hpp"
#include "hm_assert.h"
#include "profiling.hpp"

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

ENGINE_GET_SOUND_SAMPLES(get_sound_samples) {
    auto* engine = (EngineState*)memory->permanent;
    auto* audio_system = &engine->audio;
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

    for (auto& ps : audio_system->playing_sounds) {
        if (ps.audio == nullptr) {
            LoadedAudio* loaded_audio = get_audio(game_assets, ps.id);
            if (audio_system) {
                ps.audio = loaded_audio;
            }
            else {
                load_audio(game_assets, ps.id);
            }
        }
    }

    for (auto& ps : audio_system->playing_sounds) {
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

ENGINE_UPDATE_AND_RENDER(update_and_render) {

    auto* state = (EngineState*)engine_memory->permanent;
    // const f32 ratio = static_cast<f32>(app_input->client_width) / static_cast<f32>(app_input->client_height);

    global_debug_table = Platform->debug_table;

    if (!state->is_initialized) {
        state->permanent.init(static_cast<u8*>(engine_memory->permanent) + sizeof(EngineState),
            Permanent_Memory_Block_Size - sizeof(EngineState));

        state->task_system.queue = Platform->work_queue;
        TaskSystem* task_system = &state->task_system;
        for (u32 i = 0; i < array_length(task_system->tasks); i++) {
            TaskWithMemory* task = task_system->tasks + i;
            task->memory = state->permanent.allocate_arena(KiloBytes(1));
        }

        state->pointer.x = 200;
        state->pointer.y = 200;

        state->assets = initialize_game_assets(&state->permanent);

        auto bitmap_id = get_first_bitmap_id(state->assets, AssetGroupId_PlayerSpaceShip);
        {
            auto player = get_bitmap_meta(state->assets, bitmap_id);
            state->player = default_entity(&player);
            state->player.P = vec2(300, 300);
        }

        state->player_projectiles.init(state->permanent, 100);
        state->enemies.init(state->permanent, 100);
        state->explosions.init(state->permanent, 100);

        init_audio_system(&state->audio, &state->permanent);

        state->is_initialized = true;
    }

    if (state->ui_context == nullptr) {

        auto font_id = get_first_font_id(state->assets, AssetGroupId_Fonts_Ubuntu);
        LoadedFont* font = get_font(state->assets, font_id);
        if (font) {
            state->ui_context = UI_CreateContext(&state->permanent);
            UI_SetContext(state->ui_context);
            UI_SetFont(font_id.value, font);
            renderer->add_texture(font_id.value, font->bitmap, font->bitmap_width, font->bitmap_height, font->bitmap_size_per_pixel);
            UI_SetContext(nullptr);
        }
    }

    BEGIN_BLOCK("prepare_frame")
    // TODO: Gotta set this on hot reload
    clear_transient();
    remove_finished_sounds(&state->audio);

    const i32 client_width = app_input->client_width;
    const i32 client_height = app_input->client_height;

    const vec2 screen_center = vec2(app_input->client_width / 2.0f, app_input->client_height / 2.0f);

    auto& time = state->time;
    auto is_new_second = static_cast<i32>(time.t) < static_cast<i32>(time.t + app_input->dt);
    if (is_new_second) {
        time.fps = time.num_frames_this_second;
        time.num_frames_this_second = 0;
    }
    time.dt = app_input->dt;
    time.t += time.dt;
    time.num_frames_this_second++;
    END_BLOCK();

    // endregion

#if HOMEMADE_DEBUG
    {
        TIMED_BLOCK("debug_checks");
        state->permanent.check_integrity();
        state->transient.check_integrity();
    }
#endif

    {
        TIMED_BLOCK("game_update");
        auto& input = app_input->input;

        // Update based on input
        {
            if (input.space.is_pressed_this_frame()) {

                AudioId audio_id = get_first_audio(state->assets, AssetGroupId_Audio_Laser);
                play_audio(&state->audio, audio_id);

                auto proj_meta = get_first_bitmap_meta(state->assets, AssetGroupId_Projectile);
                auto pos = state->player.P;
                f32 width = (f32)proj_meta.dim[0];
                f32 height = (f32)proj_meta.dim[1];
                pos.y = pos.y + height * 0.7f;
                pos.x = pos.x + 0.5f * width - 0.5f * proj_meta.dim[0];

                Entity p = default_entity(&proj_meta);
                p.P = pos;

                state->player_projectiles.push(p);
            }

            const auto max_speed = 300.0f;
            const f32 acc = 60.0f;

            vec2 direction = {};

            if (input.w.ended_down) {
                direction.y = 1.0f;
            }
            if (input.s.ended_down) {
                direction.y = -1.0f;
            }
            if (input.a.ended_down) {
                direction.x = -1.0f;
            }
            if (input.d.ended_down) {
                direction.x = 1.0f;
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

            player.P = update_position(player.P, player.speed, player.scale, time.dt, (f32)app_input->client_width,
                (f32)app_input->client_height);
        }

        // Update static
        state->enemy_timer += app_input->dt;
        if (state->enemy_timer > 0.3) {
            state->enemy_timer = 0;

            auto enemy_meta = get_first_bitmap_meta(state->assets, AssetGroupId_EnemySpaceShip);
            Entity enemy = default_entity(&enemy_meta);
            enemy.P = vec2(100.0f, (f32)app_input->client_height);
            enemy.rotation = PI;

            state->enemies.push(enemy);
        }

        const auto projectile_speed = 1200.0f;
        for (auto& proj : state->player_projectiles) {
            proj.P.y += projectile_speed * time.dt;
        }

        for (u64 e = 0; e < state->explosions.size(); e++) {
            auto& ex = state->explosions[e];
            if (ex.progress > 1.0f) {
                state->explosions.remove_and_dec(&e);
            }
            else {
                ex.progress += (time.dt * 2.0f);
            }
        }

        // Update enemies
        for (u64 i = 0; i < state->enemies.size();) {
            auto& enemy = state->enemies[i];
            enemy.P.y += -400.0f * time.dt;
            enemy.progress += time.dt;
            enemy.P.x = sine_movement(100.0, 100.0, 3.0, enemy.progress);

            if (enemy.P.y + enemy.scale.y <= 0.0) {
                state->enemies.remove(i);
            }
            else {
                i++;
            }
        }

        // Update projectile
        for (u64 i = 0; i < state->player_projectiles.size();) {
            auto pos = state->player_projectiles[i].P;
            vec2 bottom_left = vec2(0, 0);
            vec2 top_right = vec2((f32)app_input->client_width, (f32)app_input->client_height);
            if (!hm::in_rect(pos, bottom_left, top_right)) {
                state->player_projectiles.remove(i);
                continue;
            }
            else {
                i++;
            }
        }

        for (u64 i = 0; i < state->player_projectiles.size(); i++) {
            auto* proj = &state->player_projectiles[i];
            mat3 rot_mat = mat3_rotate(proj->rotation);
            mat3 scale_mat = mat3_scale(proj->scale);
            mat3 proj_to_world = rot_mat * scale_mat;

            vec3 bl_p = proj_to_world * vec3(proj->vertices[0], 0.0) + vec3(proj->P, 0.0);
            vec3 tl_p = proj_to_world * vec3(proj->vertices[1], 0.0) + vec3(proj->P, 0.0);
            vec3 tr_p = proj_to_world * vec3(proj->vertices[2], 0.0) + vec3(proj->P, 0.0);
            vec3 br_p = proj_to_world * vec3(proj->vertices[3], 0.0) + vec3(proj->P, 0.0);

            for (u64 e = 0; e < state->enemies.size(); e++) {
                auto* enemy = &state->enemies[e];
                mat3 to_enemy_model = inverse(mat3_rotate(enemy->rotation) * mat3_scale(enemy->scale));
                vec3 bl_m = to_enemy_model * (bl_p - vec3(enemy->P, 0.0));
                vec3 tl_m = to_enemy_model * (tl_p - vec3(enemy->P, 0.0));
                vec3 tr_m = to_enemy_model * (tr_p - vec3(enemy->P, 0.0));
                vec3 br_m = to_enemy_model * (br_p - vec3(enemy->P, 0.0));

                vec2 enemy_min = enemy->vertices[0]; // e.g. bottom-left in enemy space
                vec2 enemy_max = enemy->vertices[2]; // e.g. top-right in enemy space
                if (hm::in_rect(bl_m.xy(), enemy_min, enemy_max) || hm::in_rect(tl_m.xy(), enemy_min, enemy_max) ||
                    hm::in_rect(tr_m.xy(), enemy_min, enemy_max) || hm::in_rect(br_m.xy(), enemy_min, enemy_max)) {

                    BitmapMeta ex_meta = get_first_bitmap_meta(state->assets, AssetGroupId_Explosion);
                    Entity* explosion = state->explosions.push();
                    *explosion = default_entity(&ex_meta);
                    explosion->P = enemy->P;

                    AudioId audio_id = get_first_audio(state->assets, AssetGroupId_Audio_Explosion);
                    play_audio(&state->audio, audio_id);

                    state->enemies.remove_and_dec(&e);
                    state->player_projectiles.remove_and_dec(&i);
                    break;
                }
            }
        }
    }

    ///////////////////////////
    //// Rendering ////////////
    ///////////////////////////

    {
        TIMED_BLOCK("render_game");
        RenderCommands group{};
        group.push_buffer_size = 0;
        group.max_push_buffer_size = MegaBytes(4);
        group.push_buffer = allocate<u8>(*g_transient, group.max_push_buffer_size);
        group.screen_width = app_input->client_width;
        group.screen_height = app_input->client_height;
        group.sort_keys.init(g_transient, 1024);
        group.sort_entries_offset.init(g_transient, 1024);

        auto* clear = PushRenderElement(&group, RenderEntryClear, 0);
        clear->color = vec4(0.0f, 0.0f, 0.0, 0.0);

        {
            f32 direction = clamp(state->player.speed.x, -1.0, 1.0);
            //        state->player.rotation += app_input->dt;
            // state->player.scale = vec2(2.0f, 2.0f);
            // state->player.P = vec2(24.0f, 29.0f);
            auto bitmap_id =
                get_closest_bitmap_id(state->assets, AssetGroupId_PlayerSpaceShip, AssetTag_SpaceShipDirection, direction);
            auto bitmap = get_bitmap(state->assets, bitmap_id);
            if (bitmap) {
                i32 width = bitmap->width;
                i32 height = bitmap->height;
                void* data = bitmap->data;
                renderer->add_texture(bitmap_id.value, data, width, height, sizeof(u32));

                auto& player = state->player;
                auto* render_bm = PushRenderElement(&group, RenderEntryBitmap, 0);
                render_bm->quad = {
                    .bl = player.vertices[0],
                    .tl = player.vertices[1],
                    .tr = player.vertices[2],
                    .br = player.vertices[3],
                };
                render_bm->offset = player.P;
                render_bm->scale = player.scale;
                render_bm->rotation = player.rotation;
                render_bm->texture_id = bitmap_id.value;
            }
        }

        {

            auto bitmap_id = get_first_bitmap_id(state->assets, AssetGroupId_EnemySpaceShip);
            auto bitmap = get_bitmap(state->assets, bitmap_id);

            if (bitmap) {
                i32 width = bitmap->width;
                i32 height = bitmap->height;
                void* data = bitmap->data;
                renderer->add_texture(bitmap_id.value, data, width, height, sizeof(u32));

                for (auto& enemy : state->enemies) {
                    auto* render_el = PushRenderElement(&group, RenderEntryBitmap, 0);
                    render_el->quad = {
                        .bl = enemy.vertices[0],
                        .tl = enemy.vertices[1],
                        .tr = enemy.vertices[2],
                        .br = enemy.vertices[3],
                    };
                    render_el->offset = enemy.P;
                    render_el->scale = enemy.scale;
                    render_el->rotation = enemy.rotation;
                    render_el->texture_id = bitmap_id.value;
                }
            }
        }

        {
            if (state->player_projectiles.size() != 0) {
                auto bitmap_id = get_first_bitmap_id(state->assets, AssetGroupId_Projectile);
                auto bitmap = get_bitmap(state->assets, bitmap_id);
                if (bitmap) {
                    i32 width = bitmap->width;
                    i32 height = bitmap->height;
                    void* data = bitmap->data;
                    renderer->add_texture(bitmap_id.value, data, width, height, sizeof(u32));
                }
                for (auto& proj : state->player_projectiles) {
                    if (bitmap) {
                        auto* rendel_el = PushRenderElement(&group, RenderEntryBitmap, 0);
                        rendel_el->quad = {
                            .bl = proj.vertices[0],
                            .tl = proj.vertices[1],
                            .tr = proj.vertices[2],
                            .br = proj.vertices[3],
                        };
                        rendel_el->offset = proj.P;
                        rendel_el->scale = proj.scale;
                        rendel_el->rotation = proj.rotation;
                        rendel_el->texture_id = bitmap_id.value;
                    }
                }
            }
        }
        {
            if (state->explosions.size() != 0) {
                for (auto& ex : state->explosions) {
                    auto bitmap_id =
                        get_closest_bitmap_id(state->assets, AssetGroupId_Explosion, AssetTag_ExplosionProgress, ex.progress);
                    auto meta = get_bitmap_meta(state->assets, bitmap_id);
                    auto bbox = get_bbox(&meta);
                    auto bitmap = get_bitmap(state->assets, bitmap_id);

                    if (bitmap) {
                        i32 width = bitmap->width;
                        i32 height = bitmap->height;
                        void* data = bitmap->data;
                        renderer->add_texture(bitmap_id.value, data, width, height, sizeof(u32));
                    }
                    if (bitmap) {
                        auto* rendel_el = PushRenderElement(&group, RenderEntryBitmap, 0);
                        rendel_el->quad = { .bl = bbox.bl, .tl = bbox.tl, .tr = bbox.tr, .br = bbox.br };
                        rendel_el->offset = ex.P;
                        rendel_el->scale = ex.scale;
                        rendel_el->rotation = ex.rotation;
                        rendel_el->texture_id = bitmap_id.value;
                    }
                }
            }
        }

        renderer->render(Platform->work_queue, &group);
    }

    {

        if (state->ui_context) {
            BEGIN_BLOCK("gui_create");

            RenderCommands ui_render_group{};
            ui_render_group.push_buffer_size = 0;
            ui_render_group.max_push_buffer_size = MegaBytes(4);
            ui_render_group.push_buffer = allocate<u8>(*g_transient, ui_render_group.max_push_buffer_size);
            ui_render_group.screen_width = client_width;
            ui_render_group.screen_height = client_height;
            ui_render_group.sort_keys.init(g_transient, 1024);
            ui_render_group.sort_entries_offset.init(g_transient, 1024);
            ui_render_group.screen_height = client_height;

            UI_SetContext(state->ui_context);
            Mouse* mouse = &app_input->input.mouse;
            UI_Begin(mouse, client_width, client_height);
            UI_SetLayout({ .layout_direction = UI_Direction_Right });
            vec4 background_color = vec4(29.0f, 32.0f, 75.0f, 180.0f);
            UI_PushStyleBackgroundColor(background_color);
            UI_WindowFull("Execution time", UI_Fixed(0.0f), UI_Fixed(50.0f), UI_Pixels(800.0f), UI_Pixels(100.0f)) {

                i32 i = 0;
                List<PrintDebugEvent>& debug_print_events = engine_memory->debug_print_events[1];
                for (PrintDebugEvent& event : debug_print_events) {
                    if (event.depth == 1) {
                        f32 block_fraction = 1.0f;
                        if (event.depth > 0) {
                            PrintDebugEvent* parent = &debug_print_events[event.parent_index];
                            i64 parent_cycle_count = (parent->clock_end - parent->clock_start);
                            i64 event_cycle_count = (event.clock_end - event.clock_start);
                            Assert(parent_cycle_count > event_cycle_count);
                            block_fraction = (f32)event_cycle_count / parent_cycle_count;
                        }
                        f32 frame_fraction = (f32)(event.clock_end - event.clock_start) / Platform->total_frame_duration_clock_cycles;
                        f32 ms = Platform->frame_duration_ms * frame_fraction;
                        vec4 box_color = global_color_palette[i % Global_Color_Palette_Count];
                        UI_PushStyleBackgroundColor(box_color);

                        string8 box_id = string8_concat(event.GUID, "_profile_box", g_transient);
                        UI_Entity_Status box = UI_Box(box_id.data, UI_PercentOfParent(frame_fraction), UI_PercentOfParent(1.0f));
                        if (box.first_hovered) {
                            printf("%s\n", event.GUID);
                        }
                        if (box.hovered) {
                            vec4 hover_background_color = vec4(0.0f, 0.0f, 0.0f, 220.0f);
                            UI_PushStyleBackgroundColor(hover_background_color);
                            UI_PushStyleZIndex(9000);
                            UI_PushStyleFlexDirection(UI_Flex_Column);
                            UI_Window("Hover window", UI_Fixed((f32)mouse->client_x + 5.0f), UI_Fixed((f32)mouse->client_y + 15.0f)) {
                                UI_Text(event.GUID);
                                UI_Text(string8_format(g_transient, "Fraction: %.2f", frame_fraction));
                                UI_Text(string8_format(g_transient, "%.2f ms", ms));
                            }
                            UI_PopStyleFlexDirection();
                            UI_PopStyleZIndex();
                            UI_PopStyleBackgroundColor();
                        }
                        UI_PopStyleBackgroundColor();
                        i++;
                    }
                }
            }
            UI_PopStyleBackgroundColor();

            UI_End();
            UI_Generate_Render_Commands(&ui_render_group);

            END_BLOCK();

            BEGIN_BLOCK("gui_render");
            renderer->render(Platform->work_queue, &ui_render_group);
            END_BLOCK();
        }
    }
}

ENGINE_LOAD(load) {
    load(platform_api);

    assert(sizeof(EngineState) < Permanent_Memory_Block_Size);
    auto* state = (EngineState*)memory->permanent;
    state->transient.init(memory->transient, Transient_Memory_Block_Size);
    set_transient_arena(&state->transient);
    load(&state->task_system);
}

#ifdef HOMEMADE_DEBUG
#include "debug.cpp"
#else
DEBUG_FRAME_END(debug_frame_end) {
}
#endif

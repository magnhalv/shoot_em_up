#pragma once

#include <iostream>

#include <engine/assets.h>
#include <engine/globals.hpp>

#include <math/mat4.h>
#include <math/vec4.h>
#include <platform/types.h>

#include "cli/cli.h"
#include "memory_arena.h"
#include "options.hpp"
#include "platform/platform.h"
#include "text_renderer.h"
#include <engine/audio.hpp>
#include <engine/structs/swap_back_list.h>

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

typedef struct {
    vec2 p;
    f32 deg; // rotation

    f32 progress;
    vec2 speed;
} Sprite;

struct Explosion {
    i32 frames_per_sprite = 0;
    i32 curr_frame = 0;
    i32 num_sprites = 0;
    i32 curr_sprite = 0;
    vec2 p;
    vec2 speed;
    vec2 acc;
};

struct Window {
    f32 width;
    f32 height;
    mat4 ortho;
    mat4 perspective;
};

struct PerFrameData {
    mat4 projection;
    mat4 view;
    mat4 model;
};

struct TimeInfo {
    f32 dt; // in seconds
    f32 t;

    // Performance stuff. Improve when needed
    i32 num_frames_this_second;
    i32 fps;
};

enum class InputMode { Game = 0, Gui };

struct Entity {
    vec2 P;
    vec2 dim;
    f32 direction;
    vec2 speed;
    f32 progress;
};

struct GameState {
    Sprite player;
};

struct EngineState {
    bool is_initialized = false;
    Pointer pointer;
    MemoryArena transient;
    MemoryArena permanent;

    AudioSystemState audio;
    GameAssets* assets;

    TaskSystem task_system;

    Cli cli;
    bool is_cli_active;

    Options graphics_options;
    TimeInfo time;

    TextRenderer text_renderer;
    Font* font;

    Entity player;
    SwapBackList<Explosion> explosions;
    SwapBackList<Entity> enemy_chargers;
    f32 enemy_timer;
    SwapBackList<Entity> player_projectiles;
    SwapBackList<Entity> enemy_projectiles;
};

extern "C" __declspec(dllexport) void update_and_render(EngineMemory* memory, EngineInput* app_input);
extern "C" __declspec(dllexport) void load(GLFunctions* in_gl, PlatformApi* in_platform, EngineMemory* in_memory);
extern "C" __declspec(dllexport) SoundBuffer get_sound_samples(EngineMemory* memory, i32 num_samples);

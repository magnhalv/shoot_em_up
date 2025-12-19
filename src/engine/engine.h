#pragma once

#include <core/memory_arena.h>

#include <engine/assets.h>
#include <engine/globals.hpp>

#include <math/mat4.h>
#include <math/vec4.h>
#include <platform/types.h>

#include "options.hpp"
#include "platform/platform.h"
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
    f64 t;

    // Performance stuff. Improve when needed
    i32 num_frames_this_second;
    i32 fps;
};

enum class InputMode { Game = 0, Gui };

struct Entity {
    vec2 P;
    vec2 scale;
    f32 rotation;
    vec2 vertices[4];
    vec2 speed;

    f32 progress;
    f32 animation_progress;
};

auto inline default_entity() -> Entity {
    Entity result = {};
    result.scale = vec2(1.0, 1.0);
    return result;
}

struct BBox {
    vec2 bl;
    vec2 tl;
    vec2 tr;
    vec2 br;
};

auto inline get_bbox(const BitmapMeta* meta) -> BBox {
    BBox result = {};
    f32 width = (f32)meta->dim[0];
    f32 height = (f32)meta->dim[1];
    f32 align_x = meta->align_percentage.x;
    f32 align_y = meta->align_percentage.y;
    result.bl = vec2(-width * align_x, -height * align_y);
    result.tl = vec2(-width * align_x, height * (1.0f - align_y));
    result.tr = vec2(width * (1 - align_x), height * (1.0f - align_y));
    result.br = vec2(width * (1 - align_x), -height * align_y);
    return result;
}

auto inline default_entity(const BitmapMeta* meta) -> Entity {
    Entity result = default_entity();
    BBox box = get_bbox(meta);
    result.vertices[0] = box.bl;
    result.vertices[1] = box.tl;
    result.vertices[2] = box.tr;
    result.vertices[3] = box.br;
    return result;
}

struct EngineState {
    bool is_initialized = false;
    Pointer pointer;
    MemoryArena transient;
    MemoryArena permanent;

    AudioSystemState audio;
    GameAssets* assets;

    TaskSystem task_system;

    Options graphics_options;
    TimeInfo time;

    Entity player;
    SwapBackList<Entity> explosions;
    SwapBackList<Entity> enemies;
    f64 enemy_timer;
    SwapBackList<Entity> player_projectiles;
    SwapBackList<Entity> enemy_projectiles;
};

extern "C" __declspec(dllexport) ENGINE_UPDATE_AND_RENDER(update_and_render);
extern "C" __declspec(dllexport) ENGINE_LOAD(load);
extern "C" __declspec(dllexport) ENGINE_GET_SOUND_SAMPLES(get_sound_samples);

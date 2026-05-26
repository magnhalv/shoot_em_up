#pragma once

#include <platform/platform.h>

#include <core/memory_arena.h>

#include <engine/assets.h>
#include <engine/camera.h>
#include <engine/globals.hpp>

#include <math/mat4.h>
#include <math/vec4.h>
#include <platform/platform.h>
#include <platform/types.h>

#include <engine/audio.hpp>
#include <engine/gui/imgui.hpp>
#include <engine/structs/swap_back_list.h>

struct EngineMemory {
    void* permanent = nullptr;
    void* transient = nullptr;

    MemoryBlock debug;
};

struct EngineInput {
    i32 client_width;
    i32 client_height;
    u64 frame_count;
    u64 ticks;
    u64 dt_tick;
    f64 t;
    f32 dt;
    i64 performance_counter_frequency;
    UserInput input;
};

constexpr i32 SoundSampleSize = sizeof(i16);
struct SoundBuffer {
    i16* samples;
    i32 num_samples;
    i32 sample_size_in_bytes;
    i32 samples_per_second;
    i32 tone_hz;
};

struct RendererApi;
#define ENGINE_UPDATE_AND_RENDER(name) \
    void name(EngineMemory* engine_memory, EngineInput* app_input, RendererApi* renderer)
typedef ENGINE_UPDATE_AND_RENDER(update_and_render_fn);

#define ENGINE_LOAD(name) void name(PlatformApi* platform_api, EngineMemory* memory)
typedef ENGINE_LOAD(load_fn);

#define ENGINE_GET_SOUND_SAMPLES(name) SoundBuffer name(EngineMemory* memory, i32 num_samples)
typedef ENGINE_GET_SOUND_SAMPLES(get_sound_samples_fn);

#define DEBUG_FRAME_END(name)                                                                       \
    void name(u64 ticks_at_start_of_frame, u64 ticks_at_start_of_next_frame, f32 frame_duration_ms, \
        f32 frame_duration_before_sleep_ms, EngineMemory* engine_memory, EngineInput* engine_input)
typedef DEBUG_FRAME_END(debug_frame_end_fn);

/// Processed mouse input
struct Pointer {
    f32 x = 0;
    f32 y = 0;

    auto update_pos(const Mouse& raw, i32 client_width, i32 client_height) -> void {
        // TODO: Sensitivity must be moved somewhere else
        /*const f32 sensitivity = 2.0;*/
        /*const f32 dx = static_cast<f32>(raw.dx) * sensitivity;*/
        /*const f32 dy = static_cast<f32>(raw.dy) * sensitivity;*/
        /**/
        /*x = fmin(fmax(dx + x, 0.0f), static_cast<f32>(client_width));*/
        /*y = fmin(fmax(dy + y, 0.0f), static_cast<f32>(client_height));*/
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
    result.tr = vec2(width * (1.0f - align_x), height * (1.0f - align_y));
    result.br = vec2(width * (1.0f - align_x), -height * align_y);
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

    TimeInfo time;

    Entity player;
    SwapBackList<Entity> explosions;
    SwapBackList<Entity> enemies;
    f64 enemy_timer;
    SwapBackList<Entity> player_projectiles;
    SwapBackList<Entity> enemy_projectiles;

    Camera camera;

    UI_Context* ui_context;
};

global_variable const i32 Global_Color_Palette_Count = 32;
global_variable vec4 global_color_palette[Global_Color_Palette_Count] = {
    vec4(0.902f, 0.098f, 0.294f, 1.0f), // vivid red
    vec4(0.235f, 0.706f, 0.294f, 1.0f), // green
    vec4(1.000f, 0.882f, 0.098f, 1.0f), // yellow
    vec4(0.000f, 0.510f, 0.784f, 1.0f), // blue
    vec4(0.961f, 0.510f, 0.188f, 1.0f), // orange
    vec4(0.569f, 0.118f, 0.706f, 1.0f), // purple
    vec4(0.275f, 0.941f, 0.941f, 1.0f), // cyan
    vec4(0.941f, 0.196f, 0.902f, 1.0f), // magenta
    vec4(0.824f, 0.961f, 0.235f, 1.0f), // lime
    vec4(0.980f, 0.745f, 0.831f, 1.0f), // pink
    vec4(0.000f, 0.502f, 0.502f, 1.0f), // teal
    vec4(0.863f, 0.745f, 1.000f, 1.0f), // lavender
    vec4(0.667f, 0.431f, 0.157f, 1.0f), // brown
    vec4(1.000f, 0.980f, 0.784f, 1.0f), // beige
    vec4(0.502f, 0.000f, 0.000f, 1.0f), // maroon
    vec4(0.667f, 1.000f, 0.765f, 1.0f), // mint

    vec4(0.502f, 0.502f, 0.000f, 1.0f), // olive
    vec4(1.000f, 0.843f, 0.706f, 1.0f), // apricot
    vec4(0.000f, 0.000f, 0.502f, 1.0f), // navy
    vec4(1.000f, 0.882f, 0.706f, 1.0f), // peach
    vec4(0.000f, 1.000f, 0.000f, 1.0f), // bright green
    vec4(1.000f, 0.627f, 0.478f, 1.0f), // salmon
    vec4(0.000f, 1.000f, 1.000f, 1.0f), // aqua
    vec4(0.729f, 0.333f, 0.827f, 1.0f), // medium purple
    vec4(1.000f, 0.388f, 0.278f, 1.0f), // tomato
    vec4(0.604f, 0.804f, 0.196f, 1.0f), // yellow green
    vec4(0.282f, 0.239f, 0.545f, 1.0f), // slate blue
    vec4(1.000f, 0.549f, 0.000f, 1.0f), // dark orange
    vec4(0.251f, 0.878f, 0.816f, 1.0f), // turquoise
    vec4(0.780f, 0.082f, 0.522f, 1.0f), // medium violet red
    vec4(0.529f, 0.808f, 0.922f, 1.0f), // sky blue
    vec4(1.000f, 1.000f, 1.000f, 1.0f), // white
};
global_variable const vec4 RED = vec4(1.0, 0.0, 0.0, 1.0);
global_variable const vec4 GREEN = vec4(0.0, 1.0, 0.0, 1.0);
global_variable const vec4 BLUE = vec4(0.0, 0.0, 1.0, 1.0);
global_variable const vec4 YELLOW = vec4(1.0, 1.0, 0.0, 1.0);
global_variable const vec4 PURPLE = vec4(0.5, 0.0, 0.5, 1.0);
global_variable const vec4 CYAN = vec4(0.0, 1.0, 1.0, 1.0);

extern "C" __declspec(dllexport) ENGINE_UPDATE_AND_RENDER(update_and_render);
extern "C" __declspec(dllexport) ENGINE_LOAD(load);
extern "C" __declspec(dllexport) ENGINE_GET_SOUND_SAMPLES(get_sound_samples);
extern "C" __declspec(dllexport) DEBUG_FRAME_END(debug_frame_end);

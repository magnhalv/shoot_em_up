#pragma once

#include <platform/platform.h>

#include <core/memory_arena.h>

#include <engine/assets.h>
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
#if HOMEMADE_DEBUG
    List<PrintDebugEvent> debug_print_events[TOTAL_THREAD_COUNT];
#endif
};

struct EngineInput {
    i32 client_width;
    i32 client_height;
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

#define DEBUG_FRAME_END(name) void name(EngineMemory* engine_memory, EngineInput* engine_input, RendererApi* renderer)
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

    TimeInfo time;

    Entity player;
    SwapBackList<Entity> explosions;
    SwapBackList<Entity> enemies;
    f64 enemy_timer;
    SwapBackList<Entity> player_projectiles;
    SwapBackList<Entity> enemy_projectiles;

    UI_Context* ui_context;
};

global_variable const i32 Global_Color_Palette_Count = 32;
global_variable vec4 global_color_palette[Global_Color_Palette_Count] = {
    vec4(230.0f, 25.0f, 75.0f, 255.0f),   // vivid red
    vec4(60.0f, 180.0f, 75.0f, 255.0f),   // green
    vec4(255.0f, 225.0f, 25.0f, 255.0f),  // yellow
    vec4(0.0f, 130.0f, 200.0f, 255.0f),   // blue
    vec4(245.0f, 130.0f, 48.0f, 255.0f),  // orange
    vec4(145.0f, 30.0f, 180.0f, 255.0f),  // purple
    vec4(70.0f, 240.0f, 240.0f, 255.0f),  // cyan
    vec4(240.0f, 50.0f, 230.0f, 255.0f),  // magenta
    vec4(210.0f, 245.0f, 60.0f, 255.0f),  // lime
    vec4(250.0f, 190.0f, 212.0f, 255.0f), // pink
    vec4(0.0f, 128.0f, 128.0f, 255.0f),   // teal
    vec4(220.0f, 190.0f, 255.0f, 255.0f), // lavender
    vec4(170.0f, 110.0f, 40.0f, 255.0f),  // brown
    vec4(255.0f, 250.0f, 200.0f, 255.0f), // beige
    vec4(128.0f, 0.0f, 0.0f, 255.0f),     // maroon
    vec4(170.0f, 255.0f, 195.0f, 255.0f), // mint

    vec4(128.0f, 128.0f, 0.0f, 255.0f),   // olive
    vec4(255.0f, 215.0f, 180.0f, 255.0f), // apricot
    vec4(0.0f, 0.0f, 128.0f, 255.0f),     // navy
    vec4(255.0f, 225.0f, 180.0f, 255.0f), // peach
    vec4(0.0f, 255.0f, 0.0f, 255.0f),     // bright green
    vec4(255.0f, 160.0f, 122.0f, 255.0f), // salmon
    vec4(0.0f, 255.0f, 255.0f, 255.0f),   // aqua
    vec4(186.0f, 85.0f, 211.0f, 255.0f),  // medium purple
    vec4(255.0f, 99.0f, 71.0f, 255.0f),   // tomato
    vec4(154.0f, 205.0f, 50.0f, 255.0f),  // yellow green
    vec4(72.0f, 61.0f, 139.0f, 255.0f),   // slate blue
    vec4(255.0f, 140.0f, 0.0f, 255.0f),   // dark orange
    vec4(64.0f, 224.0f, 208.0f, 255.0f),  // turquoise
    vec4(199.0f, 21.0f, 133.0f, 255.0f),  // medium violet red
    vec4(135.0f, 206.0f, 235.0f, 255.0f), // sky blue
    vec4(255.0f, 255.0f, 255.0f, 255.0f)  // white
};

extern "C" __declspec(dllexport) ENGINE_UPDATE_AND_RENDER(update_and_render);
extern "C" __declspec(dllexport) ENGINE_LOAD(load);
extern "C" __declspec(dllexport) ENGINE_GET_SOUND_SAMPLES(get_sound_samples);
extern "C" __declspec(dllexport) DEBUG_FRAME_END(debug_frame_end);

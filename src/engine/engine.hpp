#pragma once

#include <platform/platform.hpp>

#include <core/memory_arena.hpp>

#include <engine/assets.hpp>
#include <engine/camera.hpp>
#include <engine/globals.hpp>

#include <math/mat4.hpp>
#include <math/vec4.hpp>
#include <platform/platform.hpp>
#include <platform/types.hpp>

#include <engine/audio.hpp>
#include <engine/gui/imgui.hpp>
#include <engine/structs/swap_back_list.hpp>

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

    FrameBufferHandle frame_buffer_handle;

    Camera camera;

    UI_Context* ui_context;
};

extern "C" __declspec(dllexport) ENGINE_UPDATE_AND_RENDER(update_and_render);
extern "C" __declspec(dllexport) ENGINE_LOAD(load);
extern "C" __declspec(dllexport) ENGINE_GET_SOUND_SAMPLES(get_sound_samples);
extern "C" __declspec(dllexport) DEBUG_FRAME_END(debug_frame_end);

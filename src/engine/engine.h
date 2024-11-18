#pragma once

#include <math/mat4.h>
#include <math/vec4.h>
#include <platform/types.h>

#include "camera.h"
#include "cli/cli.h"
#include "framebuffer.h"
#include "gl/gl_shader.h"
#include "gl/gl_vao.h"
#include "memory_arena.h"
#include "options.hpp"
#include "platform/platform.h"
#include "sprite.hpp"
#include "text_renderer.h"

struct Sound {
  u16 samples;
  u32 num_samples;
  f32 frequency;
  i32 samples_per_sec;
};

struct WavRiffChunk {
  char identifier[4];
  u32 file_size;
  char file_format_id[4];
};

struct WavSubchunkDesc {
  char chunk_id[4];
  u32 chunk_size;
};

struct WavFmtChunk {
  char format_block_id[4];
  u32 block_size;
  u16 audio_format;
  u16 num_channels;
  u32 frequency;
  u32 bytes_per_sec;
  u16 bytes_per_block;
  u16 bits_per_sample;
};

struct WavDataChunk {
  char data_block_id[4];
  u32 data_size;
};

struct WavFile {
  WavRiffChunk* riff_chunk;
  WavFmtChunk* fmt_chunk;
  WavDataChunk* data_chunk;
  u8* data;
};

auto inline print(WavFile* file) -> void {
  printf("Header: %.4s\n", file->riff_chunk->identifier);
  printf("File Size: %u\n", file->riff_chunk->file_size);
  printf("File format id: %.4s\n", file->riff_chunk->file_format_id);
  printf("--------------\n");
  printf("Format block id: %.4s\n", file->fmt_chunk->format_block_id);
  printf("Block size: %u\n", file->fmt_chunk->block_size);
  printf("Audio format: %u\n", file->fmt_chunk->audio_format);
  printf("Num channels: %u\n", file->fmt_chunk->num_channels);
  printf("Frequency: %u\n", file->fmt_chunk->frequency);
  printf("Bytes per sec: %u\n", file->fmt_chunk->bytes_per_sec);
  printf("Bytes per block: %u\n", file->fmt_chunk->bytes_per_block);
  printf("Bits per sample: %u\n", file->fmt_chunk->bits_per_sample);
  printf("Data block id: %.4s\n", file->data_chunk->data_block_id);
  printf("Data size: %u\n", file->data_chunk->data_size);
}

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

extern Options* graphics_options;

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
  int id;
};

struct EngineState {
  Pointer pointer;
  bool is_initialized = false;
  Camera camera;
  MemoryArena transient;
  MemoryArena permanent;

  GLGlobalUniformBufferContainer uniform_buffer_container;
  Framebuffer framebuffer;
  MultiSampleFramebuffer ms_framebuffer;
  GLVao quad_vao{};

  Cli cli;
  bool is_cli_active;

  Options graphics_options;
  TimeInfo time;

  TextRenderer text_renderer;
  Font* font;

  Sprite sprite;
};

extern "C" __declspec(dllexport) void update_and_render(EngineMemory* memory, EngineInput* app_input);
extern "C" __declspec(dllexport) void load(GLFunctions* in_gl, Platform* in_platform, EngineMemory* in_memory);
extern "C" __declspec(dllexport) SoundBuffer get_sound_samples(i32 num_samples);

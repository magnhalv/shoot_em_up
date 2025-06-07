#pragma once

#include "engine/structs/swap_back_list.h"
#include <engine/array.h>
#include <engine/globals.hpp>
#include <engine/memory_arena.h>

enum class SoundType : u8 { Laser = 0, Explosion };

struct WavRiffChunk {
    char identifier[4];
    u32 file_size;
    char file_format_id[4];
};

struct WavSubchunkDesc {
    char chunk_id[4];
    u32 chunk_size;
};

struct WavFormatChunk {
    char format_block_id[4];
    u32 block_size;
    u16 audio_format;
    u16 num_channels;
    u32 frequency;
    u32 bytes_per_sec;
    u16 bytes_per_block;
    u16 bits_per_sample;

    auto print() -> void {
    }
};

struct WavDataChunk {
    char data_block_id[4];
    u32 data_size;
};

struct WavFile {
    WavRiffChunk* riff_chunk;
    WavFormatChunk* fmt_chunk;
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

struct Sound {
    SoundType type;
    Array<i16> samples;
};

struct PlayingSound {
    Sound* sound;
    i32 curr_sample;
};

struct AudioSystemState {
    bool is_initialized;
    Array<Sound> sounds;
    SwapBackList<PlayingSound> playing_sounds;
    MemoryArena* arena;
};

const SoundType sound_types[] = {
    SoundType::Laser,
    SoundType::Explosion,
};

auto init_audio_system(AudioSystemState& state, MemoryArena& init_arena) -> void;
auto remove_finished_sounds(AudioSystemState& state) -> void;
auto play_sound(SoundType sound_type, AudioSystemState& state) -> void;

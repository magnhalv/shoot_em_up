#include "engine/hm_assert.h"
#include "engine/logger.h"
#include <engine/array.h>
#include <engine/globals.hpp>
#include <engine/memory_arena.h>

enum class SoundType { Laser };

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

struct Sound {
  SoundType type;
  Array<u16> samples;
};

struct PlayingSound {
  Sound* sound;
  i32 curr_sample;
};

struct AudioSystemState {
  bool is_initialized;
  Array<Sound> sounds;
  PlayingSound playing_sounds[5];
  MemoryArena* arena;
};

const SoundType sound_types[] = {
  SoundType::Laser,
};

auto inline init_audio_system(AudioSystemState& state, MemoryArena& init_arena) {
  state.arena = init_arena.allocate_arena(MegaBytes(1));
  const char* sound_files[] = { "assets/sound/laser_primary.wav" };

  HM_ASSERT(state.is_initialized == false);
  static_assert(array_length(sound_files) == array_length(sound_types));

  auto num_sounds = array_length(sound_files);
  state.sounds.init(*state.arena, num_sounds);

  for (auto i = 0; i < num_sounds; i++) {
    auto& sound = state.sounds[i];
    auto path = sound_files[i];
    auto type = sound_types[i];
    auto file_size = g_platform->get_file_size(path);

    auto buffer = allocate<char>(*g_transient, file_size);
    auto is_success = g_platform->read_file(path, buffer, file_size);

    if (!is_success) {
      log_warning("Unable to read audio file: %s\n", path);
      sound.type = type;
      sound.samples.init(*state.arena, 0);
    }
    else {
      WavFile wav_file;
      wav_file.riff_chunk = reinterpret_cast<WavRiffChunk*>(buffer);
      u32 cursor = sizeof(WavRiffChunk);
      u32 cursor_end = wav_file.riff_chunk->file_size + 8;
      HM_ASSERT(cursor_end == file_size);
      while (cursor < cursor_end) {
        // TODO: memcopy the data into a WavFile struct, without pointers, so we can release the extra memory.
        WavSubchunkDesc* desc = reinterpret_cast<WavSubchunkDesc*>(buffer + cursor);
        if (std::memcmp("fmt", desc->chunk_id, 3) == 0) {
          wav_file.fmt_chunk = reinterpret_cast<WavFmtChunk*>(buffer + cursor);
        } else if (std::memcmp("data", desc->chunk_id, 4) == 0) {
          wav_file.data_chunk = reinterpret_cast<WavDataChunk*>(buffer + cursor);
          wav_file.data = reinterpret_cast<u8*>(buffer + cursor + sizeof(WavDataChunk));
        }
        // Discard the rest, as we do not support them.
        HM_ASSERT(desc->chunk_size > 0);
        cursor += desc->chunk_size + sizeof(WavSubchunkDesc);
      }

      auto bytes_per_sample = wav_file.fmt_chunk->bits_per_sample / 8;
      sound.samples.init(*state.arena, wav_file.data_chunk->data_size / bytes_per_sample);
      i32 src_idx = 0;
      i32 target_idx = 0;
      while (src_idx < wav_file.data_chunk->data_size) {
        u16 sample = 0;
        // we drop the least significant part
        src_idx++;
        sample |= wav_file.data[src_idx++] << 16;
        sample |= wav_file.data[src_idx++] << 8;
        sound.samples[target_idx++] = sample;
      }
    }

  }
  state.is_initialized = true;
}
auto inline play_sound(SoundType sound_type, AudioSystemState& state) -> void {
  PlayingSound* free_slot = nullptr;
  for (auto& ps : state.playing_sounds) {
    if (ps.sound == nullptr) {
      free_slot = &ps;
      continue;
    }
    if (ps.sound->type == sound_type) {
      return;
    }
  }

  for (auto& s : state.sounds) {
    if (s.type == sound_type) {
      free_slot->sound = &s;
      free_slot->curr_sample = 0;
    }
  }
}

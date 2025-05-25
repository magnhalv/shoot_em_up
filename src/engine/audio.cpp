#include "audio.hpp"
#include "engine/hm_assert.h"
#include <engine/logger.h>

auto init_audio_system(AudioSystemState& state, MemoryArena& init_arena) -> void {
    state.arena = init_arena.allocate_arena(MegaBytes(1));
    const char* sound_files[] = {
        "assets/sound/laser_primary.wav", //
        "assets/sound/output.wav",        //
    };

    HM_ASSERT(state.is_initialized == false);
    static_assert(array_length(sound_files) == array_length(sound_types));

    auto num_sounds = array_length(sound_files);
    state.sounds.init(*state.arena, num_sounds);

    for (auto i = 0; i < num_sounds; i++) {
        auto& sound = state.sounds[i];
        auto path = sound_files[i];
        sound.type = sound_types[i];
        auto file_size = Platform->get_file_size(path);

        auto buffer = allocate<char>(*g_transient, file_size);
        auto is_success = Platform->debug_read_file(path, buffer, file_size);

        if (!is_success) {
            log_warning("Unable to read audio file: %s\n", path);
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
                    wav_file.fmt_chunk = reinterpret_cast<WavFormatChunk*>(buffer + cursor);
                }
                else if (std::memcmp("data", desc->chunk_id, 4) == 0) {
                    wav_file.data_chunk = reinterpret_cast<WavDataChunk*>(buffer + cursor);
                    wav_file.data = reinterpret_cast<u8*>(buffer + cursor + sizeof(WavDataChunk));
                }
                // Discard the rest, as we do not support them.
                HM_ASSERT(desc->chunk_size > 0);
                cursor += desc->chunk_size + sizeof(WavSubchunkDesc);
            }

            print(&wav_file);
            HM_ASSERT(wav_file.fmt_chunk->frequency == 48000);
            HM_ASSERT(wav_file.fmt_chunk->num_channels == 1);
            HM_ASSERT(wav_file.fmt_chunk->bits_per_sample == 16);

            auto bytes_per_sample = wav_file.fmt_chunk->bits_per_sample / 8;
            sound.samples.init(*state.arena, wav_file.data_chunk->data_size / bytes_per_sample);
            u8* source = wav_file.data;
            u8* target = reinterpret_cast<u8*>(sound.samples.data());
            u32 idx = 0;
            while (idx < wav_file.data_chunk->data_size) {
                *(target + idx) = *(source + idx);
                idx++;
            }
        }
    }
    state.playing_sounds.init(*state.arena, 30);
    state.is_initialized = true;
}

auto remove_finished_sounds(AudioSystemState& state) -> void {
    u32 size = state.playing_sounds.size();
    for (auto i = 0; i < size; i++) {
        const auto& ps = state.playing_sounds[i];
        if (ps.curr_sample == ps.sound->samples.size()) {
            state.playing_sounds.remove(i);
            i--;
            size--;
        }
    }
}

auto play_sound(SoundType sound_type, AudioSystemState& state) -> void {
    if (state.playing_sounds.is_full()) {
        log_warning("[AUDIO] Unable to play sound, queue is full.");
        return;
    }

    for (auto& s : state.sounds) {
        if (s.type == sound_type) {
            PlayingSound ps{};
            ps.sound = &s;
            ps.curr_sample = 0;
            state.playing_sounds.push(ps);
        }
    }
}

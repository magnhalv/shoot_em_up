#pragma once

#include <core/memory_arena.h>

#include <engine/array.h>
#include <engine/assets.h>
#include <engine/globals.hpp>
#include <engine/hugin_file_formats.h>
#include <engine/structs/swap_back_list.h>

struct PlayingSound {
    AudioId id;
    i32 curr_sample;

    LoadedAudio* audio;
};

struct AudioSystemState {
    bool is_initialized;
    SwapBackList<PlayingSound> playing_sounds;
    MemoryArena* arena;
};

auto init_audio_system(AudioSystemState* state, MemoryArena* permanent_arena) -> void;
auto remove_finished_sounds(AudioSystemState* state) -> void;
auto play_audio(AudioSystemState* state, AudioId id) -> void;

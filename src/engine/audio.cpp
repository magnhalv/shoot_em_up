#include <core/logger.h>

#include "audio.hpp"
#include "engine/assets.h"
#include "engine/hm_assert.h"

auto init_audio_system(AudioSystemState* state, MemoryArena* init_arena) -> void {
    HM_ASSERT(state->is_initialized == false);

    state->playing_sounds.init(init_arena, 30);
    state->is_initialized = true;
}

auto remove_finished_sounds(AudioSystemState* state) -> void {
    u64 size = state->playing_sounds.size();
    for (u32 i = 0; i < size; i++) {
        const auto& ps = state->playing_sounds[i];
        if (ps.audio && ps.curr_sample == ps.audio->sample_count) {
            state->playing_sounds.remove(i);
            i--;
            size--;
        }
    }
}

auto play_audio(AudioSystemState* state, AudioId id) -> void {
    if (state->playing_sounds.is_full()) {
        log_warning("[AUDIO] Unable to play sound, queue is full.");
        return;
    }

    PlayingSound ps = {};
    ps.id = id;
    ps.curr_sample = 0;
    ps.audio = nullptr;
    state->playing_sounds.push(ps);
}

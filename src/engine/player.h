#ifndef HOT_RELOAD_OPENGL_PLAYER_H
#define HOT_RELOAD_OPENGL_PLAYER_H

#include "engine.h"

auto update_player(EngineState *state, EngineInput *app_input) {
    if (app_input->input.move_left.ended_down) {
        state->meshes[0].transform.position.x -= 10.0f * app_input->dt;
    }
    if (app_input->input.move_right.ended_down) {
        state->meshes[0].transform.position.x += 10.0f * app_input->dt;
    }
    if (app_input->input.move_up.ended_down) {
        state->meshes[0].transform.position.z += -10.0f * app_input->dt;
    }
    if (app_input->input.move_down.ended_down) {
        state->meshes[0].transform.position.z += 2.0f * app_input->dt;
    }
}

#endif //HOT_RELOAD_OPENGL_PLAYER_H

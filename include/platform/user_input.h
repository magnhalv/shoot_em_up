#ifndef HOT_RELOAD_OPENGL_USER_INPUT_H
#define HOT_RELOAD_OPENGL_USER_INPUT_H

#include <platform/types.h>

#include <cassert>

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

struct ButtonState {
    ButtonState() : half_transition_count(0), ended_down(false) {
    }

    i32 half_transition_count; // How many times it flipped between up and down
    bool ended_down;

    [[nodiscard]] auto is_pressed_this_frame() const -> bool {
        return ended_down && half_transition_count == 1;
    }

    [[nodiscard]] auto is_released_this_frame() const -> bool {
        return !ended_down && half_transition_count == 1;
    }
};

struct Mouse {
    i32 screen_x = 0;
    i32 screen_y = 0;
    i32 client_x = 0;
    i32 client_y = 0;
    i32 dx = 0;
    i32 dy = 0;

    union {
        ButtonState buttons[2];
        struct {
            ButtonState left;
            ButtonState right;
        };
    };
};

const i32 NUM_BUTTONS = 35;

struct UserInput {
    Mouse mouse;
    union {
        ButtonState buttons[NUM_BUTTONS + 1];
        struct {
            ButtonState a;
            ButtonState b;
            ButtonState c;
            ButtonState d;
            ButtonState e;
            ButtonState f;
            ButtonState g;
            ButtonState h;
            ButtonState i;
            ButtonState j;
            ButtonState k;
            ButtonState l;
            ButtonState m;
            ButtonState n;
            ButtonState o;
            ButtonState p;
            ButtonState q;
            ButtonState r;
            ButtonState s;
            ButtonState t;
            ButtonState u;
            ButtonState v;
            ButtonState w;
            ButtonState x;
            ButtonState y;
            ButtonState z;

            ButtonState back;
            ButtonState enter;
            ButtonState tab;

            ButtonState move_up;
            ButtonState move_down;
            ButtonState move_left;
            ButtonState move_right;

            ButtonState space;
            ButtonState oem_5;

            // Note: All buttons must be added above this line
            ButtonState terminator;
        };
    };

    void frame_clear(const UserInput& prev_frame_input) {
        mouse.dx = 0;
        mouse.dy = 0;
        for (auto idx = 0; idx < 2; idx++) {
            mouse.buttons[idx].half_transition_count = 0;
            mouse.buttons[idx].ended_down = prev_frame_input.mouse.buttons[idx].ended_down;
        }

        assert(NUM_BUTTONS + 1 == ArrayCount(buttons));

        for (auto idx = 0; idx < NUM_BUTTONS; idx++) {
            buttons[idx].half_transition_count = 0;
            buttons[idx].ended_down = prev_frame_input.buttons[idx].ended_down;
        }
    }
};

#endif // HOT_RELOAD_OPENGL_USER_INPUT_H

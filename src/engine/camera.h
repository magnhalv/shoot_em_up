#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <math/mat4.h>
#include <math/vec2.h>
#include <math/vec3.h>

#include <platform/platform.h>
#include <platform/types.h>
#include <platform/user_input.h>


const vec3 up{0, 1, 0};

struct Camera {
    f32 _yaw = 0;
    f32 _pitch =0;
    vec3 _position = {};
    vec3 _direction = {};


    void init(f32 yaw, f32 pitch, vec3 position) {
        _yaw = yaw;
        _pitch = pitch;
        _position = position;
    }


    void update_keyboard(UserInput &input) {
        glm::vec2 dir = glm::normalize(glm::vec2(_direction.x, _direction.z));
        const f32 speed = 0.025f;
        if (input.move_up.ended_down) {
            _position.x += dir.x * speed;
            _position.z += dir.y * speed;
        }
        if (input.move_down.ended_down) {
            _position.x -= dir.x * speed;
            _position.z -= dir.y * speed;
        }
        vec3 cross_result = cross(_direction, up);
        glm::vec2 side_dir = glm::normalize(glm::vec2(cross_result.x, cross_result.z));
        if (input.move_left.ended_down) {
            _position.x -= side_dir.x * speed;
            _position.z -= side_dir.y * speed;
        }
        if (input.move_right.ended_down) {
            _position.x += side_dir.x * speed;
            _position.z += side_dir.y * speed;
        }
    }

    void update_cursor(f32 dx, f32 dy) {
        f32 sensitivity = 0.2f;
        _yaw += dx * sensitivity;
        _pitch += dy * sensitivity;

        if (_pitch > 89.0f) {
            _pitch = 89.0f;
        }
        if (_pitch < -89.0f) {
            _pitch = -89.0f;
        }

        vec3 new_direction;
        new_direction.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
        new_direction.y = sin(glm::radians(_pitch));
        new_direction.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
        _direction = normalized(new_direction);
    }

    [[nodiscard]] auto get_view() const -> mat4 {
        return lookAt(_position, _position + _direction, up);
    }

    [[nodiscard]] auto get_position() const -> vec3 {
        return _position;
    }
};

#pragma once

#include <math/mat4.hpp>
#include <math/trigonometry.hpp>
#include <math/vec2.hpp>
#include <math/vec3.hpp>

#include <platform/platform.hpp>
#include <platform/types.hpp>
#include <platform/user_input.hpp>

struct Camera {
    f32 m_yaw;
    f32 m_pitch;
    vec3 m_position;
    vec3 m_direction;

    mat4 m_world_to_view;
};

auto inline camera_init(f32 yaw, f32 pitch, vec3 position) -> Camera {
    Camera camera;
    camera.m_yaw = yaw;
    camera.m_pitch = pitch;
    camera.m_position = position;

    vec3 new_direction;
    new_direction.x = cosf(radians(camera.m_yaw)) * cosf(radians(camera.m_pitch));
    new_direction.y = sinf(radians(camera.m_pitch));
    new_direction.z = sinf(radians(camera.m_yaw)) * cosf(radians(camera.m_pitch));
    camera.m_direction = normalized(new_direction);

    return camera;
}

void inline camera_update_keyboard(Camera& camera, UserInput& input) {
    vec2 dir = normalized(vec2(camera.m_direction.x, camera.m_direction.z));
    const f32 speed = 0.025f;
    if (input.w.ended_down) {
        camera.m_position.x += dir.x * speed;
        camera.m_position.z += dir.y * speed;
    }
    if (input.s.ended_down) {
        camera.m_position.x -= dir.x * speed;
        camera.m_position.z -= dir.y * speed;
    }
    const vec3 up{ 0, 1, 0 };
    vec3 cross_result = cross(up, camera.m_direction);
    vec2 side_dir = normalized(vec2(cross_result.x, cross_result.z));
    if (input.a.ended_down) {
        camera.m_position.x -= side_dir.x * speed;
        camera.m_position.z -= side_dir.y * speed;
    }
    if (input.d.ended_down) {
        camera.m_position.x += side_dir.x * speed;
        camera.m_position.z += side_dir.y * speed;
    }
}

void inline update_cursor(Camera& camera, f32 dx, f32 dy) {
    f32 sensitivity = 0.2f;
    camera.m_yaw += dx * sensitivity;
    camera.m_pitch += dy * sensitivity;

    if (camera.m_pitch > 89.0f) {
        camera.m_pitch = 89.0f;
    }
    if (camera.m_pitch < -89.0f) {
        camera.m_pitch = -89.0f;
    }

    // TODO: Change this to always be in radians
    vec3 new_direction;
    new_direction.x = cosf(radians(camera.m_yaw)) * cosf(radians(camera.m_pitch));
    new_direction.y = sinf(radians(camera.m_pitch));
    new_direction.z = sinf(radians(camera.m_yaw)) * cosf(radians(camera.m_pitch));
    camera.m_direction = normalized(new_direction);
}

auto inline camera_get_view(const Camera& camera) -> mat4 {
    const vec3 up{ 0, 1, 0 };
    return lookAt(camera.m_position, camera.m_position + camera.m_direction, up);
}

auto inline camera_print(const Camera& camera) {
    printf("Camera: \n");
    printf("  yaw: %f\n", camera.m_yaw);
    printf("  pitch: %f\n", camera.m_yaw);
    printf("  position: [%f, %f, %f] \n", camera.m_position.x, camera.m_position.y, camera.m_position.z);
    printf("  direction: [%f, %f, %f] \n", camera.m_direction.x, camera.m_direction.y, camera.m_direction.z);
}

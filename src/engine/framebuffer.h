#ifndef HOT_RELOAD_OPENGL_FRAMEBUFFER_H
#define HOT_RELOAD_OPENGL_FRAMEBUFFER_H

#include <platform/types.h>
#include "gl/gl.h"
#include "logger.h"

struct Framebuffer {
    u32 fbo;
    u32 texture;
    u32 rbo; // render buffer object

    auto init(u32 screen_width, u32 screen_height) -> void {
        gl->gen_framebuffers(1, &fbo);
        gl->bind_framebuffer(GL_FRAMEBUFFER, fbo);

        // texture
        gl->textures_gen(1, &texture);
        gl->texture_bind(GL_TEXTURE_2D, texture);
        gl->tex_image_2d(GL_TEXTURE_2D, 0, GL_RGB, screen_width, screen_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        gl->tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl->tex_parameter_i(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl->framebuffer_texture_2d(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
        gl->texture_bind(GL_TEXTURE_2D, 0);

        // render buffer object
        gl->renderbuffers_gen(1, &rbo);
        gl->renderbuffer_bind(GL_RENDERBUFFER, rbo);
        gl->renderbuffer_storage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screen_width, screen_height);
        gl->framebuffer_renderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
        gl->renderbuffer_bind(GL_RENDERBUFFER, 0);

        const auto status = gl->framebuffer_check_status(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            crash_and_burn("Failed to initialize framebuffer. Error: %d", status);
        }

        gl->bind_framebuffer(GL_FRAMEBUFFER, 0);
    }
};

struct MultiSampleFramebuffer {
    u32 fbo;
    u32 texture;
    u32 rbo; // render buffer object

    auto init(u32 screen_width, u32 screen_height) -> void {
        const i32 num_samples = 4;

        gl->gen_framebuffers(1, &fbo);
        gl->bind_framebuffer(GL_FRAMEBUFFER, fbo);

        // texture
        gl->textures_gen(1, &texture);
        gl->texture_bind(GL_TEXTURE_2D_MULTISAMPLE, texture);
        gl->tex_image_2d_multisample(GL_TEXTURE_2D_MULTISAMPLE, num_samples, GL_RGB, screen_width, screen_height,
                                     GL_TRUE);
        gl->framebuffer_texture_2d(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texture, 0);
        gl->texture_bind(GL_TEXTURE_2D, 0);

        // render buffer object
        gl->renderbuffers_gen(1, &rbo);
        gl->renderbuffer_bind(GL_RENDERBUFFER, rbo);
        gl->renderbuffer_storage_multisample(GL_RENDERBUFFER, num_samples, GL_DEPTH24_STENCIL8, screen_width,
                                             screen_height);
        gl->framebuffer_renderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
        gl->renderbuffer_bind(GL_RENDERBUFFER, 0);

        const auto status = gl->framebuffer_check_status(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            crash_and_burn("Failed to initialize multi-sample framebuffer. Error: %d", status);
        }

        gl->bind_framebuffer(GL_FRAMEBUFFER, 0);
    }
};

#endif //HOT_RELOAD_OPENGL_FRAMEBUFFER_H

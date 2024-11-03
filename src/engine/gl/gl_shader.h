#pragma once

#include <string>

#include <glad/gl.h>
#include <platform/types.h>
#include <math/mat4.h>
#include <math/vec4.h>

#include "../assets.h"

const i32 Shader_Path_Max_Length = 512;
const u32 Max_Buffers = 3;

struct Shader {
    char path[Shader_Path_Max_Length];
    TimeStamp last_modified;
};

struct GLUniformBuffer {
    u32 handle;
    void *data;
    u32 index;
    GLsizeiptr size;
    GLbitfield flags;
};

struct GLShaderProgram {
public:
    auto initialize(const char *vertex_path, const char *fragment_path) -> bool;

    auto useProgram() const -> void;

    auto free() -> void;

    [[nodiscard]] GLuint getHandle() const { return _handle; }

    auto set_uniform(const char *name, const vec4 &vec) const -> void;
    auto set_uniform(const char *name, const vec3 &vec) const -> void;
    auto set_uniform(const char *name, const mat4 &vec) const -> void;

    auto relink_if_changed() -> void;

private:
    auto create_program(const char *vertex_path, const char *fragment_path) -> u32;

    FileAsset _vertex_source{};
    FileAsset _fragment_source{};

    GLuint _handle{};
};

enum class UniformBuffer: i32 {
    PerFrame = 0,
    Light = 1,
    Material = 2
};

struct GLGlobalUniformBufferContainer {
public:
    auto init(UniformBuffer index, void *data, GLsizeiptr size, GLbitfield flags = 0) -> bool;
    auto upload() const -> void;

    template<typename T>
    auto get_location(UniformBuffer idx) -> T* {
        return (T*)_uniform_buffers[+idx].data;
    }
private:
    GLUniformBuffer _uniform_buffers[Max_Buffers];
};

GLenum GLShaderType_from_file_name(const char *file_name);

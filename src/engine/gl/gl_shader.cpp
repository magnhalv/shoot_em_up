#include <glad/gl.h>
#include <cassert>
#include <cstdio>
#include <string>

#include "../logger.h"
#include "../memory_arena.h"
#include "../platform.h"
#include "gl.h"
#include "gl_shader.h"

auto print_shader_source(const char *text) -> void {
    int line = 1;

    printf("\n%3i: ", line);

    while (text && *text++) {
        if (*text == '\n') {
            printf("\n%3i: ", ++line);
        } else if (*text == '\r') {
        } else {
            printf("%c", *text);
        }
    }

    printf("\n");
}

auto read_shader_file(const char *fileName) -> char * {
    FILE *file = fopen(fileName, "r");

    if (!file) {
        log_error("I/O error. Cannot open shader file '%s'\n", fileName);
        return {};
    }
    // TODO: Use platform read file function
    fseek(file, 0L, SEEK_END);
    const auto bytes_in_file = ftell(file);
    fseek(file, 0L, SEEK_SET);

    char *buffer = (char *) allocate_transient(bytes_in_file + 1);
    const size_t bytes_read = fread(buffer, 1, bytes_in_file, file);
    fclose(file);

    buffer[bytes_read] = 0;

    static constexpr unsigned char BOM[] = {0xEF, 0xBB, 0xBF};

    if (bytes_read > 3) {
        if (!memcmp(buffer, BOM, 3))
            memset(buffer, ' ', 3);
    }

    return buffer;
    /*std::string code(buffer);

    while (code.find("#include ") != std::string::npos)
    {
        const auto pos = code.find("#include ");
        const auto p1 = code.find('<', pos);
        const auto p2 = code.find('>', pos);
        if (p1 == std::string::npos || p2 == std::string::npos || p2 <= p1)
        {
            printf("Error while loading shader program: %s\n", code.c_str());
            return {};
        }
        const std::string name = code.substr(p1 + 1, p2 - p1 - 1);
        const std::string include = read_shader_file(name.c_str());
        code.replace(pos, p2-pos+1, include.c_str());
    }

    return code;*/
}

auto compile_shader(const char *path) -> u32 {
    auto type = GLShaderType_from_file_name(path);
    auto text = read_shader_file(path);
    auto handle = gl->create_shader(type);
    gl->shader_source(handle, 1, &text, nullptr);
    gl->compile_shader(handle);

    char buffer[8192];
    GLsizei length = 0;
    gl->get_shader_info_log(handle, sizeof(buffer), &length, buffer);

    if (length) {
        print_shader_source(text);
        log_error("%sFile: %s\n", buffer, path);
        return Gl_Invalid_Id;
    }
    return handle;
}

auto print_program_info_log(GLuint handle) -> void {
    char buffer[8192];
    GLsizei length = 0;
    gl->get_program_info_log(handle, sizeof(buffer), &length, buffer);
    if (length) {
        printf("%s\n", buffer);
        assert(false);
    }
}

auto GLShaderProgram::create_program(const char *vertex_path, const char *fragment_path) -> u32 {
    auto program_handle = gl->create_program();
    auto vertex_handle = compile_shader(vertex_path);
    if (vertex_handle == Gl_Invalid_Id) {
        return Gl_Invalid_Id;
    }

    auto fragment_handle = compile_shader(fragment_path);
    if (fragment_handle == Gl_Invalid_Id) {
        return Gl_Invalid_Id;
    }

    const auto vertex_length = strlen(vertex_path);
    const auto fragment_length = strlen(fragment_path);

    assert(vertex_length < Shader_Path_Max_Length);
    assert(fragment_length < Shader_Path_Max_Length);

    gl->attach_shader(program_handle, vertex_handle);
    gl->attach_shader(program_handle, fragment_handle);
    // TODO: Check for linking errors
    gl->link_program(program_handle);

    char buffer[8192];
    GLsizei length = 0;
    gl->get_program_info_log(program_handle, sizeof(buffer), &length, buffer);
    if (length) {
        gl->delete_program(program_handle);
        log_error("Failed to link shader program.\n%s\n", buffer);
        return Gl_Invalid_Id;
    }

    return program_handle;
}

auto GLShaderProgram::initialize(const char *vertex_path, const char *fragment_path) -> bool {
    assert(_handle == 0);
    const auto vertex_length = strlen(vertex_path);
    const auto fragment_length = strlen(fragment_path);

    assert(vertex_length < Shader_Path_Max_Length);
    assert(fragment_length < Shader_Path_Max_Length);

    const auto handle = create_program(vertex_path, fragment_path);

    if (handle == Gl_Invalid_Id) {
        crash_and_burn("Failed to create shader program.");
    }

    _handle = handle;

    strcpy_s(_vertex_source.path, vertex_path);
    strcpy_s(_fragment_source.path, fragment_path);
    _vertex_source.last_modified = platform->get_file_last_modified(vertex_path);
    _fragment_source.last_modified = platform->get_file_last_modified(fragment_path);
    return true;
}

auto GLShaderProgram::relink_if_changed() -> void {
    auto vertex_last_modified = platform->get_file_last_modified(_vertex_source.path);
    auto fragment_last_modified = platform->get_file_last_modified(_fragment_source.path);
    if (vertex_last_modified > _vertex_source.last_modified ||
        fragment_last_modified > _fragment_source.last_modified) {
        const auto new_handle = create_program(_vertex_source.path, _fragment_source.path);
        if (new_handle != Gl_Invalid_Id) {
            log_info("Compiled new gl shader program with id: %d", new_handle);
            log_info("Deleting old gl shader program with id: %d", _handle);
            gl->delete_program(_handle);
            _handle = new_handle;
        }
        _vertex_source.last_modified = vertex_last_modified;
        _fragment_source.last_modified = fragment_last_modified;
    }
}


void GLShaderProgram::set_uniform(const char *name, const vec4 &vec) const {
    i32 id = gl->get_uniform_location(_handle, name);
    gl->uniform_4f(id, vec.x, vec.y, vec.z, vec.w);
}

auto GLShaderProgram::set_uniform(const char *name, const vec3 &vec) const -> void {
    i32 id = gl->get_uniform_location(_handle, name);
    gl->uniform_3f(id, vec.x, vec.y, vec.z);
}

void GLShaderProgram::set_uniform(const char *name, const mat4 &mat) const {
    i32 id = gl->get_uniform_location(_handle, name);
    gl->gl_uniform_matrix_4_fv(id, 1, GL_FALSE, mat.v);
}

void GLShaderProgram::free() {
    gl->delete_program(_handle);
    _handle = 0;
}

int ends_with(const char *s, const char *part) {
    return (strstr(s, part) - s) == (strlen(s) - strlen(part));
}

GLenum GLShaderType_from_file_name(const char *file_name) {
    if (ends_with(file_name, ".vert")) {
        return GL_VERTEX_SHADER;
    }
    if (ends_with(file_name, ".frag")) {
        return GL_FRAGMENT_SHADER;
    }
    if (ends_with(file_name, ".geom"))
        return GL_GEOMETRY_SHADER;

    if (ends_with(file_name, ".tesc"))
        return GL_TESS_CONTROL_SHADER;

    if (ends_with(file_name, ".tese"))
        return GL_TESS_EVALUATION_SHADER;

    if (ends_with(file_name, ".comp"))
        return GL_COMPUTE_SHADER;

    assert(false);

    return 0;
}


void GLShaderProgram::useProgram() const {
    assert(_handle != 0);
    gl->use_program(_handle);
    auto err = gl->get_error();
    if (err != GL_NO_ERROR) {
        printf("Found an error!\n");
        exit(1);
    }
}

auto GLGlobalUniformBufferContainer::init(UniformBuffer index, void *data, GLsizeiptr size, GLbitfield flags) -> bool {
    auto &buffer = _uniform_buffers[+index];
    buffer.data = data;
    buffer.size = size;
    buffer.index = +index;
    buffer.flags = flags;

    gl->create_buffers(1, &buffer.handle);
    gl->named_buffer_storage(buffer.handle, buffer.size, nullptr, GL_DYNAMIC_STORAGE_BIT);
    return true;
}

auto GLGlobalUniformBufferContainer::upload() const -> void {
    for (auto buffer : _uniform_buffers) {
        gl->bind_buffer_base(GL_UNIFORM_BUFFER, buffer.index, buffer.handle);
        gl->named_buffer_sub_data(buffer.handle, 0, buffer.size, buffer.data);
    }
}



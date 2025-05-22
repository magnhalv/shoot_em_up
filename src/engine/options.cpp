#include "options.hpp"
#include "globals.hpp"
#include "logger.h"
#include "memory_arena.h"

auto line_in_buffer(const char* line, LineBuffer& line_buffer) {
    for (auto& line_in_buffer : line_buffer._lines) {
        if (line == line_in_buffer) {
            return true;
        }
    }
    return false;
}

const char* graphic_options_path = R"(data\file.data)";

auto save_to_file(Options* options) -> void {
    const auto max_num_lines = 20;

    LineBuffer buffer(20, g_transient, 1024);
    buffer.push_line("[Options]");

    if (options->anti_aliasing) {
        buffer.push_line("antialiasing = 1");
    }
    else {
        buffer.push_line("antialiasing = 0");
    }

    if (options->debug_info) {
        buffer.push_line("debug_info = 1");
    }
    else {
        buffer.push_line("debug_info = 0");
    }

    // TODO: Generalize this
    auto raw_buffer_size = 0;
    char* raw_buffer = allocate<char>(*g_transient, buffer.size() + (buffer._lines.size() * 2));
    for (auto& line : buffer._lines) {
        memcpy(&raw_buffer[raw_buffer_size], line.data(), sizeof(char) * line.len());
        raw_buffer_size += line.len() + 2;
        raw_buffer[raw_buffer_size - 2] = '\r';
        raw_buffer[raw_buffer_size - 1] = '\n';
    }
    Platform->write_file(graphic_options_path, raw_buffer, raw_buffer_size);
}

auto read_from_file(Options* options) -> void {
    auto file_size = Platform->get_file_size(graphic_options_path);
    if (file_size == 0) {
        return;
    }
    char* raw_buffer = allocate<char>(*g_transient, file_size + 1);
    auto success = Platform->read_file(graphic_options_path, raw_buffer, file_size + 1);
    if (!success) {
        log_error("Options: Failed to read %s.", graphic_options_path);
        return;
    }

    LineBuffer buffer(20, g_transient, 1024);

    auto cursor = 0;
    auto start_of_line = 0;
    while (cursor < file_size) {
        if (raw_buffer[cursor] == '\r' && raw_buffer[cursor + 1] == '\n') {
            if (cursor != start_of_line) {
                buffer.push_line(&raw_buffer[start_of_line], cursor - start_of_line);
                cursor += 2;
                start_of_line = cursor;
            }
            else {
                cursor++;
            }
        }
        else {
            cursor++;
        }
    }

    options->anti_aliasing = line_in_buffer("antialiasing = 1", buffer);
    options->debug_info = line_in_buffer("debug_info = 1", buffer);
}

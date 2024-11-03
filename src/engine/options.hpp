#pragma once

#include "fixed_string.h"
#include "list.h"
#include "memory_arena.h"
#include <cstring>

struct Options {
  bool anti_aliasing;
  bool debug_info;
};

struct LineBuffer {
  List<FStr> _lines;
  MemoryArena* _arena;

  LineBuffer(i32 num_lines, MemoryArena* arena, size_t buffer_size) {
    _arena = arena->allocate_arena(buffer_size + sizeof(FStr) * num_lines);
    _lines.init(*_arena, num_lines);
  }

  auto push_line(const char* str) {
    _lines.push(FStr::create(str, *_arena));
  }

  auto push_line(const char* str, i32 length) {
    _lines.push(FStr::create(str, length, *_arena));
  }
  auto extend_last(const char* str) {
    auto length = strlen(str);
    auto& last = _lines[_lines.size() - 1];
    _arena->extend(last._data, sizeof(char) * length);
    memcpy(&last._data[last.len()], str, sizeof(char) * length);
    last._length += length;
    last._data[last._length] = '\0';
  }

  auto size() -> size_t {
    auto size = 0;
    for (auto& line : _lines) {
      size += line.len();
    }
    return size;
  }
};

auto save_to_file(Options* options) -> void;
auto read_from_file(Options* options) -> void;

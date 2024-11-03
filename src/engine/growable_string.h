#pragma once

#include <cassert>
#include <cstring>
#include <platform/types.h>

#include "logger.h"
#include "memory_arena.h"

/// @brief FixedString
struct GStr {
  [[nodiscard]] static auto create(const char* s, i32 max_length, MemoryArena& arena) -> GStr {
    auto length = strlen(s);
    char* data = static_cast<char*>(arena.allocate(max_length + 1));
    memcpy(data, s, length);
    data[length] = '\0';

    return GStr{ .m_data = data, .m_length = static_cast<i32>(length), .m_max_length = max_length };
  }

  [[nodiscard]] inline auto data() const -> const char* {
    return m_data;
  }

  [[nodiscard]] auto substring(i32 start, i32 length, MemoryArena& arena) const -> GStr {
    assert(start >= 0);
    assert(length >= 0);
    assert(start + length <= len());
    return GStr::create(&m_data[start], length, arena);
  }

  auto inline push(const char* s) {
    auto length = strlen(s);
    if (length + m_length > m_max_length) {
      crash_and_burn("Exceeded max length of GStr");
    }
    memcpy(m_data + m_length, s, length);
    m_length += length;
    m_data[m_length] = '\0';
  }

  auto inline pop(i32 num = 1) {
    if (m_length - num < 0) {
      m_length = 0;
    } else {
      m_length -= num;
    }
    printf("Length: %d\n", m_length);
    m_data[m_length] = '\0';
  }

  const char& operator[](size_t index) const {
    assert(index < m_length);
    return m_data[index];
  }

  bool operator==(const GStr other) const {
    if (m_length != other.len()) {
      return false;
    }
    return memcmp(m_data, other.m_data, m_length) == 0;
  }

  bool operator==(const char* other) const {
    auto other_len = strlen(other);
    if (m_length != other_len) {
      return false;
    }
    for (auto i = 0; i < m_length; i++) {
      if (m_data[i] != other[i]) {
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] inline auto len() const -> u32 {
    return m_length;
  }

  [[nodiscard]] inline auto is_full() const -> u32 {
    return m_length == m_max_length;
  }

  char* m_data;
  i32 m_length;
  i32 m_max_length;
};

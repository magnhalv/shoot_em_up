#pragma once

#include <cassert>

#include "math/vec2.h"
#include "math/vec3.h"
#include "memory_arena.h"
#include "hm_assert.h"
#include <platform/types.h>

template <typename T> struct List {

  explicit List() : m_size(0), m_data(nullptr), m_capacity(0) {
  }

  ~List() = default;

  auto init(MemoryArena& arena, size_t max_size) -> void {
    m_data = allocate<T>(arena, max_size);
    m_size = 0;
    m_capacity = max_size;
  }

  T& operator[](size_t index) {
    assert(index < m_size);
    return m_data[index];
  }

  const T& operator[](size_t index) const {
    assert(index < m_size);
    return m_data[index];
  }

  [[nodiscard]] auto inline data() const -> T* {
    return m_data;
  }

  [[nodiscard]] auto inline size() const -> size_t {
    return m_size;
  }

  auto inline empty() -> void {
    m_size = 0;
  }

  [[nodiscard]] auto inline is_empty() -> bool {
    return m_size == 0;
  }

  [[nodiscard]] auto inline push() -> T* {
    assert(m_size < m_capacity);
    return &m_data[m_size++];
  }

  [[nodiscard]] auto inline last() -> T* {
    return m_size > 0 ? &m_data[m_size - 1] : nullptr;
  }

  auto inline push(T* value) -> void {
    HM_ASSERT(m_size < m_capacity);
    m_data[m_size++] = *value;
  }

  auto inline push(T value) -> void {
    HM_ASSERT(m_size < m_capacity);
    m_data[m_size++] = value;
  }

  auto inline push_range(const T* value, size_t length) -> T* {
    assert(m_size + length <= m_capacity);
    memcpy(m_data + m_size, value, length);
    auto result = &m_data[m_size];
    m_size += length;
    return result;
  }

  auto inline pop(size_t num = 1) -> void {
    assert(m_size - num >= 0);
    m_size -= num;
  }

  class ListIterator {
    private:
    T* ptr;

    public:
    explicit ListIterator(T* ptr) : ptr(ptr) {
    }

    ListIterator& operator++() {
      ++ptr;
      return *this;
    }

    bool operator!=(const ListIterator& other) const {
      return ptr != other.ptr;
    }

    T& operator*() const {
      return *ptr;
    }
  };

  [[nodiscard]] ListIterator begin() const {
    return ListIterator(m_data);
  }

  [[nodiscard]] ListIterator end() const {
    return ListIterator(m_data + m_size);
  }

  [[nodiscard]] inline auto is_full() -> bool {
    return m_size == m_capacity;
  }

  private:
  size_t m_capacity{};
  size_t m_size;
  T* m_data;
};

extern template class List<i32>;

extern template class List<f32>;

extern template class List<vec3>;

extern template class List<vec2>;

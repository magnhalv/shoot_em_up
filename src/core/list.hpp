#pragma once

#include <cassert>

#include <platform/platform.h>
#include <platform/types.h>

#include <core/memory_arena.h>

#include "math/vec2.h"
#include "math/vec3.h"

template <typename T> struct List {

    List() : m_count(0), m_data(nullptr), m_max_count(0) {
    }

    ~List() = default;

    auto init(MemoryArena* arena, i32 max_count) -> void {
        m_data = allocate<T>(arena, max_count);
        m_count = 0;
        m_max_count = max_count;
    }

    T& operator[](i32 index) {
        Assert(index < m_count && index >= 0);
        return m_data[index];
    }

    const T& operator[](i32 index) const {
        Assert(index < m_count);
        return m_data[index];
    }

    auto data() const -> T* {
        return m_data;
    }

    auto count() const -> i32 {
        return m_count;
    }

    auto clear() -> void {
        m_count = 0;
        memset(m_data, 0, m_max_count * sizeof(T));
    }

    auto is_empty() -> bool {
        return m_count == 0;
    }

    auto push() -> T* {
        assert(m_count < m_max_count);
        return &m_data[m_count++];
    }

    auto pushi(i32* index) -> T* {
        assert(m_count < m_max_count);
        *index = m_count++;
        return &m_data[*index];
    }

    auto last() -> T* {
        return m_count > 0 ? &m_data[m_count - 1] : nullptr;
    }

    auto push(T* value) -> void {
        Assert(m_count < m_max_count);
        m_data[m_count++] = *value;
    }

    auto push(T value) -> void {
        Assert(m_count < m_max_count);
        m_data[m_count++] = value;
    }

    auto pop(i32 num = 1) -> void {
        assert(m_count - num >= 0);
        m_count -= num;
    }

    struct ListIterator {
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
        return ListIterator(m_data + m_count);
    }

    [[nodiscard]] inline auto is_full() -> bool {
        return m_count == m_max_count;
    }

    private:
    i32 m_max_count{};
    i32 m_count;
    T* m_data;
};

extern template struct List<i32>;

extern template struct List<f32>;

extern template struct List<vec3>;

extern template struct List<vec2>;

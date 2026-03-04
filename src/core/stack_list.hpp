#pragma once

#include <cassert>

#include <platform/types.h>

#include <core/memory_arena.h>
#include <core/span.hpp>

template <typename T, u32 Capacity> struct StackList {
    T& operator[](i32 index) {
        assert(index < m_count && index >= 0);
        return m_data[index];
    }

    const T& operator[](i32 index) const {
        assert(index < m_count);
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
    }

    auto is_empty() -> bool {
        return m_count == 0;
    }

    auto push() -> T* {
        assert(m_count < Capacity);
        return &m_data[m_count++];
    }

    auto pushi(i32* index) -> T* {
        assert(m_count < Capacity);
        *index = m_count++;
        return &m_data[*index];
    }

    auto last() -> T* {
        return m_count > 0 ? &m_data[m_count - 1] : nullptr;
    }

    auto push(T value) -> void {
        Assert(m_count < Capacity);
        m_data[m_count++] = value;
    }

    auto pop(i32 num = 1) -> void {
        assert(m_count - num >= 0);
        m_count -= num;
    }

    auto as_span() -> Span<T> {
        return Span{ m_data, m_count };
    }

    struct iterator {
        private:
        T* ptr;

        public:
        explicit iterator(T* ptr) : ptr(ptr) {
        }

        iterator& operator++() {
            ++ptr;
            return *this;
        }

        bool operator!=(const iterator& other) const {
            return ptr != other.ptr;
        }

        T& operator*() const {
            return *ptr;
        }
    };

    struct const_iterator {
        private:
        const T* ptr;

        public:
        explicit const_iterator(const T* ptr) : ptr(ptr) {
        }

        const_iterator& operator++() {
            ++ptr;
            return *this;
        }

        bool operator!=(const const_iterator& other) const {
            return ptr != other.ptr;
        }

        const T& operator*() const {
            return *ptr;
        }
    };

    [[nodiscard]] iterator begin() {
        return iterator(m_data);
    }
    [[nodiscard]] iterator end() {
        return iterator(m_data + m_count);
    }

    [[nodiscard]] const_iterator begin() const {
        return const_iterator(m_data);
    }
    [[nodiscard]] const_iterator end() const {
        return const_iterator(m_data + m_count);
    }

    [[nodiscard]] inline auto is_full() -> bool {
        return m_count == Capacity;
    }

    private:
    i32 m_count;
    T m_data[Capacity];
};

#pragma once

#include <cassert>

#include <platform/types.h>

#include <core/memory_arena.h>

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

    struct StackListIterator {
        private:
        T* ptr;

        public:
        explicit StackListIterator(T* ptr) : ptr(ptr) {
        }

        StackListIterator& operator++() {
            ++ptr;
            return *this;
        }

        bool operator!=(const StackListIterator& other) const {
            return ptr != other.ptr;
        }

        T& operator*() const {
            return *ptr;
        }
    };

    [[nodiscard]] StackListIterator begin() const {
        return StackListIterator(m_data);
    }

    [[nodiscard]] StackListIterator end() const {
        return StackListIterator(m_data + m_count);
    }

    [[nodiscard]] inline auto is_full() -> bool {
        return m_count == Capacity;
    }

    private:
    i32 m_count;
    T m_data[Capacity];
};

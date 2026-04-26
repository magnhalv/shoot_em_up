#pragma once

#include <cassert>

#include <engine/array.h>
#include <platform/types.h>

template <typename T, u32 Capacity> struct StackArray {
    T& operator[](u32 index) {
        assert(index < Capacity);
        return m_data[index];
    }

    const T& operator[](u32 index) const {
        assert(index < Capacity);
        return m_data[index];
    }

    auto data() -> T* {
        return m_data;
    }

    auto data() const -> const T* {
        return m_data;
    }

    constexpr auto count() const -> u32 {
        return Capacity;
    }

    auto to_array() -> Array<T> {
        return Array<T>(m_data, Capacity);
    }

    auto fill(T value) -> void {
        for (u32 i = 0; i < Capacity; i++) {
            m_data[i] = value;
        }
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
        return iterator(m_data + Capacity);
    }

    [[nodiscard]] const_iterator begin() const {
        return const_iterator(m_data);
    }
    [[nodiscard]] const_iterator end() const {
        return const_iterator(m_data + Capacity);
    }

    T m_data[Capacity];
};

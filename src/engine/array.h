#pragma once

#include <platform/types.h>

#include "math/vec2.h"
#include "math/vec3.h"
#include "memory_arena.h"
#include <engine/hm_assert.h>

template <typename T> struct Array {
    static auto create(size_t size, MemoryArena& arena) -> Array<T>* {
        auto* result = allocate<Array>(arena, 1);
        result->init(arena, size);
        return result;
    }

    Array() : m_size(0), m_data(nullptr) {
    }

    Array(T* values, size_t size) : m_data{ values }, m_size{ size } {
    }

    ~Array() = default;

    auto init(T* values, size_t size) -> void {
        m_data = values;
        m_size = size;
    }

    auto init(MemoryArena& arena, size_t size) -> void {
        m_data = allocate<T>(arena, size);
        m_size = size;
    }

    T& operator[](size_t index) {
        HM_ASSERT(index < m_size);
        return m_data[index];
    }

    const T& operator[](size_t index) const {
        HM_ASSERT(index < m_size);
        return m_data[index];
    }

    [[nodiscard]] auto inline data() const -> T* {
        return m_data;
    }

    [[nodiscard]] auto inline size() const -> size_t {
        return m_size;
    }

    class ArrayIterator {
        private:
        T* ptr;

        public:
        explicit ArrayIterator(T* ptr) : ptr(ptr) {
        }

        ArrayIterator& operator++() {
            ++ptr;
            return *this;
        }

        bool operator!=(const ArrayIterator& other) const {
            return ptr != other.ptr;
        }

        T& operator*() const {
            return *ptr;
        }
    };

    [[nodiscard]] ArrayIterator begin() const {
        return ArrayIterator(m_data);
    }

    [[nodiscard]] ArrayIterator end() const {
        return ArrayIterator(m_data + m_size);
    }

    size_t m_size;
    T* m_data;
};

template <typename T> auto inline span(Array<T>& arr, size_t start, size_t end = -1) -> Array<T> {
    if (end == -1) {
        end = arr.size();
    }
    HM_ASSERT(start >= 0 && start < end);
    HM_ASSERT(end <= arr.size());

    return Array<T>{ &arr[start], end - start };
}

extern template class Array<i32>;

extern template class Array<f32>;

extern template class Array<vec3>;

extern template class Array<vec2>;

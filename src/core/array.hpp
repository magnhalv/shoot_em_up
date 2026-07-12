#pragma once

#include <cassert>
#include <cstdio>

#include <platform/assert.hpp>
#include <platform/types.hpp>

#include <core/memory.hpp>
#include <core/memory_arena.hpp>

#include <math/vec2.hpp>
#include <math/vec3.hpp>

template <typename T> struct Array {
    static auto create(size_t count, MemoryArena& arena) -> Array<T> {
        Array<T> result;
        result.init_arena(arena, count);
        return result;
    }

    Array() : m_count(0), m_data(nullptr) {
    }

    Array(T* values, size_t size) : m_data{ values }, m_count{ size } {
    }

    ~Array() = default;

    auto init(T* values, size_t size) -> void {
        m_data = values;
        m_count = size;
    }

    auto init_arena(MemoryArena& arena, u64 size) -> void {
        m_data = allocate<T>(arena, size);
        m_count = size;
    }

    T& operator[](size_t index) {
        Assert(index < m_count);
        return m_data[index];
    }

    const T& operator[](size_t index) const {
        assert(index < m_count);
        return m_data[index];
    }

    [[nodiscard]] auto inline data() const -> T* {
        return m_data;
    }

    [[nodiscard]] auto inline constexpr count() const -> size_t {
        return m_count;
    }

    [[nodiscard]] auto inline size() const -> size_t {
        return m_count * sizeof(T);
    }

    struct ArrayIterator {
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
        return ArrayIterator(m_data + m_count);
    }

    size_t m_count;
    T* m_data;
};

template <typename T> auto inline concat(Array<T>& arr1, Array<T>& arr2, MemoryArena& arena) -> Array<T> {
    Array<T> result = Array<T>::create(arr1.count() + arr2.count(), arena);
    copy_memory(arr1.data(), result.data(), arr1.size());
    copy_memory(arr2.data(), result.data() + arr1.count(), arr2.size());

    return result;
}

template <typename T> auto inline span(Array<T>& arr, size_t start, size_t end = 0) -> Array<T> {
    if (end == 0) {
        end = arr.count();
    }
    assert(start >= 0 && (start < end || (end == 0 && start == 0)));
    assert(end <= arr.count());

    return Array<T>{ &arr[start], end - start };
}

extern template struct Array<i32>;

extern template struct Array<f32>;

extern template struct Array<vec3>;

extern template struct Array<vec2>;

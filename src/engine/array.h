#pragma once

#include <cassert>
#include <cstdio>
#include <ostream>

#include <platform/types.h>

#include "math/vec3.h"
#include "math/vec2.h"
#include "memory_arena.h"

template<typename T>
struct Array {
    static auto create(size_t size, MemoryArena &arena) -> Array<T> * {
        auto *result = allocate<Array>(arena, 1);
        result->init(arena, size);
        return result;
    }

    Array() : _size(0), _data(nullptr) {}

    Array(T *values, size_t size) : _data{values}, _size{size} {}

    ~Array() = default;

    auto init(T *values, size_t size) -> void {
        _data = values;
        _size = size;
    }

    auto init(MemoryArena &arena, size_t size) -> void {
        _data = allocate<T>(arena, size);
        _size = size;
    }

    T &operator[](size_t index) {
        //HM_ASSERT(index < _size);
        return _data[index];
    }

    const T &operator[](size_t index) const {
        assert(index < _size);
        return _data[index];
    }

    [[nodiscard]] auto inline data() const -> T * {
        return _data;
    }

    [[nodiscard]] auto inline size() const -> size_t {
        return _size;
    }

    class ArrayIterator {
    private:
        T *ptr;

    public:
        explicit ArrayIterator(T *ptr) : ptr(ptr) {}

        ArrayIterator &operator++() {
            ++ptr;
            return *this;
        }

        bool operator!=(const ArrayIterator &other) const {
            return ptr != other.ptr;
        }

        T &operator*() const {
            return *ptr;
        }
    };

    [[nodiscard]] ArrayIterator begin() const {
        return ArrayIterator(_data);
    }

    [[nodiscard]] ArrayIterator end() const {
        return ArrayIterator(_data + _size);
    }

    size_t _size;
    T *_data;
};

template<typename T>
auto inline span(Array<T> &arr, size_t start, size_t end = -1) -> Array<T> {
    if (end == -1) {
        end = arr.size();
    }
    assert(start >= 0 && start < end);
    assert(end <= arr.size());

    return Array<T>{&arr[start], end - start};
}

extern template
class Array<i32>;

extern template
class Array<f32>;

extern template
class Array<vec3>;

extern template
class Array<vec2>;

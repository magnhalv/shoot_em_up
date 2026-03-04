#pragma once

#include <platform/types.h>

template <typename T> struct Span {

    Span(T* data, Size count) : m_data(nullptr), m_count(count) {
    }

    ~Span() = default;

    T& operator[](Size index) {
        Assert(index < m_count && index >= 0);
        return m_data[index];
    }

    const T& operator[](Size index) const {
        Assert(index < m_count);
        return m_data[index];
    }

    auto data() const -> T* {
        return m_data;
    }

    auto size() const -> Size {
        return m_count * sizeof(T);
    }

    auto count() const -> Size {
        return m_count;
    }

    auto is_empty() -> bool {
        return m_count == 0;
    }

    auto first() -> T* {
        return m_count > 0 ? &m_data[0] : nullptr;
    }

    auto last() -> T* {
        return m_count > 0 ? &m_data[m_count - 1] : nullptr;
    }

    struct SpanIterator {
        private:
        T* ptr;

        public:
        explicit SpanIterator(T* ptr) : ptr(ptr) {
        }

        SpanIterator& operator++() {
            ++ptr;
            return *this;
        }

        bool operator!=(const SpanIterator& other) const {
            return ptr != other.ptr;
        }

        T& operator*() const {
            return *ptr;
        }
    };

    [[nodiscard]] SpanIterator begin() const {
        return SpanIterator(m_data);
    }

    [[nodiscard]] SpanIterator end() const {
        return SpanIterator(m_data + m_count);
    }

    private:
    Size m_count{};
    T* m_data;
};

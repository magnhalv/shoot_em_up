#pragma once

#include "macros.hpp"

#define DeferLoop_(begin, end, n) for (int CONCAT(_i_, n) = ((begin), 0); !CONCAT(_i_, n); CONCAT(_i_, n) += 1, (end))

#define DeferLoop(begin, end) DeferLoop_(begin, end, __COUNTER__)

template <typename F> struct scope_exit : F {
    ~scope_exit() {
        static_cast<F&>(*this)();
    }
};

#define Defer(...)                                         \
    auto CONCAT(scope_exit_, __COUNTER__) = ::scope_exit { \
        [&] __VA_ARGS__                                    \
    }

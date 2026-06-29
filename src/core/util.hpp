#pragma once

#include <platform/types.hpp>

#include <core/string8.hpp>
#include <core/array.hpp>

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

auto inline count_digits(i32 n) -> i32 {
    i32 count = 0;
    if (n == 0) {
        return 1;
    }
    if (n < 0) {
        n = -n;
    }
    while (n > 0) {
        count++;
        n /= 10;
    }

    return count;
}

auto inline fill_inc(Array<i32>& arr, i32 start = 0) {
    for (size_t i = 0; i < arr.count(); i++) {
        arr[i] = start + (i32)i;
    }
}

auto inline format_bytes(u64 bytes, MemoryArena& arena) -> string8 {
    u64 gb = GigaBytes(1);
    u64 mb = MegaBytes(1);
    u64 kb = KiloBytes(1);

    if (bytes > gb) {
        f64 in_gb = (f64)bytes / gb;
        return string8_format(&arena, "%.1f GB", in_gb);
    }
    else if (bytes > mb) {
        f64 in_mb = (f64)bytes / mb;
        return string8_format(&arena, "%.1f MB", in_mb);
    }
    else if (bytes > kb) {
        f64 in_kb = (f64)bytes / kb;
        return string8_format(&arena, "%.1f KB", in_kb);
    }
    else {
        return string8_format(&arena, "%d bytes", bytes);
    }
}

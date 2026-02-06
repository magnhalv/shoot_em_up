#pragma once

#include <cfloat>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <type_traits>

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

// signed integers
#define i8_min INT8_MIN
#define i8_max INT8_MAX

#define i16_min INT16_MIN
#define i16_max INT16_MAX

#define i32_min INT32_MIN
#define i32_max INT32_MAX

#define i64_min INT64_MIN
#define i64_max INT64_MAX

// unsigned integers
#define u8_min 0
#define u8_max UINT8_MAX

#define u16_min 0
#define u16_max UINT16_MAX

#define u32_min 0
#define u32_max UINT32_MAX

#define u64_min 0
#define u64_max UINT64_MAX

// floating point
#define f32_min (-FLT_MAX)
#define f32_max FLT_MAX

#define f64_min (-DBL_MAX)
#define f64_max DBL_MAX

using memory_index = size_t;

const u64 us_in_second = 1000000;
const u64 ms_in_second = 1000;

const f32 F32_MAX = FLT_MAX;
const f32 PI = 3.1415927f;

using TimeStamp = u64;

constexpr u64 KiloBytes(u64 num_kb) noexcept {
    return num_kb * 1024;
}
constexpr u64 MegaBytes(u64 num_mb) noexcept {
    return KiloBytes(1024 * num_mb);
}
constexpr u64 GigaBytes(u64 num_gb) noexcept {
    return MegaBytes(1024 * num_gb);
}

struct MemoryBlock {
    void* data;
    u32 size;
};

const i32 Max_Path_Length = 128;

// Returns the length of a static array
template <typename T, std::size_t N> constexpr std::size_t array_length(const T (&)[N]) noexcept {
    return N;
}

template <typename T>
constexpr auto operator+(T e) noexcept -> std::enable_if_t<std::is_enum<T>::value, std::underlying_type_t<T>> {
    return static_cast<std::underlying_type_t<T>>(e);
}

#define local_persist static
#define global_variable static
#define internal static

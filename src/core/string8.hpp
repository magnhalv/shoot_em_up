#pragma once

#include <platform/platform.h>
#include <platform/types.h>

#include <core/list.hpp>
#include <core/memory.h>
#include <core/memory_arena.h>
#include <core/span.hpp>

#include <third-party/stb_sprintf.hpp>
// https://graphics.stanford.edu/~seander/bithacks.html
#define HAS_ZERO_BYTE (v)(((v) - 0x01010101UL) & ~(v) & 0x80808080UL)
auto inline c_string_length(const char* str) -> Size {
    i32 counter = 0;
    while ((*str++) != '\0') {
        counter++;
    }
    return counter;
}

struct string8 {
    const char* data;
    Size size;

    string8() : data{ nullptr }, size{ 0 } {
    }

    string8(const char* s) : data{ s }, size{ c_string_length(s) } {
    }

    string8(const char* s, Size length) : data{ s }, size{ length } {
    }

    // TODO
    auto code_point_count() -> i32 {
        return size;
    }

    // TODO
    auto grapheme_cluster_count() -> i32 {
        return size;
    }

    const char& operator[](Size index) {
        Assert(index < size && index >= 0);
        return data[index];
    }
};

auto inline string8_concat(string8 a, string8 b, MemoryArena* arena) -> string8 {
    Size new_length = a.size + b.size;
    char* memory = allocate<char>(arena, new_length);

    copy_memory((void*)a.data, (void*)memory, a.size);
    copy_memory((void*)b.data, (void*)(memory + a.size), b.size);

    return { memory, new_length };
}

auto inline string8_format(MemoryArena* arena, const char* format, ...) -> string8 {
    const u32 buffer_size = 512;
    char* buffer = allocate<char>(arena, buffer_size);

    va_list args;
    va_start(args, format);
    i32 str_length = stbsp_vsnprintf(buffer, buffer_size, format, args);
    va_end(args);

    string8 result = { buffer, str_length };
    arena->shrink(buffer, buffer_size - str_length - 1);
    return result;
}

auto inline cstr_find_last(const char character, const char* cstr) -> i32 {
    Size length = c_string_length(cstr);
    for (i32 i = length - 1; i >= 0; i--) {
        if (cstr[i] == character) {
            return i;
        }
    }
    return -1;
}

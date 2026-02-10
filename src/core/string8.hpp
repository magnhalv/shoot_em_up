#pragma once

#include <platform/platform.h>
#include <platform/types.h>

#include <core/memory.h>
#include <core/memory_arena.h>

#include <third-party/stb_sprintf.hpp>

auto inline c_string_length(const char* str) -> i32 {
    i32 counter = 0;
    while ((*str++) != '\0') {
        counter++;
    }
    return counter;
}

struct string8 {
    const char* data;
    i32 size;

    string8() : data{ nullptr }, size{ 0 } {
    }

    string8(const char* s) : data{ s }, size{ c_string_length(s) } {
        Assert(size < i32_max);
    }

    string8(const char* s, i32 length) : data{ s }, size{ length } {
    }

    // TODO
    auto code_point_count() -> i32 {
        return size;
    }

    // TODO
    auto grapheme_cluster_count() -> i32 {
        return size;
    }

    const char& operator[](i32 index) {
        Assert(index < size && index >= 0);
        return data[index];
    }
};

auto inline string8_concat(string8 a, string8 b, MemoryArena* arena) -> string8 {
    i32 new_length = a.size + b.size;
    char* memory = allocate<char>(arena, new_length);

    copy_memory((void*)a.data, (void*)memory, a.size);
    copy_memory((void*)b.data, (void*)(memory + a.size), b.size);

    return { memory, new_length };
}

auto inline string8_format(MemoryArena* arena, const char* format, ...) -> string8 {
    const i32 buffer_size = 24;
    char* buffer = allocate<char>(arena, buffer_size);

    va_list args;
    va_start(args, format);
    i32 str_length = stbsp_vsnprintf(buffer, buffer_size, format, args);
    va_end(args);

    string8 result = { buffer, str_length };
    arena->shrink(buffer, buffer_size - str_length - 1);
    return result;
}

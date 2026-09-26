#pragma once

#include <platform/platform.hpp>
#include <platform/types.hpp>

#include <core/list.hpp>
#include <core/memory.hpp>
#include <core/memory_arena.hpp>
#include <core/span.hpp>

#include <third-party/stb_sprintf.hpp>
// https://graphics.stanford.edu/~seander/bithacks.html
#define HAS_ZERO_BYTE (v)(((v) - 0x01010101UL) & ~(v) & 0x80808080UL)
auto inline c_string_length(const char* str) -> Size {
    Size counter = 0;
    while ((*str++) != '\0') {
        counter++;
    }
    return counter;
}

struct String8 {
    Size size;
    char* data;

    static auto create_empty(Size size, MemoryArena* arena) -> String8 {
        String8 result;
        result.size = size;
        result.data = allocate<char>(arena, size);
        return result;
    }

    static auto create_by_copy(char* cstr, MemoryArena* arena) -> String8 {
        String8 result;
        result.size = c_string_length(cstr);
        result.data = allocate<char>(arena, result.size);
        copy_memory(cstr, result.data, result.size);
        return result;
    }

    auto code_point_count() -> Size {
        return size;
    }

    auto grapheme_cluster_count() -> Size {
        return size;
    }

    const char& operator[](Size index) {
        Assert(index < size && index >= 0);
        return data[index];
    }
};

struct CString8 {
    Size size;
    const char* data;

    CString8() : size{ 0 }, data{ nullptr } {
    }

    CString8(const char* s) : size{ c_string_length(s) }, data{ s } {
    }

    CString8(const char* s, Size length) : size{ length }, data{ s } {
    }

    CString8(const String8& s) : size{ s.size }, data(s.data) {
    }

    static auto create_empty(Size size, MemoryArena* arena) -> CString8 {
        CString8 result;
        result.size = size;
        result.data = allocate<char>(arena, size);
        return result;
    }

    // TODO
    auto code_point_count() -> Size {
        return size;
    }

    // TODO
    auto grapheme_cluster_count() -> Size {
        return size;
    }

    const char& operator[](Size index) {
        Assert(index < size && index >= 0);
        return data[index];
    }
};

auto inline string8_concat(CString8 a, CString8 b, MemoryArena* arena) -> CString8 {
    Size new_length = a.size + b.size;
    char* memory = allocate<char>(arena, new_length);

    copy_memory((void*)a.data, (void*)memory, a.size);
    copy_memory((void*)b.data, (void*)(memory + a.size), b.size);

    return { memory, new_length };
}

auto inline string8_equal(CString8 a, CString8 b) -> bool {
    if (a.size != b.size) {
        return false;
    }

    if (a.data == b.data) {
        return true;
    }

    return is_memory_equal((void*)a.data, (void*)b.data, a.size);
}

auto inline string8_format(MemoryArena* arena, const char* format, ...) -> String8 {
    const u32 buffer_size = 512;
    char* buffer = allocate<char>(arena, buffer_size);

    va_list args;
    va_start(args, format);
    i32 str_length = stbsp_vsnprintf(buffer, buffer_size, format, args);
    va_end(args);

    arena->shrink(buffer, buffer_size - str_length - 1);
    String8 result;
    result.data = buffer;
    result.size = str_length;
    return result;
}

auto inline cstr_find_last(const char character, const char* cstr) -> Size {
    Size length = c_string_length(cstr);
    for (Size i = length - 1; i >= 0; i--) {
        if (cstr[i] == character) {
            return i;
        }
    }
    return -1;
}

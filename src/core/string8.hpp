#pragma once

#include <cstring> // TODO: Remove

#include <platform/types.h>

#include <core/memory.h>
#include <core/memory_arena.h>

struct string8 {
    const char* data;
    u64 length;

    string8(const char* s) : data{ s }, length{ strlen(s) } {
    }

    string8(const char* s, u64 length) : data{ s }, length{ length } {
    }

    const char& operator[](u64 index) {
        Assert(index < length && index >= 0);
        return data[index];
    }
};

auto inline string8_concat(string8 a, string8 b, MemoryArena* arena) -> string8 {
    u64 new_length = a.length + b.length;
    char* memory = allocate<char>(arena, new_length);

    copy_memory((void*)a.data, (void*)memory, a.length);
    copy_memory((void*)b.data, (void*)(memory + a.length), b.length);

    return { memory, new_length };
}

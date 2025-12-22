#pragma once

#include <cstring> // TODO: Remove

#include <platform/types.h>

struct string8 {
    const char* data;
    u64 length;

    string8(const char* s) : data{ s }, length{ strlen(s) } {
    }
};

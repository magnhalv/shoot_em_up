#pragma once

#include "fixed_string.h"
#include "linked_list.h"

struct LinkedListBuffer {
    LinkedList<FStr> list;
    MemoryArena* _arena = nullptr;

    auto init(size_t buf_size, MemoryArena &parent) {
        _arena = parent.allocate_arena(buf_size);
    }

    auto add(const char *str) -> void {
        auto new_entry = list.alloc(*_arena);
        new_entry->init(str, *_arena);
    }

    auto add(FStr &str) -> void {
        auto new_entry = list.alloc(*_arena);
        new_entry->init(str.data(), *_arena);
    }
};
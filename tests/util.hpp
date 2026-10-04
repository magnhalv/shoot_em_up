#pragma once

#include <csetjmp>
#include <cstdlib>

#include "core/stack_array.hpp"
#include "doctest.h"

#include <core/memory_arena.hpp>

const size_t default_size = KiloBytes(1);

struct SingleArenaFixture {
    SingleArenaFixture() {
        arena.init(malloc(default_size), default_size);
    }

    ~SingleArenaFixture() {
        free(arena.memory);
    }

    public:
    MemoryArena arena;
};

struct TwoArenaFixture {
    TwoArenaFixture() {
        for (MemoryArena& arena : arenas) {
            arena.init(malloc(default_size), default_size);
        }
    }

    ~TwoArenaFixture() {
        for (MemoryArena& arena : arenas) {
            free(arena.memory);
        }
    }

    public:
    StackArray<MemoryArena, 4> arenas;
};

struct TransientFixture {
    TransientFixture() {
        local.init(malloc(default_size), default_size);
        transient.init(malloc(default_size), default_size);
        set_transient_arena(&transient);
    }

    ~TransientFixture() {
        unset_transient_arena();
        free(local.memory);
        free(transient.memory);
    }

    public:
    MemoryArena local;
    MemoryArena transient;
};

#define CHECK_CRASH(expr, msg)                         \
    do {                                               \
        jmp_buf buf;                                   \
        auto has_crashed = false;                      \
        if (setjmp(buf) == 0) {                        \
            set_crash_jump(&buf);                      \
            expr;                                      \
            CHECK(has_crashed);                        \
        }                                              \
        else {                                         \
            has_crashed = true;                        \
        }                                              \
        CHECK(has_crashed);                            \
        CHECK(global_has_crashed);                     \
        CHECK(strcmp(msg, global_crash_message) == 0); \
    } while (0)

#pragma once

#include <platform/assert.hpp>
#include <platform/types.hpp>

#include <core/memory_arena.hpp>
#include <core/stack_array.hpp>

struct PlatformWorkQueue;
struct ThreadContext {
    i32 thread_id;
    i32 thread_idx;
    StackArray<MemoryArena*, 2> arenas;
    PlatformWorkQueue* queue;
};

inline global_variable thread_local ThreadContext* thread_tcxt = nullptr;

inline auto set_tctx_selected(ThreadContext* context) -> void {
    thread_tcxt = context;
}

inline auto tctx_selected() -> ThreadContext* {
    return thread_tcxt;
}

inline auto tctx_get_scratch(MemoryArena** conflicts, Size count) -> MemoryArena* {
    ThreadContext* tcxt = tctx_selected();
    Assert(tcxt);
    for (MemoryArena* candiate : tcxt->arenas) {
        bool has_conflict = false;

        for (Size i = 0; i < count; i++) {
            if (conflicts[i] == candiate) {
                has_conflict = true;
                break;
            }
        }

        if (!has_conflict) {
            return candiate;
        }
    }

    return nullptr;
}

inline auto scratch_begin(MemoryArena** conflicts = {}, Size count = 0) -> Temp {
    MemoryArena* arena = tctx_get_scratch(conflicts, count);
    return temp_begin(arena);
}

inline auto scratch_end(Temp temp) -> void {
    temp_end(temp);
}

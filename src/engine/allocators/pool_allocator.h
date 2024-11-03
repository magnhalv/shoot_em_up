#pragma once

#include <platform/types.h>
#include <cinttypes>

const u8 GUARD_SIZE = sizeof(GUARD_PATTERN);

struct PoolAllocator {
    u64 num_pools;
    u64 pool_size;
    u8 *memory;
    u64 memory_size;

    u64 allocated_pools;

    [[nodiscard]] static auto calc_total_size(u64 req_num_pools, u64 req_pool_size) -> u64 {
        return (req_pool_size + GUARD_SIZE) * req_num_pools + GUARD_SIZE;
    }

    auto init(u64 in_num_pools, u64 in_pool_size, void *in_memory, u64 in_memory_size) -> void {
        assert(in_num_pools <= 64);
        assert(in_memory_size == calc_total_size(in_num_pools, in_pool_size));
        num_pools = in_num_pools;
        pool_size = in_pool_size;
        memory = static_cast<u8 *>(in_memory);
        memory_size = in_memory_size;
        allocated_pools = 0;

        u32 *value = reinterpret_cast<u32 *>(memory);
        *value = GUARD_PATTERN;
        for (auto i = 0; i < num_pools; i++) {
            u64 pos = ((pool_size + GUARD_SIZE) * i) + pool_size + GUARD_SIZE;
            value = reinterpret_cast<u32 *>(&memory[pos]);
            *value = GUARD_PATTERN;
        }
    }

    auto allocate() -> void * {
        for (auto i = 0; i < num_pools; i++) {
            if (is_free(i)) {
                log_info("PoolAllocator: allocating pool %d", i);
                set_pool_to_allocated(i);
                return get_pool(i);
            }
        }

        log_error("PoolAllocator: Tried to allocate pool, but there are no pools left. Exiting...");
        exit(1);
    }

    auto free(void *pool) -> void {
        auto diff = static_cast<u8 *>(pool) - memory - GUARD_SIZE;
        auto idx = static_cast<u64>(diff / (pool_size + GUARD_SIZE));
        set_pool_to_free(idx);
        memset(pool, 0, pool_size);
        log_info("PoolAllocator: freed pool %d", idx);
    }

    auto check_integrity() const -> void {
        for (auto idx = 0; idx < num_pools; idx++) {
            u64 pos = ((pool_size + GUARD_SIZE) * idx) + pool_size + GUARD_SIZE;
            u32 *value = reinterpret_cast<u32 *>(&memory[pos]);
            if (*value != GUARD_PATTERN) {
                log_error("Check integrity failed. Memory guard is corrupted. Exiting...");
                exit(1);
            }
        }

        for (auto idx = 0; idx < num_pools; idx++) {
            if (!is_free(idx)) {
                continue;
            }
            u64 pos = ((pool_size + GUARD_SIZE) * idx) + GUARD_SIZE;
            u32 *value = reinterpret_cast<u32 *>(&memory[pos]);
            auto size = pool_size;
            while (size > 0) {
                if (*value++ != 0) {
                    log_error("Check integrity failed:\n  Unused pool '%d' is corrupted at byte %d.\n  Exiting...", idx,
                              (pool_size - size));
                    exit(1);
                }
                size -= sizeof(u32);
            }
            assert(size == 0);
        }

    }

    [[nodiscard]] inline auto is_free(u32 pool_idx) const -> bool {
        u64 one = 1;
        return (allocated_pools & (one << pool_idx)) == 0;
    }

    inline auto set_pool_to_allocated(u32 pool_idx) -> void {
        u64 one = 1;
        allocated_pools |= (one << pool_idx);
    }

    inline auto set_pool_to_free(u32 pool_idx) -> void {
        u64 one = 1;
        allocated_pools &= ~(one << pool_idx);
    }

    [[nodiscard]] inline auto get_pool(u32 pool_idx) const -> void * {
        auto pos = (pool_size + GUARD_SIZE) * pool_idx + GUARD_SIZE;
        return static_cast<void *>(&memory[pos]);
    }
};

#pragma once

#include <platform/types.h>

#include <core/memory.h>
#include <core/memory_arena.h>

auto inline merge_sort_merge(i32* a, i32 start, i32 mid, i32 end, i32* b) {
    i32 a_idx = start;
    i32 b_idx = mid;
    for (i32 i = start; i < end; i++) {
        if (b_idx == end || ((a_idx < mid) && (b[a_idx] <= b[b_idx]))) {
            a[i] = b[a_idx];
            a_idx++;
        }
        else {
            a[i] = b[b_idx];
            b_idx++;
        }
    }
}

auto inline merge_sort_split(i32* a, i32 start, i32 end, i32* b) {
    if (end - start <= 1) {
        return;
    }
    i32 mid = start + (end - start) / 2;
    merge_sort_split(b, start, mid, a);
    merge_sort_split(b, mid, end, a);
    merge_sort_merge(a, start, mid, end, b);
}

auto inline merge_sort(i32* values, i32 count, MemoryArena* temp_arena) {
    i32* b = allocate<i32>(temp_arena, count);
    copy_memory(values, b, sizeof(i32) * count);
    i32* a = values;
    merge_sort_split(a, 0, count, b);
}

auto inline merge_sort_merge_indices(i32* dest, i32* dest_indices, i32 start, i32 mid, i32 end, i32* src, i32* src_indices) {
    i32 a_idx = start;
    i32 b_idx = mid;
    for (i32 i = start; i < end; i++) {
        if (b_idx == end || ((a_idx < mid) && (src[a_idx] <= src[b_idx]))) {
            dest[i] = src[a_idx];
            dest_indices[i] = src_indices[a_idx];
            a_idx++;
        }
        else {
            dest[i] = src[b_idx];
            dest_indices[i] = src_indices[b_idx];
            b_idx++;
        }
    }
}

auto inline merge_sort_split_indices(i32* dest, i32* dest_indices, i32 start, i32 end, i32* src, i32* src_indices) {
    if (end - start <= 1) {
        return;
    }
    i32 mid = start + (end - start) / 2;
    merge_sort_split_indices(src, src_indices, start, mid, dest, dest_indices);
    merge_sort_split_indices(src, src_indices, mid, end, dest, dest_indices);
    merge_sort_merge_indices(dest, dest_indices, start, mid, end, src, src_indices);
}

auto inline merge_sort_indices(i32* values, i32 count, MemoryArena* arena) -> i32* {
    Assert(values);
    i32* src = allocate<i32>(arena, count);
    i32* src_indices = allocate<i32>(arena, count);
    i32* dest_indices = allocate<i32>(arena, count);
    copy_memory(values, src, sizeof(i32) * count);

    for (i32 i = 0; i < count; i++) {
        dest_indices[i] = i;
        src_indices[i] = i;
    }
    i32* dest = values;
    merge_sort_split_indices(dest, dest_indices, 0, count, src, src_indices);

    return dest_indices;
}

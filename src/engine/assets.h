#pragma once


#include <platform/types.h>

#include <engine/memory_arena.h>
#include <engine/array.h>

struct FileAsset {
    char path[Max_Path_Length];
    TimeStamp last_modified;
};

typedef struct {
  i32 width;
  i32 height;
  u8 *data;
} Bitmap;

auto load_bitmap(const char* path, MemoryArena *arena) -> Bitmap*;

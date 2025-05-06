#pragma once

#include <platform/platform.h>
#include <platform/types.h>

#include <engine/array.h>
#include <engine/memory_arena.h>

struct FileAsset {
  char path[Max_Path_Length];
  TimeStamp last_modified;
};

typedef struct {
  i32 width;
  i32 height;
  u8* data;
} Bitmap;

enum AssetTypeId {};

enum AssetType { AssetType_Sound = 0, AssetType_Bitmap, AssetCount };

struct AssetSourceBitmap {
  char* file_name;
};

struct AssetSourceSound {
  char* file_name;
  u32 first_sample_index;
};

struct AssetSource {
  AssetType type;
  union {
    AssetSourceBitmap bitmap;
    AssetSourceSound sound;
  };
};

struct HuginBitmap {
  u32 dim[2];
  f32 align_percentage[2];
};

struct HuginSound {
  u32 sample_count;
  u32 channel_count;
  u32 chain;
};

struct HuginAssetType {
  u32 type_id;
  u32 first_asset_index;
  u32 one_past_last_asset_index;
};

struct HuginAsset {
  u64 data_offset;
  union {
    HuginBitmap bitmap;
  };
}

const u32 VERY_LARGE_NUMBER = 4096;

typedef struct {
  u32 num_asset_types;
  HuginAssetType asset_types[AssetCount];

  u32 num_assets;
  AssetSource asset_sources[VERY_LARGE_NUMBER];
  HuginAsset assets[VERY_LARGE_NUMBER];

} GameAssets;

static void begin_asset_type(GameAssets* assets, AssetTypeId type_id) {
  Assert(assets->De)
}

auto load_bitmap(const char* path, MemoryArena* arena) -> Bitmap*;

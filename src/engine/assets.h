#pragma once

#include <cstdio>
#include <platform/platform.h>
#include <platform/types.h>

#include <engine/array.h>
#include <engine/hugin_file_formats.h>
#include <engine/memory_arena.h>

const u32 BitmapBytePerPixel = 4;

struct LoadedBitmap {
    i32 width;
    i32 height;
    i32 pitch;

    vec2 align_percentage;
    // TODO: Make this a void pointer, to support multiple renderers
    u32 texture_handle;
    void* data;
};

enum AssetState {
    AssetState_Unloaded = 0,
    AssetState_Queued,
    AssetState_Loaded,
};

struct AssetMemoryHeader {
    AssetType asset_type;
    union {
        LoadedBitmap bitmap;
    };
};

struct Asset {
    AssetState state;
    AssetMemoryHeader* asset_memory;
};

struct GameAssets {
    HuginAssetType asset_types[Asset_Count];

    u32 asset_count;
    AssetMeta* assets_meta;
    Asset* assets;
};

auto get_bitmap_meta(GameAssets* game_assets, BitmapId id) -> HuginBitmap;
auto get_bitmap(GameAssets* game_assets, BitmapId id) -> LoadedBitmap*;
auto load_bitmap(GameAssets* game_assets, BitmapId id, MemoryArena* arena) -> void;
auto initialize_game_assets(const char* file_name, MemoryArena* arena) -> GameAssets*;

auto get_first_bitmap_from(GameAssets* game_assets, AssetTypeId asset_type_id) -> BitmapId;
auto get_first_bitmap_meta(GameAssets* game_assets, AssetTypeId asset_type_id) -> HuginBitmap;

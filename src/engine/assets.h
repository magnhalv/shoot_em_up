#pragma once

#include <platform/platform.h>
#include <platform/types.h>

#include <core/memory_arena.h>

#include <engine/array.h>
#include <engine/hugin_file_formats.h>

const u32 BitmapBytePerPixel = 4;
const u32 BytesPerAudioSample = 2;

struct LoadedBitmap {
    i32 width;
    i32 height;
    i32 pitch;

    vec2 align_percentage;

    void* data;
};

struct LoadedAudio {
    u32 sample_count;
    u32 channel_count;
    u32 chain;

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
        LoadedAudio audio;
    };
};

struct Asset {
    AssetState state;
    AssetMemoryHeader* asset_memory;
    u32 asset_file_index;
};

inline constexpr u32 ASSET_FILES_MAX_COUNT = 2;

struct GameAssets {
    bool is_initialized;
    AssetGroup asset_groups[AssetGroup_Count];

    u32 asset_count;
    AssetMeta* assets_meta;
    Asset* assets;

    u32 asset_files_count;
    PlatformFileHandle asset_files[ASSET_FILES_MAX_COUNT];

    MemoryArena* memory;
};

auto initialize_game_assets(MemoryArena* arena) -> GameAssets*;

auto get_bitmap_meta(GameAssets* game_assets, BitmapId id) -> HuginBitmap;
auto get_bitmap(GameAssets* game_assets, BitmapId id) -> LoadedBitmap*;
auto get_audio(GameAssets* game_assets, AudioId id) -> LoadedAudio*;

auto load_bitmap(GameAssets* game_assets, BitmapId id) -> void;
auto load_audio(GameAssets* game_assets, AudioId id) -> void;

auto get_first_bitmap_id(GameAssets* game_assets, AssetGroupId asset_group_id) -> BitmapId;
auto get_first_bitmap_meta(GameAssets* game_assets, AssetGroupId asset_group_id) -> HuginBitmap;
auto get_first_audio(GameAssets* game_assets, AssetGroupId asset_group_id) -> AudioId;

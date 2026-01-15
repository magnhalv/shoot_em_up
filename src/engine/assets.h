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
    i32 sample_count;
    i32 channel_count;
    i32 chain;

    void* data;
};

struct LoadedFont {
    i32 code_point_first;
    i32 code_point_last;
    i32 code_point_count;
    CodePoint* code_points;

    f32 ascent;
    f32 descent;
    f32 font_height;

    i32 bitmap_width;
    i32 bitmap_height;
    i32 bitmap_size_per_pixel;
    void* bitmap;
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
        LoadedFont font;
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
    AssetGroup asset_groups[AssetGroupId_Count];

    u32 asset_count;
    AssetMeta* assets_meta;
    Asset* assets;

    u32 asset_tag_count;
    AssetTag* asset_tags;

    u32 asset_files_count;
    PlatformFileHandle asset_files[ASSET_FILES_MAX_COUNT];

    MemoryArena* memory;
};

auto initialize_game_assets(MemoryArena* arena) -> GameAssets*;

auto get_bitmap_meta(GameAssets* game_assets, BitmapId id) -> BitmapMeta;
auto get_bitmap(GameAssets* game_assets, BitmapId id) -> LoadedBitmap*;
auto get_audio(GameAssets* game_assets, AudioId id) -> LoadedAudio*;
auto get_font(GameAssets* game_assets, FontId id) -> LoadedFont*;

auto load_bitmap(GameAssets* game_assets, BitmapId id) -> void;
auto load_audio(GameAssets* game_assets, AudioId id) -> void;
auto load_font(GameAssets* game_assets, FontId id) -> void;

auto get_first_bitmap_id(GameAssets* game_assets, AssetGroupId asset_group_id) -> BitmapId;
auto get_closest_bitmap_id(GameAssets* game_assets, AssetGroupId asset_group_id, AssetTagId tag_id, f32 value) -> BitmapId;
auto get_first_bitmap_meta(GameAssets* game_assets, AssetGroupId asset_group_id) -> BitmapMeta;
auto get_first_audio(GameAssets* game_assets, AssetGroupId asset_group_id) -> AudioId;
auto get_first_font_id(GameAssets* game_assets, AssetGroupId asset_group_id) -> FontId;

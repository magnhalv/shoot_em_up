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

// TODO: Read AllocateGameAssets in handmade hero.

static auto initialize_game_assets(const char* file_name, MemoryArena* arena) -> GameAssets* {
    FILE* file = fopen(file_name, "rb");
    GameAssets* game_assets = allocate<GameAssets>(arena);

    if (!file) {
        printf("ERROR: Could not open file '%s'\n", file_name);
    }
    else {
        HafHeader header = { 0 };
        fread(&header, sizeof(HafHeader), 1, file);

        if (header.magic_value != HAF_MAGIC_VALUE) {
            InvalidCodePath;
        }

        if (header.version != HAF_VERSION) {
            InvalidCodePath;
        }
        u32 asset_count = header.asset_count;
        u32 asset_type_count = header.asset_type_count;

        fseek(file, header.asset_types, SEEK_SET);
        fread(game_assets->asset_types, asset_type_count * sizeof(HuginAssetType), 1, file);
        Assert(game_assets->asset_types[1].type_id == Asset_PlayerSpaceShip);

        game_assets->asset_count = asset_count;
        game_assets->assets_meta = allocate<AssetMeta>(arena, asset_count);
        game_assets->assets = allocate<Asset>(arena, asset_count);
        fseek(file, header.assets, SEEK_SET);
        fread(game_assets->assets_meta, asset_count * sizeof(AssetMeta), 1, file);
    }

    fclose(file);

    return game_assets;
}

static auto get_first_bitmap_from(GameAssets* game_assets, AssetTypeId asset_type_id) -> BitmapId {
    u32 result = 0;

    HuginAssetType* type = game_assets->asset_types + asset_type_id;
    if (type->first_asset_index != type->one_past_last_asset_index) {
        result = type->first_asset_index;
    }

    return BitmapId{ result };
}

static auto get_first_bitmap_meta(GameAssets* game_assets, AssetTypeId asset_type_id) -> HuginBitmap {
    u32 id = 0;

    HuginAssetType* type = game_assets->asset_types + asset_type_id;
    if (type->first_asset_index != type->one_past_last_asset_index) {
        id = type->first_asset_index;
    }

    auto* meta = game_assets->assets_meta + id;
    return meta->bitmap;
}

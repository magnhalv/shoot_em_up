#pragma once

#include "platform/user_input.h"
#include <cstdio>
#include <cstring>
#include <platform/platform.h>
#include <platform/types.h>

#include <engine/array.h>
#include <engine/memory_arena.h>

typedef struct {
    i32 width;
    i32 height;
    u8* data;
} Bitmap;

struct LoadedBitmap {
    i32 width;
    i32 height;
    i32 pitch;
    void* data;
    void* free; // used for alignment
};

auto load_bitmap(const char* path, MemoryArena* arena) -> Bitmap*;
auto load_bitmap(const char* path) -> LoadedBitmap;

static_assert(true); // due to pragma clang bug
#pragma pack(push, 1)
struct BitmapId {
    u32 value;
};

struct SoundId {
    u32 value;
};
#pragma pack(pop)

struct FileAsset {
    char path[Max_Path_Length];
    TimeStamp last_modified;
};

enum AssetTypeId : u32 {
    Asset_None = 0,
    Asset_PlayerSpaceShip,

    Asset_Count
};

enum AssetType { AssetType_Sound = 0, AssetType_Bitmap };

struct AssetSourceBitmap {
    const char* file_name;
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
        HuginSound sound;
    };
};

const u32 MAX_ASSETS_COUNT = 4096;

struct GameAssetsWrite {
    u32 num_asset_types;
    HuginAssetType asset_types[Asset_Count];

    u32 asset_count;
    AssetSource asset_sources[MAX_ASSETS_COUNT];
    HuginAsset assets[MAX_ASSETS_COUNT];

    HuginAssetType* current_asset_type;
    u32 asset_index;
};

enum AssetState {
    AssetState_Unloaded = 0,
    AssetState_Queued,
    AssetState_Loaded,
};

struct GameAssetsRead {
    HuginAssetType asset_types[Asset_Count];

    u32 asset_count;
    HuginAsset* assets;
    AssetState* asset_states;
};

#define HAF_CODE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))

struct HafHeader {
#define HAF_MAGIC_VALUE HAF_CODE('h', 'a', 'f', 'c')
    u32 magic_value;
#define HAF_VERSION 0
    u32 version;

    u32 asset_type_count;
    u32 asset_count;

    u64 asset_types;
    u64 assets;
};

static void begin_asset_type(GameAssetsWrite* assets, AssetTypeId type_id) {
    Assert(assets->current_asset_type == NULL);

    assets->current_asset_type = assets->asset_types + type_id;
    assets->current_asset_type->type_id = type_id;
    assets->current_asset_type->first_asset_index = assets->asset_count;
    assets->current_asset_type->one_past_last_asset_index = assets->current_asset_type->first_asset_index;
}

struct AddedAsset {
    u32 id;
    HuginAsset* hugin_asset;
    AssetSource* source;
};

static auto add_asset(GameAssetsWrite* assets) -> AddedAsset {
    Assert(assets->current_asset_type);
    Assert(assets->current_asset_type->one_past_last_asset_index < ArrayCount(assets->assets));

    u32 index = assets->current_asset_type->one_past_last_asset_index++;

    AssetSource* source = assets->asset_sources + index;
    HuginAsset* ha = assets->assets + index;

    assets->asset_index = index;
    AddedAsset result;
    result.id = index;
    result.hugin_asset = assets->assets + index;
    result.source = source;
    return result;
}

static auto add_bitmap_asset(
    GameAssetsWrite* assets, const char* file_name, f32 align_percentage_x = 0.5f, f32 align_percentage_y = 0.5f) {
    AddedAsset asset = add_asset(assets);
    asset.hugin_asset->bitmap.align_percentage[0] = align_percentage_x;
    asset.hugin_asset->bitmap.align_percentage[1] = align_percentage_y;
    asset.source->type = AssetType_Bitmap;
    asset.source->bitmap.file_name = file_name;

    return BitmapId{ asset.id };
}

static auto end_asset_type(GameAssetsWrite* assets) -> void {
    Assert(assets->current_asset_type);
    assets->asset_count = assets->current_asset_type->one_past_last_asset_index;
    assets->current_asset_type = 0;
    assets->asset_index = 0;
}

static auto initialize(GameAssetsWrite* assets) -> void {
    assets->asset_count = 1;
    assets->current_asset_type = 0;
    assets->asset_index = 0;

    assets->num_asset_types = Asset_Count;
    memset(assets->asset_types, 0, sizeof(assets->asset_types));
}

// TODO: Read AllocateGameAssets in handmade hero.

static auto read_asset_file(const char* file_name, MemoryArena* arena) -> GameAssetsRead* {
    FILE* file = fopen(file_name, "rb");
    GameAssetsRead* game_assets = allocate<GameAssetsRead>(arena);

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
        game_assets->assets = allocate<HuginAsset>(arena, asset_count);
        game_assets->asset_states = allocate<AssetState>(arena, asset_count);
        fseek(file, header.assets, SEEK_SET);
        fread(game_assets->assets, asset_count * sizeof(HuginAsset), 1, file);

        {
          auto space_ship = game_assets->asset_types[Asset_PlayerSpaceShip];

          auto spaceship_asset = game_assets->assets[space_ship.first_asset_index];

          printf("0: %d", spaceship_asset.bitmap.dim[0]);
          printf("1: %d", spaceship_asset.bitmap.dim[1]);
        }
    }

    return game_assets;
}

static auto write_asset_file(GameAssetsWrite* assets, const char* file_name) -> void {

    FILE* out = fopen(file_name, "wb");

    if (!out) {
        printf("ERROR: Could not open file '%s'\n", file_name);
    }
    else {
        HafHeader header = {};
        header.magic_value = HAF_MAGIC_VALUE;
        header.version = HAF_VERSION;
        header.asset_type_count = Asset_Count;
        header.asset_count = assets->asset_count;

        u32 asset_type_array_size = header.asset_type_count * sizeof(HuginAssetType);
        u32 asset_array_size = header.asset_count * sizeof(HuginAsset);

        header.asset_types = sizeof(HafHeader);
        header.assets = header.asset_types + asset_type_array_size;


        fwrite(&header, sizeof(header), 1, out);
        fwrite(&assets->asset_types, asset_type_array_size, 1, out);
        fseek(out, asset_array_size, SEEK_CUR);
        for (u32 asset_idx = 1; asset_idx < header.asset_count; asset_idx++) {
            AssetSource* source = assets->asset_sources + asset_idx;
            HuginAsset* dest = assets->assets + asset_idx;

            dest->data_offset = ftell(out);

            if (source->type == AssetType_Bitmap) {
                LoadedBitmap bitmap = load_bitmap(source->bitmap.file_name);
                dest->bitmap.dim[0] = bitmap.width;
                dest->bitmap.dim[1] = bitmap.height;

                Assert((bitmap.width * 4) == bitmap.pitch);
                fwrite(bitmap.data, bitmap.pitch * bitmap.height, 1, out);

                free(bitmap.free);
            }
        }

        fseek(out, (u32)header.assets, SEEK_SET);
        fwrite(assets->assets, asset_array_size, 1, out);
    }
}

static auto write_spaceships() -> void {
    GameAssetsWrite assets_;
    GameAssetsWrite* assets = &assets_;

    initialize(assets);

    begin_asset_type(assets, Asset_PlayerSpaceShip);
    add_bitmap_asset(assets, "assets/sprites/player_1.png", 0.5, 0.5);
    end_asset_type(assets);

    write_asset_file(assets, "assets.haf");
    // TODO: Write assets file
}

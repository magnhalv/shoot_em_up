#pragma once

#include "platform/user_input.h"
#include <cstdio>
#include <cstring>
#include <platform/platform.h>
#include <platform/types.h>

#include <engine/array.h>
#include <engine/memory_arena.h>

const u32 BitmapBytePerPixel = 4;

typedef struct {
    i32 width;
    i32 height;
    u8* data;
} Bitmap;

struct LoadedBitmap {
    i32 width;
    i32 height;
    i32 pitch;

    vec2 align_percentage;
    // TODO: Make this a void pointer, to support multiple renderers
    u32 texture_handle;
    void* data;
};

auto load_bitmap(const char* path, MemoryArena* arena) -> LoadedBitmap*;
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
    Asset_EnemySpaceShip,
    Asset_Projectile,

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
    vec2 align_percentage;
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

struct AssetMeta {
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
    AssetMeta assets[MAX_ASSETS_COUNT];

    HuginAssetType* current_asset_type;
    u32 asset_index;
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

struct GameAssetsRead {
    HuginAssetType asset_types[Asset_Count];

    u32 asset_count;
    AssetMeta* assets_meta;
    Asset* assets;
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
    AssetMeta* hugin_asset;
    AssetSource* source;
};

static auto add_asset(GameAssetsWrite* assets) -> AddedAsset {
    Assert(assets->current_asset_type);
    Assert(assets->current_asset_type->one_past_last_asset_index < ArrayCount(assets->assets));

    u32 index = assets->current_asset_type->one_past_last_asset_index++;

    AssetSource* source = assets->asset_sources + index;
    AssetMeta* ha = assets->assets + index;

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
    asset.hugin_asset->bitmap.align_percentage.x = align_percentage_x;
    asset.hugin_asset->bitmap.align_percentage.y = align_percentage_y;
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

static auto initialize_game_assets(const char* file_name, MemoryArena* arena) -> GameAssetsRead* {
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
        game_assets->assets_meta = allocate<AssetMeta>(arena, asset_count);
        game_assets->assets = allocate<Asset>(arena, asset_count);
        fseek(file, header.assets, SEEK_SET);
        fread(game_assets->assets_meta, asset_count * sizeof(AssetMeta), 1, file);
    }

    fclose(file);

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
        u32 asset_array_size = header.asset_count * sizeof(AssetMeta);

        header.asset_types = sizeof(HafHeader);
        header.assets = header.asset_types + asset_type_array_size;

        fwrite(&header, sizeof(header), 1, out);
        fwrite(&assets->asset_types, asset_type_array_size, 1, out);
        fseek(out, asset_array_size, SEEK_CUR);
        for (u32 asset_idx = 1; asset_idx < header.asset_count; asset_idx++) {
            AssetSource* source = assets->asset_sources + asset_idx;
            AssetMeta* dest = assets->assets + asset_idx;

            dest->data_offset = ftell(out);

            if (source->type == AssetType_Bitmap) {
                // TODO: Do not use loaded bitmap here
                LoadedBitmap bitmap = load_bitmap(source->bitmap.file_name);
                dest->bitmap.dim[0] = bitmap.width;
                dest->bitmap.dim[1] = bitmap.height;

                Assert((bitmap.width * 4) == bitmap.pitch);
                fwrite(bitmap.data, bitmap.pitch * bitmap.height, 1, out);

                free(bitmap.data);
            }
        }

        fseek(out, (u32)header.assets, SEEK_SET);
        fwrite(assets->assets, asset_array_size, 1, out);
    }
    fclose(out);
}

static auto get_first_bitmap_from(GameAssetsRead* game_assets, AssetTypeId asset_type_id) -> BitmapId {
    u32 result = 0;

    HuginAssetType* type = game_assets->asset_types + asset_type_id;
    if (type->first_asset_index != type->one_past_last_asset_index) {
        result = type->first_asset_index;
    }

    return BitmapId{ result };
}

// TODO: Not BitmapId, AssetTypeId
static auto load_bitmap2(GameAssetsRead* game_assets, BitmapId id, MemoryArena* arena, const char* file_name) -> LoadedBitmap* {
    Assert(id.value < game_assets->asset_count);
    AssetMeta* meta = game_assets->assets_meta + id.value;
    Asset* asset = game_assets->assets + id.value;

    if (asset->state == AssetState_Unloaded) {

        FILE* file = fopen(file_name, "rb");

        Assert(asset->asset_memory == NULL);
        asset->asset_memory = allocate<AssetMemoryHeader>(arena);
        fseek(file, meta->data_offset, SEEK_SET);

        asset->asset_memory->asset_type = AssetType_Bitmap;
        auto size = meta->bitmap.dim[0] * meta->bitmap.dim[1] * BitmapBytePerPixel;
        asset->asset_memory->bitmap.data = allocate<u8>(arena, size);

        asset->asset_memory->bitmap.pitch = meta->bitmap.dim[0];
        asset->asset_memory->bitmap.width = meta->bitmap.dim[0];
        asset->asset_memory->bitmap.height = meta->bitmap.dim[1];
        asset->asset_memory->bitmap.align_percentage = meta->bitmap.align_percentage;
        asset->asset_memory->bitmap.texture_handle = 0;
        fread(asset->asset_memory->bitmap.data, size, 1, file);

        fclose(file);
    
        asset->state = AssetState_Loaded;
    }

    return &asset->asset_memory->bitmap;
}

static auto write_spaceships() -> void {
    GameAssetsWrite assets_ = { 0 };
    GameAssetsWrite* assets = &assets_;

    initialize(assets);

    begin_asset_type(assets, Asset_PlayerSpaceShip);
    add_bitmap_asset(assets, "assets/sprites/player_1.png");
    end_asset_type(assets);

    begin_asset_type(assets, Asset_EnemySpaceShip);
    add_bitmap_asset(assets, "assets/sprites/blue_01.png");
    end_asset_type(assets);

    begin_asset_type(assets, Asset_Projectile);
    add_bitmap_asset(assets, "assets/sprites/projectile_1.png");
    end_asset_type(assets);

    write_asset_file(assets, "assets.haf");
    // TODO: Write assets file
}

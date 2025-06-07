#pragma once

#include <platform/types.h>

#include <math/vec2.h>

static_assert(true); // due to pragma clang bug
#pragma pack(push, 1)
struct BitmapId {
    u32 value;
};

struct AudioId {
    u32 value;
};

struct SoundId {
    u32 value;
};
#pragma pack(pop)

enum AssetTypeId : u32 {
    Asset_None = 0,

    // Bitmaps
    Asset_PlayerSpaceShip,
    Asset_EnemySpaceShip,
    Asset_Projectile,

    // Audio
    Asset_Laser,
    Asset_Explosion,

    Asset_Count
};

enum AssetType { AssetType_Invalid = 0, AssetType_Audio, AssetType_Bitmap };

struct HuginAssetType {
    u32 type_id;
    u32 first_asset_index;
    u32 one_past_last_asset_index;
};

struct AssetSourceBitmap {
    const char* file_name;
};

struct AssetSourceSound {
    const char* file_name;
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

struct HuginAudio {
    u32 sample_count;
    u32 channel_count;
    u32 chain;
};

struct AssetMeta {
    u64 data_offset;
    union {
        HuginBitmap bitmap;
        HuginAudio audio;
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

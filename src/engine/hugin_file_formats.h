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

enum AssetGroupId : u32 {
    Asset_None = 0,

    // Bitmaps
    Asset_PlayerSpaceShip,
    Asset_EnemySpaceShip,
    Asset_Projectile,
    Asset_Test,

    // Audio
    Asset_Laser,
    Asset_Explosion,

    AssetGroup_Count
};

enum AssetType { AssetType_Invalid = 0, AssetType_Audio, AssetType_Bitmap };

struct AssetGroup {
    u32 type_id;
    u32 first_asset_index;
    u32 one_past_last_asset_index;
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

// TODO: Do we need this? Perhaps merge it with asset
struct AssetMeta {
    u64 data_offset;
    union {
        HuginBitmap bitmap;
        HuginAudio audio;
    };
};

#define HAF_CODE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))

struct HafHeader {
#define HAF_MAGIC_VALUE HAF_CODE('h', 'a', 'f', 'c')
    u32 magic_value;
#define HAF_VERSION 0
    u32 version;

    u32 asset_group_count;
    u32 asset_count;

    u64 asset_groups_block;
    u64 assets_block;
};

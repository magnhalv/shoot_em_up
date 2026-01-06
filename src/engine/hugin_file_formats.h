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

struct FontId {
    u32 value;
};
#pragma pack(pop)

enum AssetGroupId : u32 {
    AssetGroupId_None = 0,

    // Bitmaps
    AssetGroupId_PlayerSpaceShip,
    AssetGroupId_EnemySpaceShip,
    AssetGroupId_Projectile,
    AssetGroupId_Explosion,
    AssetGroupId_Test,

    // Audio
    AssetGroupId_Audio_Laser,
    AssetGroupId_Audio_Explosion,

    // Fonts
    AssetGroupId_Fonts_Ubuntu,

    AssetGroupId_Count
};

enum AssetType {
    AssetType_Invalid = 0, //
    AssetType_Audio,
    AssetType_Bitmap,
    AssetType_Font,
    AssetType_Count
};

enum AssetTagId {
    AssetTag_SpaceShipDirection = 0, //
    AssetTag_ExplosionProgress = 1,  //
    AssetTag_Count                   //
};

struct AssetTag {
    AssetTagId id;
    f32 value;
    u32 asset_index;
};

struct AssetGroup {
    u32 group_id;
    u32 first_asset_index;
    u32 one_past_last_asset_index;

    u32 first_asset_tag_index;
    u32 one_past_last_asset_tag_index;
};

struct CodePoint {
    u16 x0, y0, x1, y1;
    f32 xoff, yoff, xadvance;
};

struct FontMeta {
    i32 code_point_count;
    i32 code_point_first;
    i32 code_point_last;

    f32 font_height;

    i32 bitmap_width;
    i32 bitmap_height;
    i32 bitmap_size_per_pixel;
};

struct BitmapMeta {
    u32 dim[2];
    vec2 align_percentage;
};

struct AudioMeta {
    u32 sample_count;
    u32 channel_count;
    u32 chain;
};

struct AssetMeta {
    u64 data_offset;
    union {
        BitmapMeta bitmap;
        AudioMeta audio;
        FontMeta font;
    };
};

#define HAF_CODE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))

struct HafHeader {
#define HAF_MAGIC_VALUE HAF_CODE('h', 'a', 'f', 'c')
    u32 magic_value;
#define HAF_VERSION 1
    u32 version;

    u32 asset_group_count;
    u32 asset_count;
    u32 asset_tag_count;

    u64 asset_groups_block;
    u64 assets_tag_block;
    u64 assets_meta_block; // Must be last!
    u64 assets_block;      // Must be last!
};

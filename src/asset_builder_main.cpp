#include <cstdio>
#include <libs/stb/stb_image.h>

#include <platform/platform.h>
#include <platform/types.h>

#include <engine/hugin_file_formats.h>

#include <math/vec2.h>

struct Bitmap {
    i32 width;
    i32 height;
    i32 pitch;

    void* data;
};

auto load_bitmap_stbi(const char* path) -> Bitmap {
    Bitmap result;
    i32 num_channels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char* stbi_data = stbi_load(path, &result.width, &result.height, &num_channels, 4);

    if (num_channels != 4) {
        InvalidCodePath;
    }

    if (stbi_data) {
        result.pitch = result.width * num_channels;
        auto size = result.width * result.height * num_channels; // 1 byte per channel

        result.data = malloc(size);
        memcpy(result.data, stbi_data, size);

        stbi_image_free(stbi_data);
    }
    else {
        InvalidCodePath;
    }

    return result;
}

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
                Bitmap bitmap = load_bitmap_stbi(source->bitmap.file_name);
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

/////// Write assets ////////////////
static auto write_spaceships() -> void {
    GameAssetsWrite assets_ = { 0 };
    GameAssetsWrite* assets = &assets_;

    initialize(assets);

    begin_asset_type(assets, Asset_PlayerSpaceShip);
    add_bitmap_asset(assets, "assets/bitmaps/player_1.png");
    end_asset_type(assets);

    begin_asset_type(assets, Asset_EnemySpaceShip);
    add_bitmap_asset(assets, "assets/bitmaps/blue_01.png");
    end_asset_type(assets);

    begin_asset_type(assets, Asset_Projectile);
    add_bitmap_asset(assets, "assets/bitmaps/projectile_1.png");
    end_asset_type(assets);

    write_asset_file(assets, "assets.haf");
    // TODO: Write assets file
}
int main() {
    write_spaceships();
}

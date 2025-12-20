#include "core/memory.h"
#include "engine/hm_assert.h"
#include <cstdio>
#include <libs/stb/stb_image.h>

#include <platform/platform.h>
#include <platform/types.h>

#include <engine/assets.h>
#include <engine/hugin_file_formats.h>

#include <math/vec2.h>

// Temp structures used for writing
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

const u32 MAX_ASSETS_COUNT = 4096;
const u32 MAX_TAGS_COUNT = 4096;
struct GameAssetsWrite {
    u32 asset_group_count;
    AssetGroup asset_groups[AssetGroupId_Count];

    u32 asset_count;
    AssetSource asset_sources[MAX_ASSETS_COUNT];
    AssetMeta assets_meta[MAX_ASSETS_COUNT];

    u32 asset_tag_count;
    AssetTag asset_tags[MAX_TAGS_COUNT];

    AssetGroup* current_asset_group;
    u32 curr_asset_index;
};

// Start external file format
struct WavRiffChunk {
    char identifier[4];
    u32 file_size;
    char file_format_id[4];
};

struct WavSubchunkDesc {
    char chunk_id[4];
    u32 chunk_size;
};

struct WavFormatChunk {
    char format_block_id[4];
    u32 block_size;
    u16 audio_format;
    u16 channel_count;
    u32 frequency;
    u32 bytes_per_sec;
    u16 sample_byte_count;
    u16 bits_per_sample;

    auto print() -> void {
    }
};

struct WavDataChunk {
    char data_block_id[4];
    u32 data_size;
};

struct WavFile {
    WavRiffChunk riff_chunk;
    WavFormatChunk fmt_chunk;
    WavDataChunk data_chunk;
    u8* data;
};

auto inline print(WavFile* file) -> void {
    printf("Header: %.4s\n", file->riff_chunk.identifier);
    printf("File Size: %u\n", file->riff_chunk.file_size);
    printf("File format id: %.4s\n", file->riff_chunk.file_format_id);
    printf("--------------\n");
    printf("Format block id: %.4s\n", file->fmt_chunk.format_block_id);
    printf("Block size: %u\n", file->fmt_chunk.block_size);
    printf("Audio format: %u\n", file->fmt_chunk.audio_format);
    printf("Num channels: %u\n", file->fmt_chunk.channel_count);
    printf("Frequency: %u\n", file->fmt_chunk.frequency);
    printf("Bytes per sec: %u\n", file->fmt_chunk.bytes_per_sec);
    printf("Bytes per block: %u\n", file->fmt_chunk.sample_byte_count);
    printf("Bits per sample: %u\n", file->fmt_chunk.bits_per_sample);
    printf("Data block id: %.4s\n", file->data_chunk.data_block_id);
    printf("Data size: %u\n", file->data_chunk.data_size);
}
// end

struct WavResult {
    i32 frequency;
    i32 channel_count;
    i32 bits_per_sample;

    void* data;
};

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

    if (!stbi_data) {
        printf("Invalid bitmap path: %s\n", path);
        InvalidCodePath;
    }

    if (num_channels != 4) {
        InvalidCodePath;
    }

    result.pitch = result.width * num_channels;
    auto size = result.width * result.height * num_channels; // 1 byte per channel

    result.data = malloc(size);
    memcpy(result.data, stbi_data, size);

    stbi_image_free(stbi_data);

    return result;
}

auto load_wav_file(const char* path) -> WavFile {
    FILE* file = fopen(path, "rb");

    if (!file) {
        printf("Unable to open wav file: %s\n", path);
        InvalidCodePath;
    }

    WavFile wav_file = {};
    fread(&wav_file.riff_chunk, sizeof(WavRiffChunk), 1, file);

    while (!feof(file)) {
        // TODO: memcopy the data into a WavFile struct, without pointers, so we can release the extra memory.
        WavSubchunkDesc desc = { 0 };
        u64 curr_pos = ftell(file);
        fread(&desc, sizeof(WavSubchunkDesc), 1, file);
        fseek(file, curr_pos, SEEK_SET);

        if (std::memcmp("fmt", desc.chunk_id, 3) == 0) {
            fread(&wav_file.fmt_chunk, sizeof(WavFormatChunk), 1, file);
        }
        else if (std::memcmp("data", desc.chunk_id, 4) == 0 && wav_file.data == nullptr) {
            fread(&wav_file.data_chunk, sizeof(WavDataChunk), 1, file);
            wav_file.data = (u8*)malloc(wav_file.data_chunk.data_size);
            fread(wav_file.data, 1, wav_file.data_chunk.data_size, file);
        }
        else {
            fseek(file, desc.chunk_size, SEEK_CUR);
            break;
        }
    }

    Assert(wav_file.fmt_chunk.bits_per_sample / 8 == BytesPerAudioSample);
    return wav_file;
}

static void begin_asset_group(GameAssetsWrite* assets, AssetGroupId group_id) {
    Assert(assets->current_asset_group == NULL);

    assets->current_asset_group = assets->asset_groups + group_id;
    assets->current_asset_group->group_id = group_id;

    assets->current_asset_group->first_asset_index = assets->asset_count;
    assets->current_asset_group->one_past_last_asset_index = assets->asset_count;

    assets->current_asset_group->first_asset_tag_index = assets->asset_tag_count;
    assets->current_asset_group->one_past_last_asset_tag_index = assets->asset_tag_count;
}

struct AddedAsset {
    u32 id;
    AssetMeta* hugin_asset;
    AssetSource* source;
};

static auto add_asset(GameAssetsWrite* assets) -> AddedAsset {
    Assert(assets->current_asset_group);
    Assert(assets->current_asset_group->one_past_last_asset_index < ArrayCount(assets->assets_meta));

    u32 index = assets->current_asset_group->one_past_last_asset_index++;

    AssetSource* source = assets->asset_sources + index;
    AssetMeta* ha = assets->assets_meta + index;

    assets->curr_asset_index = index;
    AddedAsset result;
    result.id = index;
    result.hugin_asset = assets->assets_meta + index;
    result.source = source;
    return result;
}

static auto add_bitmap_asset(GameAssetsWrite* assets, const char* file_name, f32 align_percentage_x = 0.5f,
    f32 align_percentage_y = 0.5f) -> BitmapId {
    AddedAsset asset = add_asset(assets);
    asset.hugin_asset->bitmap.align_percentage.x = align_percentage_x;
    asset.hugin_asset->bitmap.align_percentage.y = align_percentage_y;
    asset.source->type = AssetType_Bitmap;
    asset.source->bitmap.file_name = file_name;

    return BitmapId{ asset.id };
}

static auto add_audio_asset(GameAssetsWrite* assets, const char* file_name) -> AudioId {
    AddedAsset asset = add_asset(assets);
    asset.source->type = AssetType_Audio;
    asset.source->sound.file_name = file_name;
    asset.source->sound.first_sample_index = 0;

    return AudioId{ asset.id };
}

static auto add_tag(GameAssetsWrite* assets, AssetTagId tag_id, f32 value) -> void {
    u32 tag_index = assets->current_asset_group->one_past_last_asset_tag_index++;
    Assert(tag_index < MAX_TAGS_COUNT);
    AssetTag* tag = &assets->asset_tags[tag_index];
    tag->id = tag_id;
    tag->value = value;
    tag->asset_index = assets->curr_asset_index;
}

static auto end_asset_group(GameAssetsWrite* assets) -> void {
    Assert(assets->current_asset_group);
    assets->asset_count = assets->current_asset_group->one_past_last_asset_index;
    assets->asset_tag_count = assets->current_asset_group->one_past_last_asset_tag_index;
    assets->current_asset_group = 0;
    assets->curr_asset_index = 0;
}

static auto initialize(GameAssetsWrite* assets) -> void {
    assets->asset_count = 1;
    assets->current_asset_group = 0;
    assets->curr_asset_index = 0;

    assets->asset_group_count = AssetGroupId_Count;
    memset(assets->asset_groups, 0, sizeof(assets->asset_groups));
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
        header.asset_group_count = assets->asset_group_count;
        header.asset_count = assets->asset_count;
        header.asset_tag_count = assets->asset_tag_count;

        u32 asset_group_array_size = header.asset_group_count * sizeof(AssetGroup);
        u32 asset_array_meta_size = header.asset_count * sizeof(AssetMeta);
        u32 asset_tag_array_size = header.asset_tag_count * sizeof(AssetTag);

        header.asset_groups_block = sizeof(HafHeader);
        header.assets_meta_block = header.asset_groups_block + asset_group_array_size;
        header.assets_tag_block = header.assets_meta_block + asset_array_meta_size;
        header.assets_block = header.assets_tag_block + asset_tag_array_size;

        fwrite(&header, sizeof(header), 1, out);
        fwrite(&assets->asset_groups, asset_group_array_size, 1, out);
        HM_ASSERT(header.assets_tag_block > header.assets_meta_block);
        fseek(out, (u32)header.assets_tag_block, SEEK_SET);
        fwrite(assets->asset_tags, asset_tag_array_size, 1, out);

        fseek(out, header.assets_block, SEEK_SET);
        for (u32 asset_idx = 1; asset_idx < header.asset_count; asset_idx++) {
            AssetSource* source = assets->asset_sources + asset_idx;
            AssetMeta* dest = assets->assets_meta + asset_idx;

            dest->data_offset = ftell(out);

            if (source->type == AssetType_Bitmap) {
                Bitmap bitmap = load_bitmap_stbi(source->bitmap.file_name);
                dest->bitmap.dim[0] = bitmap.width;
                dest->bitmap.dim[1] = bitmap.height;

                Assert((bitmap.width * 4) == bitmap.pitch);
                fwrite(bitmap.data, bitmap.pitch * bitmap.height, 1, out);

                free(bitmap.data);
            }
            else if (source->type == AssetType_Audio) {
                WavFile wav_file = load_wav_file(source->sound.file_name);
                auto sample_count = wav_file.data_chunk.data_size / (wav_file.fmt_chunk.sample_byte_count);
                dest->audio.sample_count = sample_count;
                dest->audio.channel_count = wav_file.fmt_chunk.channel_count;
                dest->audio.chain = 0;

                fwrite(wav_file.data, sizeof(u8) * wav_file.data_chunk.data_size, 1, out);

                free(wav_file.data);
            }
        }

        // Meta has to be written last since we update it to where
        // the asset is stored in the file in the loop above.
        fseek(out, (u32)header.assets_meta_block, SEEK_SET);
        fwrite(assets->assets_meta, asset_array_meta_size, 1, out);
    }
    fclose(out);
}

/////// Write assets ////////////////
static auto write_bitmaps() -> void {
    GameAssetsWrite assets_ = { 0 };
    GameAssetsWrite* assets = &assets_;

    initialize(assets);

    begin_asset_group(assets, AssetGroupId_PlayerSpaceShip);
    add_bitmap_asset(assets, "assets/bitmaps/player_1.png");
    add_tag(assets, AssetTag_SpaceShipDirection, 0.0);
    add_bitmap_asset(assets, "assets/bitmaps/player_left-1.png");
    add_tag(assets, AssetTag_SpaceShipDirection, -1.0);
    add_bitmap_asset(assets, "assets/bitmaps/player_right-1.png");
    add_tag(assets, AssetTag_SpaceShipDirection, 1.0);
    end_asset_group(assets);

    begin_asset_group(assets, AssetGroupId_EnemySpaceShip);
    add_bitmap_asset(assets, "assets/bitmaps/blue_01.png");
    end_asset_group(assets);

    begin_asset_group(assets, AssetGroupId_Projectile);
    add_bitmap_asset(assets, "assets/bitmaps/projectile_1.png");
    end_asset_group(assets);

    begin_asset_group(assets, AssetGroupId_Test);
    add_bitmap_asset(assets, "assets/bitmaps/test.png");
    end_asset_group(assets);

    begin_asset_group(assets, AssetGroupId_Explosion);
    add_bitmap_asset(assets, "assets/bitmaps/explosion-1.png");
    add_tag(assets, AssetTag_ExplosionProgress, 0.0);
    add_bitmap_asset(assets, "assets/bitmaps/explosion-2.png");
    add_tag(assets, AssetTag_ExplosionProgress, 1.0 / 7.0);
    add_bitmap_asset(assets, "assets/bitmaps/explosion-3.png");
    add_tag(assets, AssetTag_ExplosionProgress, 2.0 / 7.0);
    add_bitmap_asset(assets, "assets/bitmaps/explosion-4.png");
    add_tag(assets, AssetTag_ExplosionProgress, 3.0 / 7.0);
    add_bitmap_asset(assets, "assets/bitmaps/explosion-5.png");
    add_tag(assets, AssetTag_ExplosionProgress, 4.0 / 7.0);
    add_bitmap_asset(assets, "assets/bitmaps/explosion-6.png");
    add_tag(assets, AssetTag_ExplosionProgress, 5.0 / 7.0);
    add_bitmap_asset(assets, "assets/bitmaps/explosion-7.png");
    add_tag(assets, AssetTag_ExplosionProgress, 6.0 / 7.0);
    add_bitmap_asset(assets, "assets/bitmaps/explosion-8.png");
    add_tag(assets, AssetTag_ExplosionProgress, 7.0 / 7.0);
    end_asset_group(assets);

    write_asset_file(assets, "bitmaps.haf");
}

static auto write_audio() -> void {
    GameAssetsWrite assets_ = { 0 };
    GameAssetsWrite* assets = &assets_;

    initialize(assets);

    begin_asset_group(assets, AssetGroupId_Audio_Laser);
    add_audio_asset(assets, "assets/audio/laser_primary.wav");
    end_asset_group(assets);

    begin_asset_group(assets, AssetGroupId_Audio_Explosion);
    add_audio_asset(assets, "assets/audio/explosion.wav");
    end_asset_group(assets);

    write_asset_file(assets, "audio.haf");
}

int main() {
    // Write to separate files, in order to test supporting multiple asset files.
    write_bitmaps();
    write_audio();

    printf("Built assets successfully!\n");
}

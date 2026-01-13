#include <platform/types.h>

#include <cstdio>

#include <core/memory.h>
#include <core/util.hpp>
#include <engine/hm_assert.h>

#include <engine/assets.h>
#include <engine/hugin_file_formats.h>

#include <math/vec2.h>

#include <core/lib.cpp>
#include <third-party/stb_image.cpp>
#include <third-party/stb_image_write.cpp>
#include <third-party/stb_truetype.cpp>

auto read_whole_file(const char* path) -> u8* {
    FILE* file = fopen(path, "rb");
    if (!file) {
        printf("Unable to open file: %s\n", path);
        InvalidCodePath;
    }
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    u8* buffer = (uint8_t*)malloc(size);
    fread(buffer, 1, size, file);
    return buffer;
}

// Temp structures used for writing
struct AssetSourceBitmap {
    const char* file_name;
};

struct AssetSourceSound {
    const char* file_name;
    u32 first_sample_index;
};

struct AssetSourceFont {
    const char* file_name;
};

struct AssetSource {
    AssetType type;
    union {
        AssetSourceBitmap bitmap;
        AssetSourceSound sound;
        AssetSourceFont font;
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

struct Font {
    i32 code_point_count;
    i32 first_code_point;
    i32 last_code_point;
    CodePoint* code_points;

    f32 font_height;
    i32 bitmap_width;
    i32 bitmap_height;
    i32 bitmap_bytes_per_pixel;
    u32* bitmap;
};

auto load_font(const char* path) -> Font {
    Font result = {};
    u8* file_content = read_whole_file(path);

    result.bitmap_width = 512;
    result.bitmap_height = 256;
    result.bitmap_bytes_per_pixel = sizeof(u32);
    result.bitmap = (u32*)malloc(result.bitmap_width * result.bitmap_height * result.bitmap_bytes_per_pixel);

    result.first_code_point = 32;
    result.last_code_point = 255;
    result.code_point_count = result.last_code_point - result.first_code_point;
    result.font_height = 32.0f;

    result.code_points = (CodePoint*)malloc(result.code_point_count * sizeof(CodePoint));
    stbtt_bakedchar* cdata = (stbtt_bakedchar*)malloc(result.code_point_count * sizeof(stbtt_bakedchar));
    u8* src_bitmap = (u8*)malloc(result.bitmap_width * result.bitmap_height);
    stbtt_BakeFontBitmap(file_content, 0, result.font_height,  //
        src_bitmap, result.bitmap_width, result.bitmap_height, //
        result.first_code_point, result.code_point_count,      //
        cdata);                                                // no guarantee this fits!

    // Flip y-axis and turn into u32 color
    {
        for (i32 y = 0; y < result.bitmap_height; y++) {
            u8* src = src_bitmap + (y * result.bitmap_width);
            u32* dest = result.bitmap + ((result.bitmap_height - y - 1) * result.bitmap_width);
            for (i32 x = 0; x < result.bitmap_width; x++) {
                u8 c = *(src++);
                *(dest++) = c << 24 | c << 16 | c << 8 | c;
            }
        }
    }

    for (i32 i = 0; i < result.code_point_count; i++) {

        stbtt_bakedchar* bc = &cdata[i];
        u16 y_max = result.bitmap_height - 1 - bc->y0;
        u16 y_min = result.bitmap_height - 1 - bc->y1;

        result.code_points[i].x0 = bc->x0;
        result.code_points[i].y0 = y_min;
        result.code_points[i].x1 = bc->x1;
        result.code_points[i].y1 = y_max;
        result.code_points[i].xoff = bc->xoff;
        result.code_points[i].yoff = bc->yoff;
        result.code_points[i].xadvance = bc->xadvance;

        char c = 'g';
        auto width = bc->x1 - bc->x0;
        auto height = bc->y1 - bc->y0;
        if (i == c - result.first_code_point) {
            printf("c=%c, width=%i, height=%i, yoff= %f, x0=%d, x1=%d, y0= %d, y1=%d\n", c, width, height, bc->yoff,
                bc->x0, bc->x1, bc->y0, bc->y1);
        }
    }

    stbi_flip_vertically_on_write(true);
    stbi_write_png("font_bitmap.png", result.bitmap_width, result.bitmap_height, result.bitmap_bytes_per_pixel,
        result.bitmap, result.bitmap_width * result.bitmap_bytes_per_pixel);

    free(file_content);
    free(cdata);

    return result;
}

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

static auto end_asset_group(GameAssetsWrite* assets) -> void {
    Assert(assets->current_asset_group);
    assets->asset_count = assets->current_asset_group->one_past_last_asset_index;
    assets->asset_tag_count = assets->current_asset_group->one_past_last_asset_tag_index;
    assets->current_asset_group = 0;
    assets->curr_asset_index = 0;
}

#define AddAssetGroup(assets, group_id) DeferLoop(begin_asset_group(assets, group_id), end_asset_group(assets))

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

static auto add_font_asset(GameAssetsWrite* assets, const char* file_name) -> FontId {
    AddedAsset asset = add_asset(assets);
    asset.source->type = AssetType_Font;
    asset.source->font.file_name = file_name;

    return FontId{ asset.id };
}

static auto add_tag(GameAssetsWrite* assets, AssetTagId tag_id, f32 value) -> void {
    u32 tag_index = assets->current_asset_group->one_past_last_asset_tag_index++;
    Assert(tag_index < MAX_TAGS_COUNT);
    AssetTag* tag = &assets->asset_tags[tag_index];
    tag->id = tag_id;
    tag->value = value;
    tag->asset_index = assets->curr_asset_index;
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
            AssetMeta* meta = assets->assets_meta + asset_idx;

            meta->data_offset = ftell(out);

            switch (source->type) {
            case AssetType_Bitmap: {
                Bitmap bitmap = load_bitmap_stbi(source->bitmap.file_name);
                meta->bitmap.dim[0] = bitmap.width;
                meta->bitmap.dim[1] = bitmap.height;

                Assert((bitmap.width * 4) == bitmap.pitch);
                fwrite(bitmap.data, bitmap.pitch * bitmap.height, 1, out);

                free(bitmap.data);
            } break;
            case AssetType_Audio: {
                WavFile wav_file = load_wav_file(source->sound.file_name);
                auto sample_count = wav_file.data_chunk.data_size / (wav_file.fmt_chunk.sample_byte_count);
                meta->audio.sample_count = sample_count;
                meta->audio.channel_count = wav_file.fmt_chunk.channel_count;
                meta->audio.chain = 0;

                fwrite(wav_file.data, sizeof(u8) * wav_file.data_chunk.data_size, 1, out);

                free(wav_file.data);
            } break;
            case AssetType_Font: {
                Font font = load_font(source->font.file_name);
                meta->font.code_point_first = font.first_code_point;
                meta->font.code_point_last = font.last_code_point;
                meta->font.code_point_count = font.code_point_count;

                meta->font.bitmap_width = font.bitmap_width;
                meta->font.bitmap_height = font.bitmap_height;
                meta->font.bitmap_size_per_pixel = font.bitmap_bytes_per_pixel;
                meta->font.font_height = font.font_height;

                fwrite(font.code_points, sizeof(CodePoint), font.code_point_count, out);
                fwrite(font.bitmap, font.bitmap_bytes_per_pixel, font.bitmap_width * font.bitmap_height, out);

            } break;
            case AssetType_Invalid:
            case AssetType_Count: {
                InvalidCodePath;
            }
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

    AddAssetGroup(assets, AssetGroupId_PlayerSpaceShip) {
        add_bitmap_asset(assets, "assets/bitmaps/player_1.png");
        add_tag(assets, AssetTag_SpaceShipDirection, 0.0);
        add_bitmap_asset(assets, "assets/bitmaps/player_left-1.png");
        add_tag(assets, AssetTag_SpaceShipDirection, -1.0);
        add_bitmap_asset(assets, "assets/bitmaps/player_right-1.png");
        add_tag(assets, AssetTag_SpaceShipDirection, 1.0);
    }

    begin_asset_group(assets, AssetGroupId_EnemySpaceShip);
    add_bitmap_asset(assets, "assets/bitmaps/blue_01.png");
    end_asset_group(assets);

    begin_asset_group(assets, AssetGroupId_Projectile);
    add_bitmap_asset(assets, "assets/bitmaps/projectile_1.png");
    end_asset_group(assets);

    begin_asset_group(assets, AssetGroupId_Test);
    add_bitmap_asset(assets, "assets/bitmaps/test.png");
    end_asset_group(assets);

    AddAssetGroup(assets, AssetGroupId_Explosion) {
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
    }

    AddAssetGroup(assets, AssetGroupId_Fonts_Ubuntu) {
        add_font_asset(assets, "assets/fonts/ubuntu/Ubuntu-Regular.ttf");
    }

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

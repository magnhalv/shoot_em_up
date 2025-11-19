#include <cstdio>
#include <cstdlib>

#include <platform/platform.h>

#include <core/logger.h>
#include <core/memory.h>
#include <core/memory_arena.h>

#include <engine/globals.hpp>

#include "assets.h"
#include "engine/hugin_file_formats.h"
#include "math/math.h"
#include "platform/types.h"

struct LoadAssetWork {
    PlatformFileHandle handle;
    u64 offset;
    u64 size;
    Asset* asset;

    TaskWithMemory* task;
};

static PLATFORM_WORK_QUEUE_CALLBACK(load_asset_work) {
    LoadAssetWork* work = (LoadAssetWork*)Data;

    if (work->asset->asset_memory->asset_type == AssetType_Bitmap) {
        Platform->read_file(&work->handle, work->offset, work->size, work->asset->asset_memory->bitmap.data);
        u32* data = (u32*)work->asset->asset_memory->bitmap.data;
        work->asset->state = AssetState_Loaded;
    }
    if (work->asset->asset_memory->asset_type == AssetType_Audio) {
        Platform->read_file(&work->handle, work->offset, work->size, work->asset->asset_memory->audio.data);
        u32* data = (u32*)work->asset->asset_memory->audio.data;
        work->asset->state = AssetState_Loaded;
        printf("Loaded audio!\n");
    }
    end_task(work->task);
}

auto get_bitmap_meta(GameAssets* game_assets, BitmapId id) -> HuginBitmap {
    Assert(id.value < game_assets->asset_count);
    AssetMeta* meta = game_assets->assets_meta + id.value;
    return meta->bitmap;
}

auto get_bitmap(GameAssets* game_assets, BitmapId id) -> LoadedBitmap* {
    LoadedBitmap* result = nullptr;

    Assert(id.value < game_assets->asset_count);
    Asset* asset = game_assets->assets + id.value;

    if (asset->state == AssetState_Loaded) {
        result = &asset->asset_memory->bitmap;
    }
    else {
        load_bitmap(game_assets, id);
    }

    return result;
}

auto get_audio(GameAssets* game_assets, AudioId id) -> LoadedAudio* {
    LoadedAudio* result = nullptr;

    Assert(id.value < game_assets->asset_count);
    Asset* asset = game_assets->assets + id.value;

    if (asset->state == AssetState_Loaded) {
        result = &asset->asset_memory->audio;
    }
    else {
        load_audio(game_assets, id);
    }

    return result;
}

auto load_bitmap(GameAssets* game_assets, BitmapId id) -> void {
    if (game_assets->asset_files_count == 0) {
        return;
    }
    Assert(game_assets->is_initialized);
    Assert(id.value < game_assets->asset_count);
    AssetMeta* meta = game_assets->assets_meta + id.value;
    Asset* asset = game_assets->assets + id.value;

    if (asset->state == AssetState_Unloaded) {
        TaskWithMemory* task = begin_task(Task_System);

        if (task) {
            Assert(asset->asset_memory == NULL);
            asset->asset_memory = allocate<AssetMemoryHeader>(game_assets->memory);

            asset->asset_memory->asset_type = AssetType_Bitmap;
            asset->asset_memory->bitmap.pitch = meta->bitmap.dim[0];
            asset->asset_memory->bitmap.width = meta->bitmap.dim[0];
            asset->asset_memory->bitmap.height = meta->bitmap.dim[1];
            asset->asset_memory->bitmap.align_percentage = meta->bitmap.align_percentage;
            auto size = meta->bitmap.dim[0] * meta->bitmap.dim[1] * BitmapBytePerPixel;
            asset->asset_memory->bitmap.data = allocate<u8>(game_assets->memory, size);

            LoadAssetWork* work = allocate<LoadAssetWork>(task->memory);
            work->size = size;
            work->offset = meta->data_offset;
            work->asset = asset;
            work->task = task;
            Assert(game_assets->asset_files[0].platform != nullptr);
            work->handle = game_assets->asset_files[asset->asset_file_index];
            assert(Platform->add_work_queue_entry != nullptr);
            Platform->add_work_queue_entry(Task_System->queue, load_asset_work, work);
            asset->state = AssetState_Queued;
        }
        else {
            printf("No tasks available!\n");
        }
    }
}

auto load_audio(GameAssets* game_assets, AudioId id) -> void {
    Assert(game_assets->is_initialized);
    Assert(id.value < game_assets->asset_count);
    AssetMeta* meta = game_assets->assets_meta + id.value;
    Asset* asset = game_assets->assets + id.value;

    if (asset->state == AssetState_Unloaded) {
        TaskWithMemory* task = begin_task(Task_System);

        if (task) {
            Assert(asset->asset_memory == NULL);
            asset->asset_memory = allocate<AssetMemoryHeader>(game_assets->memory);

            asset->asset_memory->asset_type = AssetType_Audio;
            asset->asset_memory->audio.sample_count = meta->audio.sample_count;
            asset->asset_memory->audio.channel_count = meta->audio.channel_count;
            asset->asset_memory->audio.chain = meta->audio.chain;

            auto size = BytesPerAudioSample * meta->audio.sample_count;
            asset->asset_memory->audio.data = allocate<u8>(game_assets->memory, size);

            LoadAssetWork* work = allocate<LoadAssetWork>(task->memory);
            work->size = size;
            work->offset = meta->data_offset;
            work->asset = asset;
            work->task = task;
            Assert(game_assets->asset_files[asset->asset_file_index].platform != nullptr);
            work->handle = game_assets->asset_files[asset->asset_file_index];
            assert(Platform->add_work_queue_entry != nullptr);
            Platform->add_work_queue_entry(Task_System->queue, load_asset_work, work);
            asset->state = AssetState_Queued;
        }
        else {
            printf("No tasks available!\n");
        }
    }
}

auto initialize_game_assets(MemoryArena* permanent) -> GameAssets* {
    GameAssets* game_assets = allocate<GameAssets>(permanent);
    game_assets->memory = permanent->allocate_arena(MegaBytes(1));

    PlatformFileGroup file_group = Platform->get_all_files_of_type_begin(PlatformFileType_AssetFile);
    assert(file_group.file_count <= ASSET_FILES_MAX_COUNT);

    i32 asset_count = 1; // Empty asset
    i32 asset_group_count = 1;
    i32 asset_tag_count = 0;

    HafHeader headers[ASSET_FILES_MAX_COUNT];
    for (auto i = 0; i < file_group.file_count; i++) {
        HafHeader* header = headers + i;

        PlatformFileHandle file_handle = Platform->open_next_file(&file_group);

        Platform->read_file(&file_handle, 0, sizeof(HafHeader), header);

        if (header->magic_value != HAF_MAGIC_VALUE) {
            InvalidCodePath;
        }

        if (header->version != HAF_VERSION) {
            log_error("Asset file version mismatch. File: %d. Engine: %d", header->version, HAF_VERSION);
            InvalidCodePath;
        }
        asset_count += header->asset_count - 1;
        asset_group_count += header->asset_group_count - 1;
        asset_tag_count += header->asset_tag_count;

        game_assets->asset_files[game_assets->asset_files_count++] = file_handle;
    }
    Platform->get_all_files_of_type_end(&file_group);

    game_assets->assets_meta = allocate<AssetMeta>(permanent, asset_count);
    game_assets->assets = allocate<Asset>(permanent, asset_count);
    game_assets->asset_tags = allocate<AssetTag>(permanent, asset_tag_count);

    game_assets->asset_groups[Asset_None].first_asset_index = 0;
    game_assets->asset_groups[Asset_None].one_past_last_asset_index = 1;
    game_assets->asset_groups[Asset_None].group_id = Asset_None;
    ZeroStruct(*game_assets->assets);
    ZeroStruct(*game_assets->assets_meta);
    game_assets->asset_count = 1;

    for (auto file_idx = 0; file_idx < game_assets->asset_files_count; file_idx++) {
        HafHeader* header = headers + file_idx;
        PlatformFileHandle* file_handle = &game_assets->asset_files[file_idx];

        AssetMeta* file_assets_meta = allocate<AssetMeta>(g_transient, header->asset_count);
        AssetGroup* file_asset_groups = allocate<AssetGroup>(g_transient, header->asset_group_count);
        AssetTag* file_asset_tags = allocate<AssetTag>(g_transient, header->asset_tag_count);

        Platform->read_file(file_handle, header->assets_block, header->asset_count * sizeof(AssetMeta), file_assets_meta);
        Platform->read_file(file_handle, header->asset_groups_block, header->asset_group_count * sizeof(AssetGroup), file_asset_groups);
        Platform->read_file(file_handle, header->assets_tag_block, header->asset_tag_count * sizeof(AssetTag), file_asset_tags);

        for (auto group_idx = 1; group_idx < header->asset_group_count; group_idx++) {
            AssetGroup* file_group = file_asset_groups + group_idx;
            // If this file has any assets for this group
            if (file_group->first_asset_index != 0) {
                {
                    // Use this diff to add to the local asset_id for the tags.
                    i32 local_to_global_asset_idx = game_assets->asset_count - file_group->first_asset_index;

                    AssetGroup* mem_group = game_assets->asset_groups + file_group->group_id;
                    mem_group->group_id = file_group->group_id;
                    mem_group->first_asset_index = game_assets->asset_count;

                    // HERE!
                    for (auto asset_idx = file_group->first_asset_index;
                        asset_idx < file_group->one_past_last_asset_index; asset_idx++) {
                        AssetMeta* file_meta = file_assets_meta + asset_idx;
                        game_assets->assets_meta[game_assets->asset_count] = *file_meta;
                        game_assets->assets[game_assets->asset_count].asset_file_index = file_idx;
                        game_assets->asset_count++;
                    }
                    if (file_group->group_id == Asset_PlayerSpaceShip) {
                        printf("Howdy\n");
                    }
                    mem_group->first_asset_tag_index = game_assets->asset_tag_count;
                    for (auto file_asset_tag_idx = file_group->first_asset_tag_index;
                        file_asset_tag_idx < file_group->one_past_last_asset_tag_index; file_asset_tag_idx++) {
                        AssetTag* tag = game_assets->asset_tags + game_assets->asset_tag_count++;
                        *tag = *(file_asset_tags + file_asset_tag_idx);
                        tag->asset_index += local_to_global_asset_idx;
                    }
                    mem_group->one_past_last_asset_tag_index = game_assets->asset_tag_count;
                    game_assets->asset_groups[file_group->group_id].one_past_last_asset_index = game_assets->asset_count;
                }
            }
        }
    }

    game_assets->is_initialized = true;
    return game_assets;
}

auto get_first_bitmap_id(GameAssets* game_assets, AssetGroupId asset_group_id) -> BitmapId {
    u32 result = 0;

    AssetGroup* group = game_assets->asset_groups + asset_group_id;
    if (group->first_asset_index != group->one_past_last_asset_index) {
        result = group->first_asset_index;
    }

    return BitmapId{ result };
}

auto get_closest_bitmap_id(GameAssets* game_assets, AssetGroupId asset_group_id, AssetTagId tag_id, f32 value) -> BitmapId {
    Assert(value <= 1.0 && value >= -1.0); // perhaps clamp instead?
    u32 result = 0;

    AssetGroup* group = game_assets->asset_groups + asset_group_id;
    f32 diff = 2.0; // biggest diff;

    result = group->first_asset_index;
    for (i32 tag_idx = group->first_asset_tag_index; tag_idx < group->one_past_last_asset_tag_index; tag_idx++) {
        AssetTag* tag = game_assets->asset_tags + tag_idx;
        if (tag->id == tag_id) {
            // TODO: HERE!!
            f32 curr_diff = hm::f32_abs(tag->value - value);
            printf("%f\n", curr_diff);
            if (curr_diff < diff) {
                result = tag->asset_index;
                diff = curr_diff;
            }
        }
    }
    printf("Result: %f\n", diff);
    printf("---------\n");

    return BitmapId{ result };
}

auto get_first_audio(GameAssets* game_assets, AssetGroupId asset_group_id) -> AudioId {
    u32 result = 0;

    AssetGroup* type = game_assets->asset_groups + asset_group_id;
    if (type->first_asset_index != type->one_past_last_asset_index) {
        result = type->first_asset_index;
    }

    return AudioId{ result };
}

auto get_first_bitmap_meta(GameAssets* game_assets, AssetGroupId asset_group_id) -> HuginBitmap {
    u32 id = 0;

    AssetGroup* type = game_assets->asset_groups + asset_group_id;
    if (type->first_asset_index != type->one_past_last_asset_index) {
        id = type->first_asset_index;
    }

    auto* meta = game_assets->assets_meta + id;
    return meta->bitmap;
}

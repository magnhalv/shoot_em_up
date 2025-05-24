#include <cstdlib>
#include <libs/stb/stb_image.h>

#include <platform/platform.h>

#include <engine/globals.hpp>
#include <engine/memory.h>
#include <engine/memory_arena.h>

#include "assets.h"

struct LoadAssetWork {
    u64 file_offset;
    u64 size;
    Asset* asset;

    TaskWithMemory* task;
};

static PLATFORM_WORK_QUEUE_CALLBACK(load_asset_work) {
    LoadAssetWork* work = (LoadAssetWork*)Data;

    if (work->asset->asset_memory->asset_type == AssetType_Bitmap) {
        FILE* file = fopen("assets.haf", "rb");
        fseek(file, work->file_offset, SEEK_SET);
        fread(work->asset->asset_memory->bitmap.data, work->size, 1, file);
        fclose(file);

        work->asset->state = AssetState_Loaded;
        printf("Asset loaded\n");
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

    return result;
}

auto load_bitmap(GameAssets* game_assets, BitmapId id, MemoryArena* permanent) -> void {
    Assert(id.value < game_assets->asset_count);
    AssetMeta* meta = game_assets->assets_meta + id.value;
    Asset* asset = game_assets->assets + id.value;

    if (asset->state == AssetState_Unloaded) {
        TaskWithMemory* task = begin_task(g_task_system);

        if (task) {
            Assert(asset->asset_memory == NULL);
            asset->asset_memory = allocate<AssetMemoryHeader>(permanent);

            asset->asset_memory->asset_type = AssetType_Bitmap;
            auto size = meta->bitmap.dim[0] * meta->bitmap.dim[1] * BitmapBytePerPixel;
            asset->asset_memory->bitmap.data = allocate<u8>(permanent, size);

            asset->asset_memory->bitmap.pitch = meta->bitmap.dim[0];
            asset->asset_memory->bitmap.width = meta->bitmap.dim[0];
            asset->asset_memory->bitmap.height = meta->bitmap.dim[1];
            asset->asset_memory->bitmap.align_percentage = meta->bitmap.align_percentage;
            asset->asset_memory->bitmap.texture_handle = 0;

            LoadAssetWork* work = allocate<LoadAssetWork>(task->memory);
            work->size = size;
            work->file_offset = meta->data_offset;
            work->asset = asset;
            work->task = task;
            assert(Platform->add_work_queue_entry != nullptr);
            Platform->add_work_queue_entry(g_task_system->queue, load_asset_work, work);
            asset->state = AssetState_Queued;
        }
        else {
            printf("No tasks available!\n");
        }
    }
}

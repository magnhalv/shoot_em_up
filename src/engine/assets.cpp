#include <cstdlib>

#include <platform/platform.h>

#include <engine/globals.hpp>
#include <engine/logger.h>
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
        const char* file_path = "assets.haf";
        FILE* file = fopen(file_path, "rb");
        if (file) {
            fseek(file, work->file_offset, SEEK_SET);
            fread(work->asset->asset_memory->bitmap.data, work->size, 1, file);
            u32* data = (u32*)work->asset->asset_memory->bitmap.data;

            work->asset->state = AssetState_Loaded;
            fclose(file);
        }
        else {
            log_error("Asset file '%s' not found!\n", file_path);
        }
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
        TaskWithMemory* task = begin_task(Task_System);

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
            Platform->add_work_queue_entry(Task_System->queue, load_asset_work, work);
            asset->state = AssetState_Queued;
        }
        else {
            printf("No tasks available!\n");
        }
    }
}

auto initialize_game_assets(const char* file_name, MemoryArena* arena) -> GameAssets* {
    FILE* file = fopen(file_name, "rb");
    GameAssets* game_assets = allocate<GameAssets>(arena);

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

auto get_first_bitmap_from(GameAssets* game_assets, AssetTypeId asset_type_id) -> BitmapId {
    u32 result = 0;

    HuginAssetType* type = game_assets->asset_types + asset_type_id;
    if (type->first_asset_index != type->one_past_last_asset_index) {
        result = type->first_asset_index;
    }

    return BitmapId{ result };
}

auto get_first_bitmap_meta(GameAssets* game_assets, AssetTypeId asset_type_id) -> HuginBitmap {
    u32 id = 0;

    HuginAssetType* type = game_assets->asset_types + asset_type_id;
    if (type->first_asset_index != type->one_past_last_asset_index) {
        id = type->first_asset_index;
    }

    auto* meta = game_assets->assets_meta + id;
    return meta->bitmap;
}

#include <cstdlib>

#include <platform/platform.h>

#include <engine/globals.hpp>
#include <engine/logger.h>
#include <engine/memory.h>
#include <engine/memory_arena.h>

#include "assets.h"

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
    Assert(game_assets->is_initialized);
    Assert(id.value < game_assets->asset_count);
    AssetMeta* meta = game_assets->assets_meta + id.value;
    Asset* asset = game_assets->assets + id.value;

    if (asset->state == AssetState_Unloaded) {
        TaskWithMemory* task = begin_task(Task_System);

        if (task) {
            Assert(asset->asset_memory == NULL);
            asset->asset_memory = allocate<AssetMemoryHeader>(permanent);

            asset->asset_memory->asset_type = AssetType_Bitmap;
            asset->asset_memory->bitmap.pitch = meta->bitmap.dim[0];
            asset->asset_memory->bitmap.width = meta->bitmap.dim[0];
            asset->asset_memory->bitmap.height = meta->bitmap.dim[1];
            asset->asset_memory->bitmap.align_percentage = meta->bitmap.align_percentage;
            asset->asset_memory->bitmap.texture_handle = 0;
            auto size = meta->bitmap.dim[0] * meta->bitmap.dim[1] * BitmapBytePerPixel;
            asset->asset_memory->bitmap.data = allocate<u8>(permanent, size);

            LoadAssetWork* work = allocate<LoadAssetWork>(task->memory);
            work->size = size;
            work->offset = meta->data_offset;
            work->asset = asset;
            work->task = task;
            Assert(game_assets->asset_files[0].platform != nullptr);
            work->handle = game_assets->asset_files[0];
            assert(Platform->add_work_queue_entry != nullptr);
            Platform->add_work_queue_entry(Task_System->queue, load_asset_work, work);
            asset->state = AssetState_Queued;
        }
        else {
            printf("No tasks available!\n");
        }
    }
}

auto initialize_game_assets(MemoryArena* arena) -> GameAssets* {
    GameAssets* game_assets = allocate<GameAssets>(arena);

    PlatformFileGroup file_group = Platform->get_all_files_of_type_begin(PlatformFileType_AssetFile);
    assert(file_group.file_count <= ASSET_FILES_MAX_COUNT);

    for (auto i = 0; i < file_group.file_count; i++) {
        HafHeader header = { 0 };
        HafHeader* ptr = &header;

        PlatformFileHandle file_handle = Platform->open_next_file(&file_group);

        Platform->read_file(&file_handle, 0, sizeof(HafHeader), ptr);

        if (header.magic_value != HAF_MAGIC_VALUE) {
            InvalidCodePath;
        }

        if (header.version != HAF_VERSION) {
            InvalidCodePath;
        }
        u32 asset_count = header.asset_count;
        u32 asset_type_count = header.asset_type_count;

        Platform->read_file(&file_handle, header.asset_types, asset_type_count * sizeof(HuginAssetType), game_assets->asset_types);
        Assert(game_assets->asset_types[1].type_id == Asset_PlayerSpaceShip);

        game_assets->asset_count = asset_count;
        game_assets->assets_meta = allocate<AssetMeta>(arena, asset_count);
        game_assets->assets = allocate<Asset>(arena, asset_count);
        Platform->read_file(&file_handle, header.assets, asset_count * sizeof(AssetMeta), game_assets->assets_meta);

        game_assets->asset_files[game_assets->asset_files_count++] = file_handle;
    }
    Platform->get_all_files_of_type_end(&file_group);

    game_assets->is_initialized = true;
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

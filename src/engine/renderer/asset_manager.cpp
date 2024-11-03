#include "asset_manager.h"

AssetManager *asset_manager = nullptr;

auto asset_manager_set_memory(void *memory) -> void {
    asset_manager = static_cast<AssetManager*>(memory);
}

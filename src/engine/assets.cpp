#include <libs/stb/stb_image.h>

#include <platform/platform.h>
#include <engine/memory.h>
#include <engine/memory_arena.h>

#include "assets.h"


auto load_bitmap(const char* path, MemoryArena *arena) -> Bitmap* {
    auto *result = allocate<Bitmap>(*arena);
    i32 num_channels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char* data = stbi_load(path, &result->width, &result->height, &num_channels, 4);
    
    if (num_channels != 4) {
      InvalidCodePath;
    }
    
    if (data) {
      auto size = result->width*result->height*num_channels; // 1 byte per channel
      result->data = allocate<u8>(*arena, size);
      mem_copy(data, result->data, size);
      stbi_image_free(data);
    }
    else {
      InvalidCodePath;
    }

    return result;
}

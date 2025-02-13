#include "renderer.h"

auto draw_rectangle(vec2 min, vec2 max, vec4 color) {

}

auto clear(i32 client_width, i32 client_height, vec4 color) {
  gl->viewport(0, 0, client_width, client_height);
  gl->clear_color(color.x, color.y, color.z, color.w);
  gl->clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Remove depth buffer?
}

auto render(RenderGroup *group, i32 client_width, i32 client_height) -> void {

  for (u32 base_address = 0; base_address < group->push_buffer_size;) {
    printf("Iteration\n");
    RenderGroupEntryHeader *header = (RenderGroupEntryHeader*)(group->push_buffer + base_address);
    base_address += sizeof(RenderGroupEntryHeader);

    void *data = (u8*)header + sizeof(*header);
    switch (header->type) {
    case RenderCommands_RenderEntryClear: {
      RenderEntryClear *entry = (RenderEntryClear*)data;

      clear(client_width, client_height, entry->color);

      base_address += sizeof(*entry);
      } break;
      case RenderCommands_RenderEntryDrawRectangle: {
        
      } break;
    }
  } 
}

#ifndef HOT_RELOAD_OPENGL_MEMORY_H
#define HOT_RELOAD_OPENGL_MEMORY_H

#include <platform/types.h>

void debug_set_memory(void* memory, u64 size);
void mem_copy(void* src, void* dest, u64 size);

void DEBUG_print_memory_as_hex(void* memory, u64 size);

#endif // HOT_RELOAD_OPENGL_MEMORY_H

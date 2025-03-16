#include "memory.h"
#include <cassert>
#include <cstring>

void debug_set_memory(void* memory, u64 size) {
  assert(size % 4 == 0);
  memset(memory, 0, size);
}


auto mem_copy(void *src, void* dest, u64 size) -> void {
  u8 *s = (u8*)src;
  u8 *d = (u8*)dest;
  for (auto i = 0; i < size; i++) {
    *(d + i) = *(s + i);
  }
}

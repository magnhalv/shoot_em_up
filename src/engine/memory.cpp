#include "memory.h"
#include <cassert>
#include <cstring>

void debug_set_memory(void* memory, u64 size) {
  assert(size % 4 == 0);
  memset(memory, 0, size);
}

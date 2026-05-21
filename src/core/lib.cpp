#include "lib.hpp"

#include "logger.cpp"
#include "memory.cpp"
#include "memory_arena.cpp"

// Third party
#include "../third-party/stb_sprintf.cpp"

auto initialize_core_lib() -> void {
    initialize_memory_lib();
}

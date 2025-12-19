cmake_minimum_required(VERSION 3.22)
project(ShootEmUp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

####### LOCAL ########

include_directories(include)
include_directories(src)
include_directories(src/deps)

add_executable(
  shoot_em_up
  src/win_main.cpp

  src/core/memory.cpp
  src/core/memory_arena.cpp
  src/core/logger.cpp
 )

add_executable(
  asset_builder
  src/asset_builder_main.cpp

  src/core/memory.cpp
  src/core/memory_arena.cpp
  src/core/logger.cpp

  src/deps/libs.cpp
)

# This needs to be after the linking of opengl, otherwise we get linking issues against glad
# Something related to assimp.
add_subdirectory(src)
add_subdirectory(tests)

add_custom_command(
  TARGET shoot_em_up POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json"
  "${CMAKE_SOURCE_DIR}/compile_commands.json"
)

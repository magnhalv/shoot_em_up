cmake_minimum_required(VERSION 3.22)
project(ShootEmUp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

####### DEPENDENCIES ########
# GLAD
include_directories(shared_deps/glad/include)
set(GLAD_SOURCES shared_deps/glad/src/gl.c shared_deps/glad/src/vulkan.c)
if(WIN32)
    set(GLAD_SOURCES ${GLAD_SOURCES} shared_deps/glad/src/wgl.c)
endif()

find_package(OpenGL REQUIRED)

####### END DEPENDENCIES ########

include_directories(include)
include_directories(src)
include_directories(src/deps/)

add_executable(shoot_em_up
        src/win_main.cpp
        ${GLAD_SOURCES}
        )

target_link_libraries(shoot_em_up ${OPENGL_gl_LIBRARY} )

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

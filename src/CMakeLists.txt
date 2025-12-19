cmake_minimum_required(VERSION 3.10)   # Adjust according to your installed version

## Engine 
project(engine)

option(ENGINE_BUILD_SHARED_LIBS "Build using shared libraries" ON)

set(CORE_SOURCE_FILES
  core/memory.cpp
  core/memory_arena.cpp
  core/logger.cpp
)

set(MATH_SOURCE_FILES
  math/mat3.cpp
  math/mat4.cpp
  math/vec2.cpp
  math/vec3.cpp
)

set(SOURCE_FILES
    engine/array.cpp
    engine/engine.cpp
    engine/audio.cpp
    engine/assets.cpp
    engine/globals.cpp
    engine/options.cpp
    ${CORE_SOURCE_FILES}
)

add_library(engine_dyn SHARED ${SOURCE_FILES})
add_library(engine_static ${SOURCE_FILES} )

include_directories(../shared_deps/glad/include)
include_directories(deps/src/glm)

include_directories(deps/libs/freetype/include)
add_subdirectory(deps/libs/freetype)

add_subdirectory(math)

set(LIBS math freetype)

# Dynamic configuration

target_link_libraries(engine_dyn ${LIBS})
set_target_properties(engine_dyn PROPERTIES DEBUG_POSTFIX "")

if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(engine_dyn PUBLIC ENGINE_DEBUG=1)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(engine_dyn PUBLIC ENGINE_DEBUG=0)
endif ()

set_target_properties(engine_dyn PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/app
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/app
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/app
        )

# Static configuration
target_link_libraries(engine_static ${LIBS})
#set_target_properties(engine_static PROPERTIES DEBUG_POSTFIX "")

if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_definitions(engine_static PUBLIC ENGINE_DEBUG=1)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(engine_static PUBLIC ENGINE_DEBUG=0)
endif ()

set_target_properties(engine_dyn PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/app
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/app
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/app
        )

target_compile_options(engine_dyn PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W4 /WX /wd4201 /analyze>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4201 /analyze>
)

## OpenGL renderer
include_directories(../shared_deps/glad/include)
set(GLAD_SOURCES ../shared_deps/glad/src/gl.c ../shared_deps/glad/src/vulkan.c)
if(WIN32)
    set(GLAD_SOURCES ${GLAD_SOURCES} ../shared_deps/glad/src/wgl.c)
endif()

find_package(OpenGL REQUIRED)

set(OPENGL_RENDERER_SRC_FILES
  renderers/gl/gl_shader.cpp
  renderers/win32_opengl_renderer.cpp
  ${MATH_SOURCE_FILES}
  ${CORE_SOURCE_FILES}
  ${GLAD_SOURCES}
)

add_library(opengl_renderer SHARED ${OPENGL_RENDERER_SRC_FILES})
target_link_libraries(opengl_renderer ${OPENGL_gl_LIBRARY} )
set_target_properties(opengl_renderer PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/app
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/app
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/app
)

## Software renderer
set(SOFTWARE_RENDERER_SRC_FILES
  renderers/win32_software_renderer.cpp
  ${CORE_SOURCE_FILES}
  ${MATH_SOURCE_FILES}
)

add_library(win32_software_renderer SHARED ${SOFTWARE_RENDERER_SRC_FILES})
set_target_properties(win32_software_renderer PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/app
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/app
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/app
)


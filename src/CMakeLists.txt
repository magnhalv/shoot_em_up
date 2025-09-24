cmake_minimum_required(VERSION 3.10)   # Adjust according to your installed version

project(engine)

option(ENGINE_BUILD_SHARED_LIBS "Build using shared libraries" ON)

set(SOURCE_FILES
        engine/array.cpp

        engine/engine.cpp

        engine/audio.cpp
        engine/assets.cpp
        engine/logger.cpp
        engine/memory.cpp
        engine/memory_arena.cpp
        engine/globals.cpp
        engine/options.cpp

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


## OpenGL renderer

include_directories(../shared_deps/glad/include)
set(GLAD_SOURCES ../shared_deps/glad/src/gl.c ../shared_deps/glad/src/vulkan.c)
if(WIN32)
    set(GLAD_SOURCES ${GLAD_SOURCES} ../shared_deps/glad/src/wgl.c)
endif()

find_package(OpenGL REQUIRED)


set(OPENGL_RENDERER_SRC_FILES
  renderers/gl/gl.cpp
  renderers/gl/gl_shader.cpp
  renderers/gl/gl_vao.cpp

  renderers/opengl_renderer.cpp

  ${GLAD_SOURCES}
)

add_library(opengl_renderer SHARED ${OPENGL_RENDERER_SRC_FILES})
target_link_libraries(opengl_renderer ${OPENGL_gl_LIBRARY} )
set_target_properties(opengl_renderer PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/app
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/app
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/app
)


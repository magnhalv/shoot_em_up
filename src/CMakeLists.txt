cmake_minimum_required(VERSION 3.10)   # Adjust according to your installed version

project(engine)

option(ENGINE_BUILD_SHARED_LIBS "Build using shared libraries" ON)

set(SOURCE_FILES
        engine/array.cpp

        engine/renderer/asset_manager.cpp

        engine/cli/cli.cpp

        engine/engine.cpp

        engine/gui.cpp

        engine/gl/gl.cpp
        engine/gl/gl_shader.cpp
        engine/gl/gl_vao.cpp

        engine/audio.cpp
        engine/logger.cpp
        engine/memory.cpp
        engine/memory_arena.cpp
        engine/globals.cpp
        engine/renderer.cpp
        engine/text_renderer.cpp
        engine/options.cpp
        engine/sprite.cpp

        deps/libs.cpp
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

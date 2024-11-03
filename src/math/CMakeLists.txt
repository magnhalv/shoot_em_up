cmake_minimum_required(VERSION 3.10)   # Adjust according to your installed version

project(Math)

add_library(math STATIC
        mat4.cpp
        quat.cpp
        transform.cpp
        vec3.cpp
        )


set_target_properties(math PROPERTIES DEBUG_POSTFIX "")

set_target_properties(math PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)
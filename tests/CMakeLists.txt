cmake_minimum_required(VERSION 3.0)
project(test_project VERSION 0.0.1 LANGUAGES CXX)

# Make test executable
add_executable(tests
        test_main.cpp
        memory_arena_test.cpp
        cli_echo_test.cpp
        linked_list_test.cpp
        fixed_string_test.cpp
        growable_string_text.cpp
        line_buffer_test.cpp
        )

set(ENGINE_BUILD_SHARED_LIBS OFF)
target_include_directories(tests PRIVATE ../src/engine)

target_compile_definitions(engine_static PUBLIC ENGINE_TEST=1 DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS=1)

target_compile_features(tests PRIVATE cxx_std_20)
target_link_libraries(tests PRIVATE engine_static)

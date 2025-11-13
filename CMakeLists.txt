cmake_minimum_required(VERSION 3.31)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(FLEXIC_ENABLE_BENCHES "Enable benchmarks for FlexiC")
option(FLEXIC_ENABLE_TESTS "Enable tests for FlexiC")
option(FLEXIC_ENABLE_TEST_COVERAGE "Enable code coverage on test suite")
option(FLEXIC_ENABLE_FUZZER "Enable fuzzing via libFuzzer")
option(FLEXIC_COMPILE_AS_CXX "Compile library as C++")

project(flexic LANGUAGES C CXX)

# C library

set(FLEXIC_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/flexic.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/flexic.c")

add_library(flexic STATIC)
target_sources(flexic PRIVATE ${FLEXIC_SOURCES})
target_include_directories(flexic PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

include(FindSanitizers)
add_sanitizers(flexic)

if(FLEXIC_COMPILE_AS_CXX)
    set_source_files_properties(${FLEXIC_SOURCES} PROPERTIES LANGUAGE CXX )
endif()

if(FLEXIC_ENABLE_TESTS)
    add_subdirectory(tests)
endif()

if(FLEXIC_ENABLE_BENCHES)
    add_subdirectory(benches)
endif()

if(FLEXIC_ENABLE_FUZZER)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "Fuzzing target requires Clang.")
    endif()
    add_subdirectory(fuzzer)
endif()

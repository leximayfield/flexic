cmake_minimum_required(VERSION 3.31)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(FLEXIC_ENABLE_TESTS "Enable tests for FlexiC")
option(FLEXIC_ENABLE_BENCHES "Enable benchmarks for FlexiC")

project(flexic LANGUAGES C CXX)

# C library

set(FLEXIC_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/flexic.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/flexic.c")

add_library(flexic STATIC)
target_sources(flexic PRIVATE ${FLEXIC_SOURCES})
target_include_directories(flexic PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(MSVC)
    target_compile_options(flexic PRIVATE /W4 /we4013)
else()
    target_compile_options(flexic PRIVATE -Wall -Wextra)
endif()

if(FLEXIC_ENABLE_TESTS)
    add_subdirectory(tests)
endif()

if(FLEXIC_ENABLE_BENCHES)
    add_subdirectory(benches)
endif()

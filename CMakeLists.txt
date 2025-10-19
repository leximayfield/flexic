cmake_minimum_required(VERSION 3.31)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(FLEXIC_ENABLE_TESTS "Enable tests for FlexiC")

project(flexic LANGUAGES C CXX)

add_library(flexic STATIC)
target_sources(flexic PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/flexic.h")
target_sources(flexic PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/flexic.c")
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

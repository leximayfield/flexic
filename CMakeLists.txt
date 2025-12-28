cmake_minimum_required(VERSION 3.26)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(FLEXIC_ENABLE_BENCHES "Enable benchmarks for FlexiC")
option(FLEXIC_ENABLE_DOCS "Enable documentation target for FlexiC")
option(FLEXIC_ENABLE_TESTS "Enable tests for FlexiC")
option(FLEXIC_ENABLE_TEST_COVERAGE "Enable code coverage on test suite")
option(FLEXIC_ENABLE_FUZZER "Enable fuzzing via libFuzzer")
option(FLEXIC_COMPILE_AS_CXX "Compile library as C++")
option(FLEXIC_FEATURE_PARSER "Enable parser feature" YES)
option(FLEXIC_FEATURE_JSON "Enable JSON writer feature" YES)

set(FLEXIC_OVERRIDE_MAX_DEPTH "" CACHE STRING "Override default iteration depth")
set(FLEXIC_OVERRIDE_MAX_ITERABLES "" CACHE STRING "Override default iteration limit")

project(flexic LANGUAGES C CXX)

# C library

set(FLEXIC_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/flexic.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/flexic.c")

add_library(flexic STATIC)
target_sources(flexic PRIVATE ${FLEXIC_SOURCES})
target_include_directories(flexic PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
if(NOT FLEXIC_FEATURE_PARSER)
    target_compile_definitions(flexic PUBLIC FLEXI_FEATURE_PARSER=0)
endif()
if(NOT FLEXIC_FEATURE_JSON)
    target_compile_definitions(flexic PUBLIC FLEXI_FEATURE_JSON=0)
endif()
if(FLEXIC_OVERRIDE_MAX_DEPTH)
    target_compile_definitions(flexic PRIVATE
        FLEXI_CONFIG_MAX_DEPTH=${FLEXIC_OVERRIDE_MAX_DEPTH})
endif()
if(FLEXIC_OVERRIDE_MAX_ITERABLES)
    target_compile_definitions(flexic PRIVATE
        FLEXI_CONFIG_MAX_ITERABLES=${FLEXIC_OVERRIDE_MAX_ITERABLES})
endif()

if(FLEXIC_COMPILE_AS_CXX)
    set_source_files_properties(${FLEXIC_SOURCES} PROPERTIES LANGUAGE CXX )
endif()

include(FindSanitizers)
add_sanitizers(flexic)

# nlohmann_json is needed by test suite and benchmarks

if(FLEXIC_ENABLE_TESTS OR FLEXIC_ENABLE_BENCHES)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json
        GIT_TAG v3.12.0)
    FetchContent_MakeAvailable(nlohmann_json)
endif()

if(FLEXIC_ENABLE_DOCS)
    add_subdirectory(docs)
endif()

if(FLEXIC_ENABLE_TESTS)
    add_subdirectory(tests)
endif()

if(FLEXIC_ENABLE_BENCHES AND FLEXIC_FEATURE_PARSER)
    add_subdirectory(benches)
endif()

if(FLEXIC_ENABLE_FUZZER)
    add_subdirectory(fuzzer)
endif()

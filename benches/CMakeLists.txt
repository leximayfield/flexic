# Grab dependencies
set(FLATBUFFERS_BUILD_FLATC OFF)
set(FLATBUFFERS_BUILD_TESTS OFF)
set(FLATBUFFERS_INSTALL OFF)

# Fix a linker bug
set(FLATBUFFERS_LOCALE_INDEPENDENT 0)
include(CheckCXXSymbolExists)
add_definitions(-DFLATBUFFERS_LOCALE_INDEPENDENT=$<BOOL:${FLATBUFFERS_LOCALE_INDEPENDENT}>)

include(FetchContent)
FetchContent_Declare(
    flatbuffers
    GIT_REPOSITORY https://github.com/google/flatbuffers
    GIT_TAG v25.9.23)
FetchContent_MakeAvailable(flatbuffers)
FetchContent_Declare(
    yyjson
    GIT_REPOSITORY https://github.com/ibireme/yyjson
    GIT_TAG 0.12.0)
FetchContent_MakeAvailable(yyjson)

# The actual benchmark executable

add_executable(flexic_bench)
target_sources(flexic_bench PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/flexic_bench.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/flexic_bench.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/bench_seek_key.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/bench_walk.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/nanobench.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/nanobench.h")
target_link_libraries(flexic_bench PRIVATE
    flexic flatbuffers yyjson nlohmann_json)

# Copy over benchmark files

file(GLOB FLEXIC_BENCH_FILES
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.flexbuf"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.json")
foreach(FLEXIC_BENCH_FILE ${FLEXIC_BENCH_FILES})
    add_custom_command(
        TARGET flexic_bench POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${CMAKE_CURRENT_SOURCE_DIR}/${FLEXIC_BENCH_FILE}"
                "${CMAKE_CURRENT_BINARY_DIR}/${FLEXIC_BENCH_FILE}"
        VERBATIM)
endforeach()

# Enable LTO on all libraries
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_RESULT OUTPUT IPO_ERROR)

if(IPO_RESULT)
    set_property(
        TARGET flexic_bench
        PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(
        TARGET flexic
        PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(
        TARGET flatbuffers
        PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(
        TARGET yyjson
        PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(
        TARGET nlohmann_json
        PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    message(STATUS "LTO enabled for benchmarking")
else()
    message(STATUS "LTO not enabled for benchmarking (${error})")
endif()

# Custom target to run benchmarks

add_custom_target(flexic_perf
                   COMMAND flexic_bench
                   DEPENDS flexic_bench
                   WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
                   VERBATIM)

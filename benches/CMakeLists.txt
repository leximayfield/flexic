set(FLATBUFFERS_BUILD_FLATC OFF)
set(FLATBUFFERS_BUILD_TESTS OFF)
set(FLATBUFFERS_INSTALL OFF)

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

add_custom_command(
        TARGET flexic_bench POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${CMAKE_CURRENT_SOURCE_DIR}/large_doc1.flexbuf"
                "${CMAKE_CURRENT_BINARY_DIR}/large_doc1.flexbuf")
add_custom_command(
        TARGET flexic_bench POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
                "${CMAKE_CURRENT_SOURCE_DIR}/large_doc1.json"
                "${CMAKE_CURRENT_BINARY_DIR}/large_doc1.json")

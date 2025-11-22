add_executable(flexic_bench)
target_sources(flexic_bench PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/flexic_bench.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/json.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/nanobench.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/nanobench.h")
target_link_libraries(flexic_bench PRIVATE flexic)

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

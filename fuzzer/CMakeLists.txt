include(FindFSan)

add_executable(flexic_fuzzer fuzzer.cpp)
target_link_libraries(flexic_fuzzer PRIVATE flexic)
add_sanitize_address(flexic_fuzzer)
add_sanitize_fuzzer(flexic_fuzzer)

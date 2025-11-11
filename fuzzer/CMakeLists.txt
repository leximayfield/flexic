add_compile_options(-fsanitize=fuzzer,address)
add_link_options(-fsanitize=fuzzer,address)

add_executable(flexic_fuzzer fuzzer.cpp)
target_link_libraries(flexic_fuzzer PRIVATE flexic)

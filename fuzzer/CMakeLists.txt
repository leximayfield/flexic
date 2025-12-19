add_executable(flexic_fuzzer
    "fuzzer.cpp"
    "../src/flexic.c")
target_include_directories(flexic_fuzzer PRIVATE "../include")
if(MSVC)
    target_compile_options(flexic_fuzzer PRIVATE "/fsanitize=fuzzer")
else()
    target_compile_options(flexic_fuzzer PRIVATE "-fsanitize=address,fuzzer,undefined")
    target_link_options(flexic_fuzzer PRIVATE "-fsanitize=address,fuzzer,undefined")
endif()

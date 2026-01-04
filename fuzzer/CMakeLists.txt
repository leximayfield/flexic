# Fuzzing the parser

function(target_fuzz_options TARGET)
    target_include_directories(${TARGET} PRIVATE "../include")
    if(MSVC)
        target_compile_options(${TARGET} PRIVATE "/fsanitize=fuzzer")
    else()
        target_compile_options(${TARGET} PRIVATE "-fsanitize=address,fuzzer,undefined")
        target_link_options(${TARGET} PRIVATE "-fsanitize=address,fuzzer,undefined")
    endif()

    if(WIN32)
        set_property(TARGET ${TARGET} PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endfunction()

add_executable(flexic_fuzz_parser
    "flexic_fuzz_parser.cpp"
    "../src/flexic.c")
target_fuzz_options(flexic_fuzz_parser)

add_executable(flexic_fuzz_writer
    "flexic_fuzz_writer.cpp"
    "../src/flexic.c")
target_fuzz_options(flexic_fuzz_writer)

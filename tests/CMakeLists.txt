include(GoogleTest)

# Testing enables warnings

if(MSVC)
    target_compile_options(flexic PRIVATE /W4 /we4013 /we4024 /we4047 /we4133 /w44062 /w44388)
else()
    target_compile_options(flexic PRIVATE -Wall -Wextra)
endif()
set_target_properties(flexic PROPERTIES
    C_CLANG_TIDY "clang-tidy;-checks=*"
    EXPORT_COMPILE_COMMANDS YES)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Test suite.
add_executable(flexic_test)
target_sources(flexic_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cursor_float.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cursor_int.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/parser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/parser_error.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/write_float.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/write_int.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/write_map.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/write_other.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/write_vector.cpp")
target_link_libraries(flexic_test PRIVATE flexic GTest::gtest_main)

add_sanitizers(flexic_test)
add_sanitizers(gtest)
add_sanitizers(gtest_main)

gtest_discover_tests(flexic_test)

if(FLEXIC_ENABLE_TEST_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        include(CodeCoverage)
        append_coverage_compiler_flags_to_target(flexic_test)
        setup_target_for_coverage_lcov(
            NAME flexic_test_cov
            EXECUTABLE flexic_test
            DEPENDENCIES flexic_test
            EXCLUDE "build/*" "tests" "/usr/include/*"
            LCOV_ARGS --ignore-errors mismatch)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        include(LLVMCov)
        append_llvmcov_compiler_flags_to_target(flexic)
        setup_target_for_coverage_llvmcov(
            NAME flexic_test_cov
            EXECUTABLE flexic_test
            DEPENDENCIES flexic_test
            OBJECT flexic
            SOURCES ${FLEXIC_SOURCES})
    else()
        message(FATAL_ERROR "Test coverage not supported by compiler")
    endif()
endif()

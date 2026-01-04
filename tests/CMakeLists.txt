# Testing enables warnings

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(flexic PRIVATE -Wall -Wextra -Wmost)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(flexic PRIVATE /W4 /we4013 /we4024 /we4047 /we4133 /we4244 /w44062 /w44388)
else()
    target_compile_options(flexic PRIVATE -Wall -Wextra)
endif()

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG v3.12.0)
FetchContent_MakeAvailable(Catch2)

# Test suite.
add_executable(flexic_test)
target_sources(flexic_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cursor_bool.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cursor_error.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cursor_float.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cursor_foreach.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cursor_sint.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cursor_uint.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cursor_map.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cursor_string.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/cursor_typed_vector.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/json.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/parser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/parser_error.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/write_error.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/write_float.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/write_int.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/write_map.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/write_other.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/write_vector.cpp")
set_property(TARGET flexic_test
    PROPERTY CXX_STANDARD 17)
set_property(TARGET flexic_test
    PROPERTY CXX_STANDARD_REQUIRED TRUE)
target_link_libraries(flexic_test PRIVATE
    flexic Catch2WithMain nlohmann_json)
if(MSVC)
    target_compile_options(flexic_test PRIVATE "/MP")
endif()

add_sanitizers(flexic_test)
add_sanitizers(Catch2)
add_sanitizers(Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(flexic_test)

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

include(CopyTarget)
copy_to_target(
    TARGET flexic_test
    GLOB "*.flexbuf")

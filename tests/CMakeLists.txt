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
    "${CMAKE_CURRENT_SOURCE_DIR}/simple.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/gold.cpp")
target_include_directories(flexic_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(flexic_test PRIVATE flexic GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(flexic_test)

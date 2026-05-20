cmake_minimum_required(VERSION 3.10)

# Enable testing
enable_testing()

# Download and configure Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.zip
)
FetchContent_MakeAvailable(googletest)

# Add test executable
add_executable(twig_tests
    test_variant.cpp
    test_tags.cpp
    test_functions.cpp
)

# Link against the main library and gtest
target_link_libraries(twig_tests
    twig
    gtest_main
)

# Include directories
target_include_directories(twig_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Register tests
add_test(NAME TwigTests COMMAND twig_tests)

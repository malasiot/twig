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

include_directories(${ICU_INCLUDE_DIRS})
# Add test executable
add_executable(twig_tests
    test_tags.cpp
    test_functions.cpp
    test_expressions.cpp
    test_translation.cpp
)

# Link against the main library and gtest
target_link_libraries(twig_tests
    twig
    gtest_main
    ICU::i18n
    ICU::uc
    variant
)

# Include directories
target_include_directories(twig_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Register tests
add_test(NAME TwigTests COMMAND twig_tests)

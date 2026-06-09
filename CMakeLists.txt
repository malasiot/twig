cmake_minimum_required(VERSION 3.10)

project (twig)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
  list(APPEND CMAKE_PREFIX_PATH
    "/opt/homebrew/opt/icu4c"
    "/usr/local/opt/icu4c"
  )
endif()



find_package(ICU REQUIRED COMPONENTS i18n uc)

if(NOT TARGET variant)
    find_package(variant QUIET)
endif()

if(NOT variant_FOUND)
    message(STATUS "variant library not found on system. Using local submodule.")
    
    # Ensure the submodule directory contains a valid build file
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/variant/CMakeLists.txt")
    MESSAGE("ok here")
        SET( VARIANT_BUILD_TESTS Off )
        add_subdirectory(libs/variant)
    else()
        # Fallback for header-only submodules missing their own CMake files
        message(STATUS "Creating an interface target directly for header-only variant.")
        add_library(variant INTERFACE)
        target_include_directories(variant INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libs/variant/include")
    endif()
endif()

enable_testing()

MESSAGE(${ICU_INCLUDE_DIRS})
include_directories(include )

add_library(twig SHARED
    src/parser.cpp
    src/parser.hpp
    src/ast.cpp
    src/ast.hpp
    src/functions.cpp
    src/renderer.cpp
    src/loader.cpp
    src/date_helpers.cpp
    src/format.cpp
    src/translator.cpp

    src/forms/form_builder.cpp
    src/forms/normalizers.cpp
    src/forms/validators.cpp

#    ${CMAKE_CURRENT_SOURCE_DIR}/include/twig/variant.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/twig/functions.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/twig/exceptions.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/twig/renderer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/twig/date_helpers.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/twig/translator.hpp
)

target_link_libraries(twig PRIVATE ICU::i18n ICU::uc variant::variant)

add_library(twig::twig ALIAS twig)

target_include_directories(twig INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Include tests subdirectory
add_subdirectory(tests)

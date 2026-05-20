cmake_minimum_required(VERSION 3.10)

project (twig)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ICU REQUIRED COMPONENTS i18n uc)

enable_testing()

include_directories(include)

add_library(twig SHARED
    src/parser.cpp
    src/parser.hpp
    src/ast.cpp
    src/ast.hpp
    src/functions.cpp
    src/renderer.cpp
    src/loader.cpp
    src/json_parser.cpp
    src/date_helpers.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/twig/variant.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/twig/functions.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/twig/exceptions.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/twig/renderer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/twig/date_helpers.hpp
    
)

target_link_libraries(twig PRIVATE ICU::i18n ICU::uc)

add_executable(twig_exec main.cpp)
target_link_libraries(twig_exec twig)

# Include tests subdirectory
add_subdirectory(tests)

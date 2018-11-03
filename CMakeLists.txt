cmake_minimum_required(VERSION 2.8)

project (twig)

add_definitions( -std=c++11 )

include_directories(include)

add_library(twig SHARED
    src/parser.cpp
    src/parser.hpp
    src/ast.cpp
    src/ast.hpp
    src/functions.cpp
    src/renderer.cpp
    src/loader.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/include/twig/variant.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/twig/functions.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/twig/exceptions.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/twig/renderer.hpp
)

target_link_libraries(twig)

add_executable(twig_exec main.cpp)
target_link_libraries(twig_exec twig)

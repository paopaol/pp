cmake_minimum_required(VERSION 2.8)

project(pp)

set(
    CMAKE_BUILD_TYPE
    Debug
)

add_compile_options(-std=c++11)
include_directories(include)

add_subdirectory(src)
add_subdirectory(test)

add_library(pp)
target_link_libraries(pp io errors time bytes)

enable_testing()

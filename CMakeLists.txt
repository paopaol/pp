cmake_minimum_required(VERSION 2.8e

project(pp)


include_directories(include)

add_subdirectory(src)
add_subdirectory(test)

add_library(pp)
target_link_libraries(pp io errors time bytes strings fmt)

enable_testing()

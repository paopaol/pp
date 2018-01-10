cmake_minimum_required(VERSION 2.8)

project(pp)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(IS_WIN TRUE)
  add_definitions(-DWIN32)
endif()

add_compile_options(-std=c++11)
enable_testing()
include_directories(include)

# auto create libcurl dir in ${PROJECT_BINARY_DIR}
link_directories(${PROJECT_BINARY_DIR}/libcurl/lib)
include_directories(${PROJECT_BINARY_DIR}/libcurl/include)

add_subdirectory(src)

add_library(pp src/pp.cpp)
target_link_libraries(pp io errors time bytes strings fmt)


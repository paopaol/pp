cmake_minimum_required(VERSION 2.8)

project(pp)


include_directories(include)

# auto create libcurl dir in ${PROJECT_BINARY_DIR}
link_directories(${PROJECT_BINARY_DIR}/libcurl/lib)
include_directories(${PROJECT_BINARY_DIR}/libcurl/include)

add_subdirectory(src)
add_subdirectory(test)

add_library(pp src/pp.cpp)
target_link_libraries(pp io errors time bytes strings fmt http)

enable_testing()

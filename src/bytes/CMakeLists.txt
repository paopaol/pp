add_library(bytes buffer.cpp)

add_executable(buffer_test buffer_test.cpp)
target_link_libraries(buffer_test pp)
add_test(
    buffer_test_example
    buffer_test
)

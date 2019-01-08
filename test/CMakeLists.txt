cmake_minimum_required(VERSION 3.5)

add_executable(test_io_event_loop test_io_event_loop.cpp test_time.cpp)
target_link_libraries(test_io_event_loop pp gtest_main)
add_test(NAME test_io_event_loop COMMAND test_io_event_loop)
GTEST_ADD_TESTS(test_io_event_loop "" test_io_event_loop.cpp)

cmake_minimum_required(VERSION 3.5)

add_executable(test_io_event_loop test_io_event_loop.cpp test_time.cpp)
target_link_libraries(test_io_event_loop pp gtest_main)
add_test(NAME test_io_event_loop COMMAND test_io_event_loop)
GTEST_ADD_TESTS(test_io_event_loop "" test_io_event_loop.cpp)

add_executable(test_tcp_connector test_tcp_connector.cpp)
target_link_libraries(test_tcp_connector pp gtest_main)
add_test(NAME test_tcp_connector COMMAND test_tcp_connector)
GTEST_ADD_TESTS(test_tcp_connector "" test_tcp_connector.cpp)

add_executable(test_signal_slot test_signal_slot.cpp)
target_link_libraries(test_signal_slot pp gtest_main)
add_test(NAME test_signal_slot COMMAND test_signal_slot)
GTEST_ADD_TESTS(test_signal_slot "" test_signal_slot.cpp)

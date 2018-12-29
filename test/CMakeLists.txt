cmake_minimum_required(VERSION 3.5)

configure_file(CMakeLists.txt.in ${CMAKE_CURRENT_BINARY_DIR}/googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" . 
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "cmake step googletest falied: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
 RESULT_VARIABLE result
 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
if(result)
 message(FATAL_ERROR "build setp for googletest failed: ${result}")
endif()

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
 EXCLUDE_FROM_ALL)

if(CMAKE_VERSION VERSION_LESS 2.8.11)
 include_directories("${gtest_SOURCE_DIR}/include")
 endif()

 include(GoogleTest)

add_executable(main main.cpp)
target_link_libraries(main pp gtest_main)
add_executable(tcp_client tcp_client.cpp)
target_link_libraries(tcp_client pp)
add_executable(http_client http_client.cpp)
target_link_libraries(http_client pp)

add_executable(test_io_event_loop test_io_event_loop.cpp test_time.cpp)
target_link_libraries(test_io_event_loop pp gtest_main)
add_test(NAME test_io_event_loop COMMAND test_io_event_loop)
GTEST_ADD_TESTS(test_io_event_loop "" test_io_event_loop.cpp)

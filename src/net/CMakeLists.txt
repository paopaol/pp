set(src_files
    )
if (IS_WIN)
    set(src_files ${src_files}
		windows/net_listener_iocp.cpp
		windows/net_listener_iocp.h
		windows/socket_win.cpp
        )
endif()

include_directories(${CMAKE_SOURCE_DIR}/src/io/windows)



add_library(net ${src_files})

if(NOT DISABLED_TEST)
    #add_executable(event_loop_test event_loop_test.cpp)
    #target_link_libraries(event_loop_test pp ws2_32)
endif()

set(src_files
    )
if (IS_WIN)
    set(src_files ${src_files}
		windows/net_iocp_accpeter.cpp
		windows/net_iocp_accpeter.h
		windows/socket_win.cpp
		windows/net_conn_win.cpp
        )
endif()

include_directories(${CMAKE_SOURCE_DIR}/src/io/windows)



add_library(net ${src_files})

if(NOT DISABLED_TEST)
    #add_executable(event_loop_test event_loop_test.cpp)
    #target_link_libraries(event_loop_test pp ws2_32)
endif()

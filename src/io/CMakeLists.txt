set(src_files
   io.cpp
   thread_local_storage.h
   event_loop.cpp
   event.cpp
    )
if (IS_WIN)
    set(src_files ${src_files}
        windows/thread_local_storage_win.cpp
        windows/event_listener_iocp.cpp
        windows/event_listener_iocp.h
		#windows/conn_win.h
		windows/event_iocp.h
		windows/event_iocp.cpp
        )
endif()

include_directories(.)
include_directories(${CMAKE_SOURCE_DIR}/src/net/windows)
include_directories(${CMAKE_SOURCE_DIR}/src/io/windows)
add_library(io ${src_files})

if(NOT DISABLED_TEST)
    add_executable(event_loop_test event_loop_test.cpp)
    target_link_libraries(event_loop_test pp ws2_32)
endif()

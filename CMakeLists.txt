cmake_minimum_required(VERSION 2.8)

project(pp)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(IS_WIN TRUE)
  add_definitions(-DWIN32)
endif()

# add_compile_options(-std=c++11)
if(NOT DISABLED_TEST)
        enable_testing()
endif()
include_directories(include)

# auto create libcurl dir in ${PROJECT_BINARY_DIR}
#link_directories(${PROJECT_BINARY_DIR}/libcurl/lib)
#include_directories(${PROJECT_BINARY_DIR}/libcurl/include)

#add_subdirectory(src)

if(IS_WIN)
    add_definitions(-DWIN32_LEAN_AND_MEAN )
    add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS )
endif()

include_directories(
    src/io
    src/net
    )
####io
set(io-src-list
   src/io/io.cpp
   src/io/thread_local_storage.h
   src/io/event_loop.cpp
   src/io/event_fd.cpp
)

if(IS_WIN)
    set(io-src-list 
        ${io-src-list}
        src/io/windows/thread_local_storage_win.cpp
        src/io/windows/event_iocp_poller.cpp
        src/io/windows/event_iocp_poller.h
		src/io/windows/event_fd_iocp.h
		src/io/windows/event_fd_iocp.cpp
        src/io/windows/overlapped_pipe.cpp
        src/io/windows/overlapped_pipe.h
        )
endif()

####net
if(IS_WIN)
    set(net-src-list
        src/net/windows/socket_conn_win.cpp
        src/net/windows/socket_iocp_accpeter.cpp
        src/net/windows/socket_iocp_accpeter.h
        src/net/windows/socket_win.cpp
    )
endif()

###system
if(IS_WIN)
    set(system-src-list
        src/system/windows/thread_win.cpp
        src/system/windows/thread_call_once_win.cpp
        src/system/windows/condition_win.cpp
        src/system/windows/mutex_win.cpp
    )
endif()



###fmt
set(fmt-src-list
        src/fmt/format.cpp
    )

###bytes
set(bytes-src-list
        src/bytes/buffer.cpp
    )
    
    
###time
set(time-src-list        
        src/time/time.cpp
    )
if(IS_WIN)
    set(time-src-list
        ${time-src-list}
        src/time/time_win.cpp
    )
endif()



add_library(pp ${io-src-list} ${net-src-list} ${fmt-src-list} ${system-src-list} ${bytes-src-list} ${time-src-list})
target_link_libraries(pp ws2_32)

add_subdirectory(test)


cmake_minimum_required(VERSION 3.3)

project(pp)


set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON "export symbols")

set(PP_BUILD_INSTALL FALSE CACHE BOOL "install library or not")
set(PP_DISABLED_TEST FALSE CACHE BOOL "build uint test or not")
set(PP_DISABLE_EXAMPLE FALSE CACHE BOOL "build example or not")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(IS_WIN TRUE)
  add_definitions(-DWIN32)
endif()

# add_compile_options(-std=c++11)
if(NOT PP_DISABLED_TEST)
  enable_testing()
endif()
include_directories(include)
include_directories(src/system)
include_directories(src/time)
include_directories(src/errors)

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
  #  src/io/io_thread_local_storage.h
  src/io/io_event_loop.cpp
  src/io/io_event_fd.cpp
  )


if(IS_WIN)
  set(io-src-list
    ${io-src-list}
    # src/io/windows/thread_local_storage_win.cpp
    src/io/windows/io_win_iocp_event_fd.cpp
    src/io/windows/io_win_iocp_event_fd.h
    src/io/windows/io_win_iocp_poller.cpp
    src/io/windows/io_win_iocp_poller.h
    #src/io/windows/io_win_overlapped_pipe.cpp
    #src/io/windows/io_win_overlapped_pipe.h

    #src/io/windows/io_win_iocp_pipe_event_fd.cpp
    #src/io/windows/io_win_iocp_pipe_event_fd.h
    #src/io/windows/io_win_iocp_tcp_event_fd.cpp
    #src/io/windows/io_win_iocp_tcp_event_fd.h
    )
endif()

include_directories(http-parser)
####net
set(net-src-list
  src/net/tcp_server.cpp
  src/net/tcp_client.cpp
  src/net/http/http_client.cpp
  src/net/http/http_conn_ctx.cpp
  src/net/http/http_conn_ctx.h
  http-parser/http_parser.c
  http-parser/http_parser.h
  )
if(IS_WIN)
  set(net-src-list
    ${net-src-list}
    src/net/windows/net_win_tcp_conn.cpp
    src/net/windows/net_win_iocp_tcp_accpeter.cpp
    src/net/windows/net_win_iocp_tcp_accpeter.h
    src/net/windows/net_win_iocp_tcp_connector.cpp
    src/net/windows/net_win_socket.cpp
    )
endif()

###system
if(IS_WIN)
  set(system-src-list
    src/system/windows/sys_win_condition.cpp
    src/system/windows/sys_win_mutex.cpp
    src/system/windows/sys_win_thread_call_once.cpp
    src/system/windows/sys_win_thread.cpp
    src/system/windows/sys_win_thread_local_storage.cpp
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
  src/time/timer.cpp
  src/time/timer_queue.cpp
  )
if(IS_WIN)
  set(time-src-list
    ${time-src-list}
    src/time/time_win.cpp
    )
endif()

###errors
set(errors-src-list
  src/errors/errors.cpp
  )
if(IS_WIN)
  set(errors-src-list
    ${errors-src-list}
    src/errors/windows/errors_windows.cpp
    )
endif()



add_library(pp ${io-src-list}
  ${net-src-list}
  ${fmt-src-list}
  ${system-src-list}
  ${bytes-src-list}
  ${time-src-list}
  ${errors-src-list})
target_link_libraries(pp PUBLIC ws2_32)
if(WIN32)
  target_compile_definitions(pp PRIVATE _SCL_SECURE_NO_WARNINGS _CRT_SECURE_NO_WARNINGS)
endif()

if(NOT PP_DISABLED_TEST)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  add_subdirectory(googletest)
  if(CMAKE_VERSION VERSION_LESS 2.8.11)
    include_directories("${gtest_SOURCE_DIR}/include")
  endif()
  include(GoogleTest)

  add_subdirectory(test)
endif()

if(NOT PP_DISABLE_EXAMPLES)
  add_subdirectory(example)
endif()


if(PP_BUILD_INSTALL)
    if(CMAKE_SIZEOF_VOID_P STREQUAL 4)
        set(platform x86)
    else()
        set(platform x64)
    endif()

    install(TARGETS pp
        CONFIGURATIONS RelWithDebInfo
        RUNTIME DESTINATION bin/static.${platform}.RelWithDebInfo
        ARCHIVE DESTINATION lib/static.${platform}.RelWithDebInfo
        LIBRARY DESTINATION lib/static.${platform}.RelWithDebInfo
        )
    install(TARGETS pp
        CONFIGURATIONS Release
        RUNTIME DESTINATION bin/static.${platform}.Release
        ARCHIVE DESTINATION lib/static.${platform}.Release
        LIBRARY DESTINATION lib/static.${platform}.Release
        )

    install(TARGETS pp
        CONFIGURATIONS Debug
        RUNTIME DESTINATION bin/static.${platform}.Debug
        ARCHIVE DESTINATION lib/static.${platform}.Debug
        LIBRARY DESTINATION lib/static.${platform}.Debug
        )
    install(DIRECTORY include/
        DESTINATION include
        )
endif()

set(src_files
   io.cpp
   thread_local_storage.h
   event_loop.cpp
    )
if (IS_WIN)
    set(src_files ${src_files} thread_local_storage_win.cpp)
endif()
add_library(io ${src_files})

if(NOT DISABLED_TEST)
    add_executable(event_loop_test event_loop_test.cpp)
    target_link_libraries(event_loop_test pp)
endif()

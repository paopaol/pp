set(src_files
   io.cpp
   thread_local_storage.h
   event_loop.cpp
    )
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(src_files ${src_files} thread_local_storage_win.cpp)
endif()
add_library(io ${src_files})

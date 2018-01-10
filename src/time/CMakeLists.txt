set(time_src_files time.cpp)
if(IS_WIN)
  list(APPEND time_src_files
    time_win.cpp
    time_win.h
    )
  include_directories(.)
endif()

add_library(time ${time_src_files})
if(IS_WIN)
  target_compile_options(time PRIVATE /wd4293)
endif()


add_executable(time_test time_test.cpp)
target_link_libraries(time_test pp)
add_test(
  time_test
  time_test
  )

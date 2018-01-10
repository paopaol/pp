add_library(strings strings.cpp)

add_executable(strings_test strings_test.cpp)
target_link_libraries(strings_test pp)
add_test(
  strings_test
  strings_test
  )

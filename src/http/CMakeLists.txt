
include(ExternalProject)

message(STATUS "build type:" $<CONFIG>)
ExternalProject_Add(
  libcurl_project
  GIT_REPOSITORY "https://github.com/curl/curl"
  GIT_TAG "curl-7_56_0"
  CMAKE_COMMAND cmake
  CMAKE_ARGS -DBUILD_TESING=OFF
                -DBUILD_CURL_EXE=OFF
                -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}/libcurl
                -DCMAKE_BUILD_TYPE=$<CONFIG>
  BUILD_COMMAND make
  BUILD_IN_SOURCE 0
  INSTALL_COMMAND make install
  )


add_library(http client.cpp request.cpp)
target_link_libraries(http errors strings curl idn rt crypto ssl pthread z)
add_dependencies(
  http
  libcurl_project
  )


add_executable(http_test http_test.cpp)
target_link_libraries(http_test pp)
add_test(
  http_test
  http_test
  )

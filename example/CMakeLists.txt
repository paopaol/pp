cmake_minimum_required(VERSION 3.5)

add_executable(main main.cpp)
target_link_libraries(main pp)
add_executable(tcp_client tcp_client.cpp)
target_link_libraries(tcp_client pp)
add_executable(http_client http_client.cpp)
target_link_libraries(http_client pp)

add_executable(bxel bxel/bxel.cpp bxel/bxel.h)
target_link_libraries(bxel pp)

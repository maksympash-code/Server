cmake_minimum_required(VERSION 3.20)
project(Server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-subsystem,console")

add_executable(server server.cpp)
target_link_libraries(server ws2_32)

add_executable(client client.cpp)
target_link_libraries(client ws2_32)
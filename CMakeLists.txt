cmake_minimum_required(VERSION 3.20)
project(Server)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-subsystem,console")

# Для сервера
add_executable(server server.cpp)
target_link_libraries(server ws2_32)

# Для клієнта
add_executable(client client.cpp)
target_link_libraries(client ws2_32)

# Для тесту клієнта та серверу
add_executable(test test.cpp)
target_link_libraries(test ws2_32)
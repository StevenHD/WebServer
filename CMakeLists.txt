cmake_minimum_required(VERSION 3.17)
project(WebServer)

set(CMAKE_CXX_STANDARD 11)

add_executable(WebServer main.cpp main_epollET.cpp non_block_et_epoll.cpp)
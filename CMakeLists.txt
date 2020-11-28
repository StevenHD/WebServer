cmake_minimum_required(VERSION 3.17)
project(WebServer)

set(CMAKE_CXX_STANDARD 11)

add_executable(WebServer test/main.cpp test/main_epollET.cpp test/non_block_et_epoll.cpp test/thpool.h test/thpool.cpp
        WebServer/locker.h
        WebServer/locker.cpp
        WebServer/task.h
        WebServer/task.cpp
        WebServer/threadpool.cpp
        WebServer/threadpool.h
        WebServer/WebServer.h
        WebServer/WebServer.cpp
        WebServer/main.cpp
        )
cmake_minimum_required(VERSION 3.17)
project(WebServer)

set(CMAKE_CXX_STANDARD 11)

add_executable(
        WebServer
        WebServer/locker.h
        WebServer/locker.cpp
        WebServer/task.h
        WebServer/task.cpp
        WebServer/threadpool.cpp
        WebServer/threadpool.h
        WebServer/WebServer.h
        WebServer/WebServer.cpp
        WebServer/main.cpp
        WebServer/List_Timer.cpp WebServer/List_Timer.h fork.cpp)
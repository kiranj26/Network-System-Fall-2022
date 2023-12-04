cmake_minimum_required(VERSION 3.16)

project(dfs LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

add_executable(dfs EventLoop.cpp EventLoop.h main.cpp DFS.cpp DFS.h Connection.cpp Connection.h)
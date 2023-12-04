cmake_minimum_required(VERSION 3.16)

project(dfs LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(OpenSSL REQUIRED)

add_executable(dfc main.cpp DFC.h DFC.cpp ListCollector.h ListCollector.cpp conn.cpp conn.h MultipleGetRequest.cpp MultipleGetRequest.h
        PutRequest.cpp PutRequest.h ListRequest.cpp ListRequest.h MkdirRequest.cpp MkdirRequest.h)

target_link_libraries(dfc PRIVATE OpenSSL::SSL)
cmake_minimum_required(VERSION 3.23)
project(server C)

set(CMAKE_C_STANDARD 99)

add_executable(server server.c threadpool.c)

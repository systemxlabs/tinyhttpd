cmake_minimum_required(VERSION 3.20)
project(tinyhttpd C)

set(CMAKE_C_STANDARD 11)

add_executable(tinyhttpd src/server.c src/util.c)
target_include_directories(tinyhttpd PRIVATE include)

add_executable(example_client client/example_client.c)
cmake_minimum_required(VERSION 3.20)
project(tinyhttpd C)

set(CMAKE_C_STANDARD 11)

aux_source_directory(src SRC_FILES)
add_executable(tinyhttpd ${SRC_FILES})
target_include_directories(tinyhttpd PRIVATE include)
# TODO 临时方案
configure_file(cgi-bin/student.py ${CMAKE_CURRENT_BINARY_DIR}/cgi-bin/student.py COPYONLY)

add_executable(example_client client/example_client.c)
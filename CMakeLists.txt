cmake_minimum_required(VERSION 3.20)
project(tinyhttpd C)

set(CMAKE_C_STANDARD 11)

# 参考 https://stackoverflow.com/questions/66829315/how-to-use-cmake-fetchcontent-to-link-openssl
#set(OPENSSL_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/openssl-src) # default path by CMake
#set(OPENSSL_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/openssl)
#set(OPENSSL_INCLUDE_DIR ${OPENSSL_INSTALL_DIR}/include)
#set(OPENSSL_CONFIGURE_COMMAND ${OPENSSL_SOURCE_DIR}/config)
#message(STATUS "OpenSSL: ${OPENSSL_SOURCE_DIR}, ${OPENSSL_INSTALL_DIR}, ${OPENSSL_INCLUDE_DIR}, ${OPENSSL_CONFIGURE_COMMAND}")
#include(ExternalProject)
#ExternalProject_Add(OpenSSL
#        SOURCE_DIR ${OPENSSL_SOURCE_DIR}
#        GIT_REPOSITORY https://github.com/openssl/openssl.git
#        GIT_TAG openssl-3.0.1
#        USES_TERMINAL_DOWNLOAD TRUE
#        CONFIGURE_COMMAND sh ${OPENSSL_CONFIGURE_COMMAND} --prefix=${OPENSSL_INSTALL_DIR} --openssldir=${OPENSSL_INSTALL_DIR}
#        BUILD_COMMAND make
#        TEST_COMMAND ""
#        INSTALL_COMMAND make install
#        INSTALL_DIR ${OPENSSL_INSTALL_DIR}
#)

# We cannot use find_library because ExternalProject_Add() is performed at build time.
# And to please the property INTERFACE_INCLUDE_DIRECTORIES,
# we make the include directory in advance.
#file(MAKE_DIRECTORY ${OPENSSL_INCLUDE_DIR})
#add_library(OpenSSL::SSL STATIC IMPORTED GLOBAL)
#set_property(TARGET OpenSSL::SSL PROPERTY IMPORTED_LOCATION ${OPENSSL_INSTALL_DIR}/lib/libssl.a)
#set_property(TARGET OpenSSL::SSL PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OPENSSL_INCLUDE_DIR})
#add_dependencies(OpenSSL::SSL OpenSSL)
#
#add_library(OpenSSL::Crypto STATIC IMPORTED GLOBAL)
#set_property(TARGET OpenSSL::Crypto PROPERTY IMPORTED_LOCATION ${OPENSSL_INSTALL_DIR}/lib/libcrypto.a)
#set_property(TARGET OpenSSL::Crypto PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OPENSSL_INCLUDE_DIR})
#add_dependencies(OpenSSL::Crypto OpenSSL)

# brew install openssl 安装openssl
set(OPENSSL_ROOT_DIR /opt/homebrew/opt/openssl@1.1/)  # brew --prefix openssl
find_package(OpenSSL REQUIRED)

add_library(tls src/tls.c)
target_include_directories(tls PUBLIC include)
target_include_directories(tls PUBLIC ${OPENSSL_INCLUDE_DIR})
target_link_libraries(tls PUBLIC ${OPENSSL_LIBRARIES})


aux_source_directory(src SRC_FILES)
add_executable(tinyhttpd ${SRC_FILES})
target_include_directories(tinyhttpd PRIVATE include)
target_link_libraries(tinyhttpd PRIVATE tls)

# TODO 临时方案
configure_file(cgi-bin/student.py ${CMAKE_CURRENT_BINARY_DIR}/cgi-bin/student.py COPYONLY)

add_executable(example_client client/example_client.c)
target_include_directories(example_client PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(example_client PRIVATE ${OPENSSL_LIBRARIES})
#target_link_libraries(example_client PRIVATE OpenSSL::SSL OpenSSL::Crypto)
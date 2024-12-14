cmake_minimum_required(VERSION 3.10)

project(SimpleChat)

include_directories(BEFORE "${CMAKE_SOURCE_DIR}/c_include")

file(GLOB SOURCES main.c "${CMAKE_SOURCE_DIR}/c_include/simple_tcp_connect.h")

add_executable(${PROJECT_NAME} ${SOURCES})
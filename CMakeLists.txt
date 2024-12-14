cmake_minimum_required(VERSION 3.10)

project(SimpleChat)

include_directories(BEFORE "${PROJECT_SOURCE_DIR}/c_include")

file(GLOB SOURCES main.c "${PROJECT_SOURCE_DIR}/c_include/*.h" "${PROJECT_SOURCE_DIR}/c_src/*.c")

add_executable(${PROJECT_NAME} ${SOURCES})
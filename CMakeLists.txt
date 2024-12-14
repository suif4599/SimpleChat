cmake_minimum_required(VERSION 3.10)

project(SimpleChat)

file(GLOB SOURCES *.c *.h)

add_executable(${PROJECT_NAME} ${SOURCES})
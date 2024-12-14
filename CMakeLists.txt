cmake_minimum_required(VERSION 3.10)

project(SimpleChat)

include_directories(BEFORE "${PROJECT_SOURCE_DIR}/c_include")

file(GLOB SOURCES main.c "${PROJECT_SOURCE_DIR}/c_src/*.c" "${PROJECT_SOURCE_DIR}/c_src/*.h")

add_executable(${PROJECT_NAME} ${SOURCES})

if (WIN32)
    target_link_libraries(${PROJECT_NAME} Ws2_32)
endif()
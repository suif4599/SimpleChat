cmake_minimum_required(VERSION 3.10)

# Set the project name
project(SimpleChat)

# Add all c files in the home directory
file(GLOB SOURCES "*.c")

# Add the executable
add_executable(SimpleChat ${SOURCES})
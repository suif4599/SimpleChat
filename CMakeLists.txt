cmake_minimum_required(VERSION 3.10)

# Set the project name
project(SimpleChat)

# Add all c files in the home directory and all subdirectories
file(GLOB_RECURSE SOURCES "*.c")

# Remove all files in the build directory
file(GLOB_RECURSE EXCLUDED_FILES "build/*.c")
foreach(excluded_file IN LISTS EXCLUDED_FILES)
    list(REMOVE_ITEM SOURCES ${excluded_file})
endforeach()

# Add the executable
# message("SOURCES: ${SOURCES}")
add_executable(SimpleChat ${SOURCES})
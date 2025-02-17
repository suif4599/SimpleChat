cmake_minimum_required(VERSION 3.10)


set(CMAKE_C_COMPILER "C:\\msys64\\mingw64\\bin\\gcc.exe")

if(MSVC)
    message(FATAL_ERROR "GCC is required to build this project")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

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
add_executable(SimpleChat ${SOURCES})

# Add libraries
if(WIN32)
    target_link_libraries(SimpleChat PRIVATE Ws2_32)
    add_compile_definitions(_WINSOCK_DEPRECATED_NO_WARNINGS)
endif()



# Example CMake command line to create project build files:
#
# *** Windows ***
# cmake -G "Visual Studio 17 2022" -A Win32 -B Build -S .
#
# *** Linux ***
# cmake -G "Unix Makefiles" -B Build -S .

# Specify the minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and language (C++)
project(StateMachine VERSION 1.0 LANGUAGES CXX)

# The library (src/, the Arduino library layout), the desktop demo
# (examples/desktop) and the copies of Allocator and xallocator (allocator/)
file(GLOB SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp" "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/examples/desktop/*.cpp" "${CMAKE_SOURCE_DIR}/examples/desktop/*.h"
    "${CMAKE_SOURCE_DIR}/allocator/*.cpp" "${CMAKE_SOURCE_DIR}/allocator/*.h")

# Add an executable target
add_executable(StateMachineApp ${SOURCES})
target_include_directories(StateMachineApp PRIVATE
    ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/examples/desktop ${CMAKE_SOURCE_DIR}/allocator)


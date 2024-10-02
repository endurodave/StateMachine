# Example CMake command line to create project build files:
#
# *** Windows ***
# cmake -G "Visual Studio 17 2022" -A Win32 -B ../StateMachineBuild -S .
#
# *** Linux ***
# cmake -G "Unix Makefiles" -B ../StateMachineBuild -S .

# Specify the minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and language (C or C++)
project(StateMachine VERSION 1.0 LANGUAGES CXX)

# Collect all .cpp source files in the current directory
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/*.cpp")

# Add an executable target
add_executable(StateMachineApp ${SOURCES})




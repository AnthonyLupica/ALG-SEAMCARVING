# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(SeamCarving)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 11)

# Set the source files for the project
set(SOURCE_FILES seamCarving.cpp)

# Add an executable target
add_executable(a ${SOURCE_FILES})
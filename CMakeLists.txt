project(VulkanTest)
cmake_minimum_required(VERSION 3.6)

find_package(glfw3 3.2 REQUIRED)

# Support C++1y
add_definitions(-std=c++1y)

# Change binary directory
# keeps things a bit cleaner
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Binaries)

add_subdirectory(src)

cmake_minimum_required(VERSION 3.10)
project(ssbs)

set(ssbs_VERSION_MAJOR 1)
set(ssbs_VERSION_MINOR 0)
set(CMAKE_CXX_STANDARD, 17)

include_directories(src)
add_executable(ssbs src/main.cpp src/data.cpp src/display.cpp)
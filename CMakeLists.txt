cmake_minimum_required(VERSION 3.22)
project(CHIP_8)

set(CMAKE_CXX_STANDARD 20)



add_executable(CHIP_8 main.cpp Chip8.hpp Chip8.cpp)
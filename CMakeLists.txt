cmake_minimum_required(VERSION 3.20)
project(HW_OS1)

set(CMAKE_CXX_STANDARD 11)

add_executable(HW_OS1 Commands.cpp Commands.h signals.cpp signals.h smash.cpp)

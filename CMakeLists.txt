cmake_minimum_required(VERSION 3.13)
project(Demo C)
set(CMAKE_C_STANDARD 99)
# Link to needed libraries
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wconversion -Wextra -pthread")

include_directories(.)

add_executable(Demo demo.c)

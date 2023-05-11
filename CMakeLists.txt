cmake_minimum_required(VERSION 3.13)
project(OSMP C)

#set to c99 and not gnu99
set(CMAKE_C_STANDARD 99)

add_executable(osmp ./OSMP/OSMPStarter/osmprun.c ./OSMP/OSMPLib/osmplib.c ./OSMP/OSMPLib/osmplib.h ./OSMP/OSMPStarter/osmprun.h ./OSMP/OSMP.h)

target_link_libraries(osmp rt -lpthread)
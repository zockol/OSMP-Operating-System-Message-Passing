cmake_minimum_required(VERSION 3.21)

project(
        ModernCMakeExample
        VERSION 1.0
        LANGUAGES C)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_library(osmplib ./OSMP/OSMPLib/osmplib.c)

add_executable(osmp ./OSMP/OSMPStarter/osmprun.c)
add_executable(osmpIsendIrecieve ./OSMP/OSMPExecutable/osmpIsendIrecieve.c)
add_executable(osmpBarrierTest ./OSMP/OSMPExecutable/osmpbarriertest.c)
add_executable(osmpSendRecieve ./OSMP/OSMPExecutable/osmpsendrecieve.c)
add_executable(osmpBroadcast ./OSMP/OSMPExecutable/osmpbcast.c)
add_executable(osmpexecutable ./OSMP/OSMPExecutable/osmpexecutable.c)

target_link_libraries(osmp PRIVATE osmplib)
target_link_libraries(osmpIsendIrecieve PRIVATE osmplib)
target_link_libraries(osmpBarrierTest PRIVATE osmplib)
target_link_libraries(osmpSendRecieve PRIVATE osmplib)
target_link_libraries(osmpBroadcast PRIVATE osmplib)
target_link_libraries(osmpexecutable PRIVATE osmplib)
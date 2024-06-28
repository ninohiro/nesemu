cmake_minimum_required(VERSION 3.5)
project(nesemu)

find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)

add_executable(nesemu nesemu.cpp)
add_library(ines OBJECT ines.cpp)
add_library(nes OBJECT nes.cpp nes_ppu.cpp)

target_link_libraries(nesemu PRIVATE SDL2::SDL2main)
target_link_libraries(nesemu PRIVATE SDL2::SDL2)
target_link_libraries(nesemu PRIVATE ines)
target_link_libraries(nesemu PRIVATE nes)

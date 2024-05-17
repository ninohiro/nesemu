cmake_minimum_required(VERSION 3.5)
project(nesemu)

find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)

add_executable(nesemu nesemu.cpp)
add_library(ines OBJECT ines.cpp)

target_link_libraries(nesemu PRIVATE SDL2::SDL2main)
target_link_libraries(nesemu PRIVATE SDL2::SDL2)
target_link_libraries(nesemu PRIVATE ines)

enable_testing()
add_executable(test_ines test_ines.cpp)
target_link_libraries(test_ines PRIVATE ines)

add_test(NAME test_ines COMMAND test_ines)

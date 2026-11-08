cmake_minimum_required(VERSION 3.28)
project(modmask)

find_package(X11 REQUIRED)
add_executable(simple simple.c)
target_link_libraries(simple PRIVATE X11::X11)

add_subdirectory(st)
add_subdirectory(tools)


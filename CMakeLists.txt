cmake_minimum_required(VERSION 3.28)
project(modmask)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "")

# this produces the target `callgraph`, I need it for custom commands
add_subdirectory(tools)


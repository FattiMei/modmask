find_package(LLVM REQUIRED)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})

# use the same C++ standard as LLVM
set(CMAKE_CXX_STANDARD 17 CACHE STRING "")

# I'm using the `add_llvm_executable` macro, but I should know what it does
include(AddLLVM)
add_llvm_executable(callgraph callgraph.cpp)


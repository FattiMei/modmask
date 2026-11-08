find_package(LLVM REQUIRED)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

# normally I would use the `add_llvm_executable` macro, but I want to try a vanilla build
set(CALLGRAPH_TOOL "callgraph")
add_executable(${CALLGRAPH_TOOL} callgraph.cpp)
target_include_directories(${CALLGRAPH_TOOL} PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(${CALLGRAPH_TOOL} PRIVATE LLVM)


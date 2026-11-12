find_package(LLVM REQUIRED)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

set(CMAKE_CXX_STANDARD 17 CACHE STRING "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(callgraph_local "callgraph")
set(callgraph ${callgraph_local} PARENT_SCOPE)

# normally I would use the `add_llvm_executable` macro, but I want to try a vanilla build
add_executable(${callgraph_local} "${CMAKE_CURRENT_SOURCE_DIR}/callgraph.cpp")
target_include_directories(${callgraph_local} PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(${callgraph_local} PRIVATE LLVM)


find_program(clang
	NAMES "clang"
	PATHS ${LLVM_TOOLS_BINARY_DIR}
	NO_DEFAULT_PATH
)
if (clang)
	message(STATUS "Found clang at ${clang}")
else()
	message(WARNING "A compatible clang version was not found, can't run reachability analysis")
endif()


function(add_analysis analysis_name target)
	if (NOT clang)
		message(ERROR "Can't run ${analysis_name} as clang is not found")
	endif()

	get_target_property(target_sources ${target} SOURCES)
	set(includes "$<LIST:TRANSFORM,$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>,PREPEND,-I>")
	set(definitions "$<LIST:TRANSFORM,$<TARGET_PROPERTY:${target},COMPILE_DEFINITIONS>,PREPEND,-D>")

	# tidy: the ir files must be placed in an appropriate directory
	set(ir_dir "${CMAKE_CURRENT_BINARY_DIR}/${analysis_name}")
	make_directory(${ir_dir})
	set(ir_files "")

	foreach(src ${target_sources})
		# these are sources of a target => they exist
		# I still have problems about paths
		get_filename_component(absolute_path ${src} ABSOLUTE)
		get_filename_component(name_we ${absolute_path} NAME_WE)
		set(output_ir_file "${ir_dir}/${name_we}.ll")

		add_custom_command(
			OUTPUT ${output_ir_file}
			COMMAND ${clang} -g -S -emit-llvm ${includes} ${definitions} -o ${output_ir_file} ${absolute_path}
			COMMAND_EXPAND_LISTS
			VERBATIM
			DEPENDS ${absolute_path}
			COMMENT "Generating ${output_ir_file}"
		)

		list(APPEND ir_files ${output_ir_file})
	endforeach()

	# any variable dereference uses the variable from the current scope
	add_custom_target(
		${analysis_name}
		COMMAND ${callgraph} ${ir_files}
		DEPENDS ${callgraph} ${ir_files}
	)
endfunction()


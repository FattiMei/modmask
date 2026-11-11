find_package(LLVM REQUIRED)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")


set(CMAKE_CXX_STANDARD 17 CACHE STRING "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")


# normally I would use the `add_llvm_executable` macro, but I want to try a vanilla build
set(callgraph "callgraph")
add_executable(${callgraph} "${CMAKE_CURRENT_SOURCE_DIR}/callgraph.cpp")
target_include_directories(${callgraph} PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(${callgraph} PRIVATE LLVM)


find_program(clang
	NAMES "clang"
	PATHS ${LLVM_TOOLS_BINARY_DIR}
	NO_DEFAULT_PATH
)

if (clang)
	message(STATUS "Found clang at ${clang}")
else()
	message(WARNING "A compatible clang version was not found")
	message(WARNING "Can't run reachability analysis")
endif()


macro(debug_print variable)
	message(STATUS "${variable} is ${${variable}}")
endmacro()


function(add_analysis analysis_name target)
	if (NOT clang)
		message(ERROR "Can't run ${analysis_name} as clang is not found")
	endif()

	get_target_property(target_sources ${target} SOURCES)
	get_target_property(compile_definitions ${target} COMPILE_DEFINITIONS)
	get_target_property(include_directories ${target} INCLUDE_DIRECTORIES)

	# here generator expressions will be used for assembling compiler commands
	set(includes "")
	set(defines "")
	set(clang_compile_command "${clang} -S -emit-llvm ${includes} ${defines}"

	# tidy: the ir files must be placed in an appropriate directory
	set(ir_dir "${CMAKE_CURRENT_BINARY_DIR}/${analysis_name}")
	make_directory(${ir_dir})
	set(ir_files "")

	foreach(src ${target_sources})
		# these are sources of a target => they exist
		get_filename_component(absolute_path ${src} ABSOLUTE)
		get_filename_component(name_we ${absolute_path} NAME_WE)
		set(output_ir_file "${ir_dir}/${name_we}.ll")

		add_custom_command(
			OUTPUT ${output_ir_file}
			COMMAND ${clang_compile_command} -o ${output_ir_file} ${absolute_path}
			DEPENDS ${absolute_path}
			COMMENT "Generating ${output_ir_file}"
		)

		list(APPEND ir_files ${output_ir_file})
	endforeach()

	# there is still the scoping problem with the "callgraph" variable
	add_custom_target(
		${analysis_name}
		COMMAND ${callgraph} ${ir_files}
		DEPENDS ${callgraph} ${ir_files}
	)

	debug_print(target_sources)
	debug_print(compile_definitions)
	debug_print(include_directories)
	debug_print(ir_files)
	debug_print(callgraph)
endfunction()


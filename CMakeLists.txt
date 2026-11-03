cmake_minimum_required(VERSION 3.28)
project(modmask LANGUAGES C CXX)


find_package(X11        REQUIRED)
find_package(Fontconfig REQUIRED)
find_package(Freetype   REQUIRED)

set(TARGET_NAME st)
LIST(APPEND INCLUDE_DIRS ${FREETYPE_INCLUDE_DIRS} ${FONTCONFIG_INCLUDE_DIRS})

add_executable(${TARGET_NAME} st.c x.c)
target_include_directories(${TARGET_NAME} PRIVATE ${INCLUDE_DIRS})
target_compile_definitions(${TARGET_NAME} PRIVATE
	-DVERSION="0.9.3"
	-D_XOPEN_SOURCE=600
)
target_link_libraries(${TARGET_NAME} PRIVATE
	X11::X11
	Fontconfig::Fontconfig
	Freetype::Freetype
	Xft
	m
)


# this section is dedicated to the developer tools
add_subdirectory(tools)

set(INPUT_IR ${CMAKE_CURRENT_BINARY_DIR}/x.ll)
set(INPUT_SRC ${CMAKE_SOURCE_DIR}/x.c)
add_custom_command(
	OUTPUT ${INPUT_IR}
	COMMAND clang-19 -g -O0 -S -emit-llvm -D_XOPEN_SOURCE=600 -DVERSION=\"0.9.3\" -I ${INCLUDE_DIRS} ${INPUT_SRC} -o ${INPUT_IR}
	DEPENDS ${INPUT_SRC}
	COMMENT "Building the LLVM IR for analysis..."
)
add_custom_target(unreachable
	COMMAND ${UNREACHABLE_TOOL} ${INPUT_IR}
	DEPENDS ${UNREACHABLE_TOOL} ${INPUT_IR}
)


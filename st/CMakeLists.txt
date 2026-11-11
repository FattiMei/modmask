find_package(X11        REQUIRED)
find_package(Fontconfig REQUIRED)
find_package(Freetype   REQUIRED)

set(st_terminal "st_minimal")
add_executable(${st_terminal} st.c x.c)
target_compile_definitions(${st_terminal} PRIVATE
	-DVERSION="0.9.3"
	-D_XOPEN_SOURCE=600
)
target_include_directories(${st_terminal} PRIVATE
	${FONTCONFIG_INCLUDE_DIRS}
	${FREETYPE_INCLUDE_DIRS}
)
target_link_libraries(${st_terminal} PRIVATE
	X11::X11
	Xft
)

if(${callgraph})
	add_analysis(unreachable_st ${st_terminal})
endif()

find_package(X11        REQUIRED)
find_package(Fontconfig REQUIRED)
find_package(Freetype   REQUIRED)

set(ST_TERMINAL "st_minimal")
add_executable(${ST_TERMINAL} st.c x.c)
target_compile_definitions(${ST_TERMINAL} PRIVATE
	-DVERSION="0.9.3"
	-D_XOPEN_SOURCE=600
)
target_include_directories(${ST_TERMINAL} PRIVATE
	${FONTCONFIG_INCLUDE_DIRS}
	${FREETYPE_INCLUDE_DIRS}
)
target_link_libraries(${ST_TERMINAL} PRIVATE
	X11::X11
	Xft
)


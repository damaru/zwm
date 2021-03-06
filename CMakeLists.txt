cmake_minimum_required (VERSION 2.8)
project (zwm)
INCLUDE(FindPkgConfig)
find_package(X11)
find_package(Freetype)
if (FREETYPE_FOUND)
	list(APPEND INCLUDES ${FREETYPE_INCLUDE_DIRS}
	    ${X11_Xft_INCLUDE_PATH})
	    list(APPEND LIBS ${FREETYPE_LIBRARIES}
	    ${X11_Xft_LIB})
	list(APPEND DEFINES "HAVE_XFT")
else ()
	list(APPEND DISABLED "HAVE_XFT")
endif ()

if(NOT X11_FOUND)
	message(FATAL_ERROR "Failed to find X11 which is required to build zwm")
endif(NOT X11_FOUND)
set(LIBS ${LIBS} ${X11_LIBRARIES} "-lXinerama")
set(CMAKE_C_FLAGS "-g -Wall")
include_directories(${INCLUDES})
add_executable(
	zwm 
	zwm.h 
	atoms.c 
	client.c 
	decor.c
	events.c 
	ewmh.c 
	keypress.c 
	layouts.c 
	list.c
	mouse.c 
	panel.c 
	session.c 
	view.c 
	zwm.c 
	systray.c
	util.c
	)
target_link_libraries(zwm ${LIBS})

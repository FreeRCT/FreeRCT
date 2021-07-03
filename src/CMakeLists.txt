# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

# Sanity check
IF(NOT FREERCT_DONE_TOP_LEVEL_CMAKE)
	message(FATAL_ERROR "Please run cmake in the top-level directory, not this one.")
ENDIF()

project(freerct)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

# Source files
# TODO: Use explicit list instead?
file(GLOB freerct_SRCS
     "${CMAKE_SOURCE_DIR}/src/*.cpp"
     "${CMAKE_SOURCE_DIR}/src/*.h"
)

set(STRINGFILE "${CMAKE_SOURCE_DIR}/src/generated/coasters_strings")
add_custom_command(OUTPUT ${STRINGFILE}.cpp ${STRINGFILE}.h
                   COMMAND rcdgen --header ${STRINGFILE}.h --code ${STRINGFILE}.cpp --prefix COASTERS --base STR_GENERIC_COASTER_START
				   DEPENDS rcdgen
                   COMMENT "Generating COASTERS string files"
)
set(STRINGFILE "${CMAKE_SOURCE_DIR}/src/generated/shops_strings")
add_custom_command(OUTPUT ${STRINGFILE}.cpp ${STRINGFILE}.h
                   COMMAND rcdgen --header ${STRINGFILE}.h --code ${STRINGFILE}.cpp --prefix SHOPS --base STR_GENERIC_SHOP_START
				   DEPENDS rcdgen
                   COMMENT "Generating SHOPS string files"
)
set(STRINGFILE "${CMAKE_SOURCE_DIR}/src/generated/gentle_thrill_rides_strings")
add_custom_command(OUTPUT ${STRINGFILE}.cpp ${STRINGFILE}.h
                   COMMAND rcdgen --header ${STRINGFILE}.h --code ${STRINGFILE}.cpp --prefix GENTLE_THRILL_RIDES --base STR_GENERIC_GENTLE_THRILL_RIDES_START
				   DEPENDS rcdgen
                   COMMENT "Generating GENTLE/THRILL RIDES string files"
)
set(STRINGFILE "${CMAKE_SOURCE_DIR}/src/generated/entrance_exit_strings")
add_custom_command(OUTPUT ${STRINGFILE}.cpp ${STRINGFILE}.h
                   COMMAND rcdgen --header ${STRINGFILE}.h --code ${STRINGFILE}.cpp --prefix ENTRANCE_EXIT --base STR_GENERIC_ENTRANCE_EXIT_START
				   DEPENDS rcdgen
                   COMMENT "Generating ENTRANCE/EXITS string files"
)
set(STRINGFILE "${CMAKE_SOURCE_DIR}/src/generated/scenery_strings")
add_custom_command(OUTPUT ${STRINGFILE}.cpp ${STRINGFILE}.h
                   COMMAND rcdgen --header ${STRINGFILE}.h --code ${STRINGFILE}.cpp --prefix SCENERY --base STR_GENERIC_SCENERY_START
				   DEPENDS rcdgen
                   COMMENT "Generating SCENERY string files"
)
set(STRINGFILE "${CMAKE_SOURCE_DIR}/src/generated/gui_strings")
add_custom_command(OUTPUT ${STRINGFILE}.cpp ${STRINGFILE}.h
                   COMMAND rcdgen --header ${STRINGFILE}.h --code ${STRINGFILE}.cpp --prefix GUI --base STR_GUI_START
				   DEPENDS rcdgen
                   COMMENT "Generating GUI string files"
)

# Add platform specific files
IF(UNIX)
	file(GLOB freerct_platform_SRCS
	     "${CMAKE_SOURCE_DIR}/src/unix/*.cpp"
	     "${CMAKE_SOURCE_DIR}/src/unix/*.h"
	)
ELSEIF(WIN32)
	file(GLOB freerct_platform_SRCS
	     "${CMAKE_SOURCE_DIR}/src/windows/*.cpp"
	     "${CMAKE_SOURCE_DIR}/src/windows/*.h"
	)
ENDIF()
set(freerct_SRCS ${freerct_SRCS} ${freerct_platform_SRCS})

# Add generated files
set(freerct_SRCS ${freerct_SRCS}
    "${CMAKE_SOURCE_DIR}/src/rev.cpp"
    "${CMAKE_SOURCE_DIR}/src/generated/coasters_strings.h"
    "${CMAKE_SOURCE_DIR}/src/generated/gui_strings.h"
    "${CMAKE_SOURCE_DIR}/src/generated/shops_strings.h"
    "${CMAKE_SOURCE_DIR}/src/generated/gentle_thrill_rides_strings.h"
    "${CMAKE_SOURCE_DIR}/src/generated/entrance_exit_strings.h"
    "${CMAKE_SOURCE_DIR}/src/generated/scenery_strings.h"
)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/rev.cpp" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/coasters_strings.cpp" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/coasters_strings.h" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/gui_strings.cpp" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/gui_strings.h" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/shops_strings.cpp" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/shops_strings.h" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/gentle_thrill_rides_strings.cpp" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/gentle_thrill_rides_strings.h" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/entrance_exit_strings.cpp" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/entrance_exit_strings.h" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/scenery_strings.cpp" GENERATED)
set_source_files_properties("${CMAKE_SOURCE_DIR}/src/generated/scenery_strings.h" GENERATED)

# On windows, "WIN32" option need to be passed to
# add_excutable to get a Windows instead of Console
# application.
IF(WIN32)
	add_executable(freerct WIN32 ${freerct_SRCS})
ELSE()
	add_executable(freerct ${freerct_SRCS})
ENDIF()
add_dependencies(freerct rcd)

# Library detection
find_package(SDL2 REQUIRED)
IF(SDL2_FOUND)
	include_directories(${SDL2_INCLUDE_DIR})
	target_link_libraries(freerct ${SDL2_LIBRARY})
ENDIF()

find_package(SDL2_ttf REQUIRED)
# Legacy variable names
IF(SDL2_TTF_FOUND)
	include_directories(${SDL2TTF_INCLUDE_DIR})
	target_link_libraries(freerct ${SDL2TTF_LIBRARY})
ENDIF()

# Determine version string
if (NOT DEFINED VERSION_STRING)
	find_package(Git)
	IF(GIT_FOUND AND IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.git")
		execute_process(COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
			            OUTPUT_VARIABLE VERSION_STRING_1
			            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		)
		execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --always
			            OUTPUT_VARIABLE VERSION_STRING_2
			            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		)
		string(STRIP "0.1~git${VERSION_STRING_1}-${VERSION_STRING_2}" VERSION_STRING)
	ELSE()
		message(WARNING "WARNING: No VCS checkout detected and no custom version specified")
		set(VERSION_STRING "0.0.0-revdetect-broken")
	ENDIF()
ENDIF()

message(STATUS "Version string: ${VERSION_STRING}")
message(STATUS "Generating src/rev.cpp")
configure_file("${CMAKE_SOURCE_DIR}/src/rev.cpp.in" "${CMAKE_SOURCE_DIR}/src/rev.cpp")

IF(ASAN)
	set(CMAKE_C_FLAGS
		"-fsanitize=address -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-omit-frame-pointer -g"
		CACHE STRING "Flags used by the C compiler during AddressSanitizer builds."
		FORCE)
	set(CMAKE_CXX_FLAGS
		"-fsanitize=address -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-omit-frame-pointer -g"
		CACHE STRING "Flags used by the C++ compiler during AddressSanitizer builds."
		FORCE)
ENDIF()

# Compiler flags
# TODO: Extend for debug/release versions
IF(MSVC)
	# "If /Zc:rvalueCast is specified, the compiler follows section 5.4 of the
	# C++11 standard". We need C++11 for the way we use threads.
	add_compile_options(/Zc:rvalueCast)

	IF(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		# Enable multi-threaded compilation.
		add_compile_options(/MP)
	ENDIF()

	add_compile_options(/W3)
ELSE()
	add_compile_options(-g)
	add_compile_options(-Wall -Wextra -pedantic)
ENDIF()

# Add platform define
IF(UNIX)
	add_definitions("-DLINUX")
ELSEIF(WIN32)
	add_definitions("-DWINDOWS")
ELSE()
	message(FATAL_ERROR "Unsupported platform")
ENDIF()

INSTALL(PROGRAMS ${CMAKE_BINARY_DIR}/bin/freerct DESTINATION bin)

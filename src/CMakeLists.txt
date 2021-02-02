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
	include_directories("${SDL2_INCLUDE_DIR}")
	target_link_libraries(freerct ${SDL2_LIBRARY})
ENDIF()

find_package(SDL2_ttf REQUIRED)
# Legacy variable names
IF(SDL2_TTF_FOUND)
	include_directories("${SDL2TTF_INCLUDE_DIR}")
	target_link_libraries(freerct ${SDL2TTF_LIBRARY})
ENDIF()

# Determine version string
find_package(Git)
IF(GIT_FOUND AND IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.git")
	execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --always
	                OUTPUT_VARIABLE VERSION_STRING
	                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	)
	string(STRIP "${VERSION_STRING}" VERSION_STRING) # Newline
ELSE()
	# Only do this if you really want to
	IF(OVERRIDE_VCS)
		message(WARNING "WARNING: No VCS checkout detected but overridden by user")
		set(VERSION_STRING "0.0.0-norev")
	ELSE()
		message(FATAL_ERROR "No VCS checkout detected. Enable the OVERRIDE_VCS option if this is intended. This is not recommended.")
	ENDIF()
ENDIF()

message(STATUS "Version string: ${VERSION_STRING}")
message(STATUS "Generating src/rev.cpp")
configure_file("${CMAKE_SOURCE_DIR}/src/rev.cpp.in" "${CMAKE_SOURCE_DIR}/src/rev.cpp")

# Compiler flags
# TODO: Extend for debug/release versions
IF(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W2")
	# Note: SDL must be dynamic
ELSE()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -pedantic -Wall")
ENDIF()

# NOCOM
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
set (CMAKE_EXE_LINKER_FLAGS "-fsanitize=address" CACHE STRING "NOCOM" FORCE)

# Add platform define
IF(UNIX)
	add_definitions("-DLINUX")
ELSEIF(WIN32)
	add_definitions("-DWINDOWS")
ELSE()
	message(FATAL_ERROR "Unsupported platform")
ENDIF()

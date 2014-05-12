# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

# CMake File for FreeRCT

project(FreeRCT)
cmake_minimum_required(VERSION 2.8)

set(FREERCT_DONE_TOP_LEVEL_CMAKE ON)
set(CMAKE_USE_RELATIVE_PATHS ON)

option(OVERRIDE_VCS "Override the lack of a VCS checkout")

# Create the directories that the binaries and data files go into
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/rcd)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include individual projects
message(STATUS "")

message(STATUS "Building rcdgen")
add_subdirectory(src/rcdgen)

message(STATUS "Building RCD Files")
add_subdirectory(graphics/rcd)

message(STATUS "Building FreeRCT")
add_subdirectory(src)

IF(NOT MSVC)
	get_target_property(FREERCT_EXE freerct LOCATION)
	add_custom_target(run
	                  COMMAND ${FREERCT_EXE}
	                  WORKING_DIRECTORY src
	)
	add_dependencies(run freerct)

	# WARNING: Assumes gdb is in path, doesn't(/can't) find it
	add_custom_target(gdbrun
	                  COMMAND gdb ${FREERCT_EXE}
	                  WORKING_DIRECTORY src
	)
	add_dependencies(gdbrun freerct)

	# Documentation rules
	find_package(Doxygen)
	IF(DOXYGEN_FOUND)
		add_custom_target(doc COMMAND ${DOXYGEN_EXECUTABLE})
	ENDIF()

	list(APPEND DOCFILES
	     "data_format"
		 "manual"
		 "savegame"
	)

	# Warning: doesn't check for rst2html's existence
	FOREACH(DOCFILE ${DOCFILES})
		add_custom_target(${DOCFILE}.html
		                  COMMAND rst2html ${DOCFILE}.rst ${DOCFILE}.html
						  WORKING_DIRECTORY doc
		)
	ENDFOREACH()

ENDIF()

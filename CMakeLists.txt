# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

# CMake File for FreeRCT

project(FreeRCT)
cmake_minimum_required(VERSION 2.8)

# FreeRCT release 0.1
IF(NOT DEFINED VERSION_STRING)
	set(VERSION_STRING "0.1")
ENDIF()

# Add a compiler flag VAR_NAME=FLAG. If VAR_NAME is already set, FLAG is appended.
FUNCTION(add_flags VAR_NAME FLAG)
	IF(DEFINED ${VAR_NAME})
		set(${VAR_NAME} "${${VAR_NAME}} ${FLAG}" PARENT_SCOPE)
	ELSE()
		set(${VAR_NAME} "${FLAG}" PARENT_SCOPE)
	ENDIF()
ENDFUNCTION()

# Add flags for both the C and C++ compilers.
FUNCTION(add_c_cpp_flags FLAG)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}" PARENT_SCOPE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}" PARENT_SCOPE)
ENDFUNCTION()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake")
set(FREERCT_DONE_TOP_LEVEL_CMAKE ON)
set(CMAKE_USE_RELATIVE_PATHS ON)

option(ASAN "Use AddressSanitizer (https://clang.llvm.org/docs/AddressSanitizer.html)")
option(DEBIAN_PACKAGING "Package for system-wide Debian/Ubuntu installation")
option(RELEASE "Compile as release build")
option(WEBASSEMBLY "Compile as a WebAssembly program")

IF(NOT DEFINED FREERCT_BASENAME)
	# Should always be a valid Appstream ID
	# See https://www.freedesktop.org/software/appstream/docs/chap-Metadata.html#tag-id-generic
	set(FREERCT_BASENAME "net.freerct.FreeRCT")
ENDIF()

IF(DEBIAN_PACKAGING)
	set(CMAKE_INSTALL_PREFIX "/usr")
	set(CMAKE_INSTALL_DATAROOTDIR "share")
	set(CMAKE_INSTALL_BINDIR "games")
	set(PACKAGING_DATA_DIR "${CMAKE_INSTALL_DATAROOTDIR}/games/freerct")
ELSE()
	IF(NOT CMAKE_INSTALL_DATAROOTDIR)
		set(CMAKE_INSTALL_DATAROOTDIR "share")
	ENDIF()
	set(PACKAGING_DATA_DIR "${CMAKE_INSTALL_DATAROOTDIR}/freerct")
ENDIF()

# Create the directories that the binaries and data files go into
set(FRCT_BINARY_DIR "${CMAKE_BINARY_DIR}/bin")
file(MAKE_DIRECTORY "${FRCT_BINARY_DIR}")
file(MAKE_DIRECTORY "${FRCT_BINARY_DIR}/rcd")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${FRCT_BINARY_DIR}")

# Include individual projects
message(STATUS "")

message(STATUS "Building rcdgen")
add_subdirectory(src/rcdgen)

message(STATUS "Building RCD Files")
add_subdirectory(graphics/rcd)

message(STATUS "Building FreeRCT")
add_subdirectory(src)

message(STATUS "Installing packaging data")
add_subdirectory(packaging_data)

IF(NOT MSVC)
	add_custom_target(run
	                  COMMAND freerct
	                  WORKING_DIRECTORY "${FRCT_BINARY_DIR}"
	)
	add_dependencies(run freerct)

	# Warning: assumes gdb exists
	add_custom_target(gdbrun
	                  COMMAND gdb freerct
	                  WORKING_DIRECTORY "${FRCT_BINARY_DIR}"
	)
	add_dependencies(gdbrun freerct)

	# Documentation rules
	find_package(Doxygen)
	IF(DOXYGEN_FOUND)
		add_custom_target(doc
		                  COMMAND ${DOXYGEN_EXECUTABLE}
		                  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		)
	ENDIF()

	list(APPEND DOCFILES
	     "data_format"
		 "manual"
		 "savegame"
		 "savegame_history"
	)

	# Warning: assumes rst2html exists
	FOREACH(DOCFILE ${DOCFILES})
		add_custom_target("${DOCFILE}.html"
		                  COMMAND rst2html ${DOCFILE}.rst ${DOCFILE}.html
		                  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/doc"
		)
	ENDFOREACH()

ENDIF()

install(DIRECTORY data DESTINATION "${PACKAGING_DATA_DIR}")

# All the files in cpack/ are based largely on OpenTTD's packaging setup
# (https://github.com/OpenTTD/OpenTTD/commit/b7643b1d36cb41eb6712042761c13b9819fcbe74)
include(cpack/InstallAndPackage.cmake)

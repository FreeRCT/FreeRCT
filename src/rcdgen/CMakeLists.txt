# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

# Sanity check
IF(NOT FREERCT_DONE_TOP_LEVEL_CMAKE)
	message(FATAL_ERROR "Please run cmake on the top-level directory, not this one.")
ENDIF()

PROJECT(rcdgen)

# Source files
set(BisonOutput ${CMAKE_SOURCE_DIR}/src/rcdgen/parser.cpp)
set(FlexOutput ${CMAKE_SOURCE_DIR}/src/rcdgen/scanner.cpp)

# TODO: Use explicit list instead?
FILE(GLOB rcdgen_SRCS
     ${CMAKE_SOURCE_DIR}/src/rcdgen/*.cpp
     ${CMAKE_SOURCE_DIR}/src/rcdgen/*.h
)

set(rcdgen_SRCS ${rcdgen_SRCS}
    # Generated files
    ${BisonOutput}
    ${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h
    ${FlexOutput}

    # Files in parent directory
    ${CMAKE_SOURCE_DIR}/src/getoptdata.cpp
    ${CMAKE_SOURCE_DIR}/src/stdafx.h
)

add_executable(rcdgen ${rcdgen_SRCS})

# Library detection
find_package(PNG REQUIRED)
IF(PNG_FOUND)
	include_directories(${PNG_INCLUDE_DIR})
	target_link_libraries(rcdgen ${PNG_LIBRARY})
ENDIF()

find_package(ZLIB REQUIRED)
IF(ZLIB_FOUND)
	include_directories(${ZLIB_INCLUDE_DIR})
	target_link_libraries(rcdgen ${ZLIB_LIBRARY})
ENDIF()

find_package(BISON)
# Bison/m4 is broken on windows
IF(NOT WIN32 AND BISON_FOUND)
	add_custom_command(OUTPUT ${BisonOutput} ${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h
	                   COMMAND ${BISON_EXECUTABLE}
	                   --defines=${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h
	                   --output=${BisonOutput}
	                   ${CMAKE_SOURCE_DIR}/src/rcdgen/parser.y
	                   DEPENDS ${CMAKE_SOURCE_DIR}/src/rcdgen/parser.y
	                   COMMENT "Generating parser.cpp"
	)
	set_source_files_properties(${BisonOutput} GENERATED)
	set_source_files_properties(${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h GENERATED)
ELSE()
	message(STATUS "No bison executable found, using pregenerated parser")
	configure_file(${CMAKE_SOURCE_DIR}/src/rcdgen/parser.cpp.pregen ${BisonOutput})
	configure_file(${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h.pregen ${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h)
ENDIF()

find_package(FLEX)
IF(FLEX_FOUND)
	add_custom_command(OUTPUT ${FlexOutput}
	                   COMMAND ${FLEX_EXECUTABLE} -o${FlexOutput}
	                   ${CMAKE_SOURCE_DIR}/src/rcdgen/scanner.l
	                   DEPENDS ${CMAKE_SOURCE_DIR}/src/rcdgen/scanner.l
	                   COMMENT "Generating scanner.cpp"
	)
	set_source_files_properties(${FlexOutput} GENERATED)
ELSE()
	message(STATUS "No flex executable found, using pregenerated scanner")
	configure_file(${CMAKE_SOURCE_DIR}/src/rcdgen/scanner.cpp.pregen ${FlexOutput})
ENDIF()

# Compiler flags
# TODO: Extend for debug/release versions
IF(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W2")
	# Compile statically
	FOREACH(flag
	        CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS_DEBUG
	        CMAKE_C_FLAGS_DEBUG_INIT CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO
	        CMAKE_CXX_FLAGS_DEBUG  CMAKE_CXX_FLAGS_DEBUG_INIT)
		string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
	ENDFOREACH()
ELSE()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -pedantic -Wall")
ENDIF()

IF(NOT WIN32 AND BISON_FOUND AND FLEX_FOUND)
	add_custom_target(exportscanner
	                  COMMAND ${CMAKE_COMMAND} -P ExportScanner.cmake
	                  COMMENT "Generating scanner files"
	)
ENDIF()

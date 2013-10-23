# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

# Sanity check
IF(FREERCT_DONE_TOP_LEVEL_CMAKE)
ELSE()
	message(FATAL_ERROR "Please run cmake on the top-level directory, not this one.")
ENDIF()

PROJECT(rcdgen)

# Source files
SET(BisonOutput ${CMAKE_SOURCE_DIR}/src/rcdgen/parser.cpp)
SET(FlexOutput ${CMAKE_SOURCE_DIR}/src/rcdgen/scanner.cpp)

# TODO: Use explicit list instead?
FILE(GLOB rcdgen_SRCS
  ${CMAKE_SOURCE_DIR}/src/rcdgen/*.cpp
  ${CMAKE_SOURCE_DIR}/src/rcdgen/*.h
)

SET(rcdgen_SRCS ${rcdgen_SRCS}
  # Generated files
  ${BisonOutput}
  ${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h
  ${FlexOutput}

  # Files in parent directory
  ${CMAKE_SOURCE_DIR}/src/getoptdata.cpp
  ${CMAKE_SOURCE_DIR}/src/stdafx.h
)

ADD_EXECUTABLE(rcdgen ${rcdgen_SRCS})

# Library detection
FIND_PACKAGE(PNG REQUIRED)
IF(PNG_FOUND)
	INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(rcdgen ${PNG_LIBRARY})
ENDIF()

FIND_PACKAGE(BISON)
# Bison/m4 is broken on windows
IF(NOT WIN32 AND BISON_FOUND)
	add_custom_command(OUTPUT ${BisonOutput} ${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h
	                   COMMAND ${BISON_EXECUTABLE}
	                   --defines=${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h
	                   --output=${BisonOutput}
	                   ${CMAKE_SOURCE_DIR}/src/rcdgen/parser.y
	                   DEPENDS ${CMAKE_SOURCE_DIR}/src/rcdgen/parser.y
	                   COMMENT "Generating parser.cpp")
	SET_SOURCE_FILES_PROPERTIES(${BisonOutput} GENERATED)
	SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h GENERATED)
ELSE()
	message(STATUS "No bison executable found, using pregenerated parser")
	CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/src/rcdgen/parser.cpp.pregen ${BisonOutput})
	CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h.pregen ${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h)
ENDIF()

FIND_PACKAGE(FLEX)
IF(FLEX_FOUND)
	add_custom_command(OUTPUT ${FlexOutput}
	                   COMMAND ${FLEX_EXECUTABLE} -o${FlexOutput}
	                   ${CMAKE_SOURCE_DIR}/src/rcdgen/scanner.l
	                   DEPENDS ${CMAKE_SOURCE_DIR}/src/rcdgen/scanner.l
	                   COMMENT "Generating scanner.cpp")
	SET_SOURCE_FILES_PROPERTIES(${FlexOutput} GENERATED)
ELSE()
	message(STATUS "No flex executable found, using pregenerated scanner")
	CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/src/rcdgen/scanner.cpp.pregen ${FlexOutput})
ENDIF()

# Compiler flags
# TODO: Extend for release versions
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -std=c++11")

IF(NOT WIN32 AND BISON_FOUND AND FLEX_FOUND)
	add_custom_target(exportscanner
	                  COMMAND ${CMAKE_COMMAND} -P ExportScanner.cmake
	                  COMMENT "Generating scanner files")
ENDIF()

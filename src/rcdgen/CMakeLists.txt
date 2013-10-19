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

# Add generated files
SET_SOURCE_FILES_PROPERTIES(${BisonOutput} GENERATED)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h GENERATED)
SET_SOURCE_FILES_PROPERTIES(${FlexOutput} GENERATED)

ADD_EXECUTABLE(rcdgen ${rcdgen_SRCS})

# Library detection
FIND_PACKAGE(PNG REQUIRED)
IF(PNG_FOUND)
	INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(rcdgen ${PNG_LIBRARY})
ENDIF()

FIND_PACKAGE(BISON REQUIRED)
IF(BISON_FOUND)
    add_custom_command(
      OUTPUT ${BisonOutput}
      COMMAND ${BISON_EXECUTABLE}
              --defines=${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h
              --output=${BisonOutput}
              ${CMAKE_SOURCE_DIR}/src/rcdgen/parser.y
      COMMENT "Generating parser.cpp"
    )
ENDIF()

FIND_PACKAGE(FLEX REQUIRED)
IF(FLEX_FOUND)
    add_custom_command(
      OUTPUT ${FlexOutput}
      COMMAND ${FLEX_EXECUTABLE}
              -o${FlexOutput}
              ${CMAKE_SOURCE_DIR}/src/rcdgen/scanner.l
      COMMENT "Generating scanner.cpp"
    )
ENDIF()

# Compiler flags
# TODO: Extend for release versions
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -std=c++11")

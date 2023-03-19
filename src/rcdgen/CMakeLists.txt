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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

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

# Source files
set(BisonOutput "${CMAKE_SOURCE_DIR}/src/rcdgen/parser.cpp")
set(FlexOutput "${CMAKE_SOURCE_DIR}/src/rcdgen/scanner.cpp")

# TODO: Use explicit list instead?
FILE(GLOB rcdgen_SRCS
     "${CMAKE_SOURCE_DIR}/src/rcdgen/*.cpp"
     "${CMAKE_SOURCE_DIR}/src/rcdgen/*.h"
)

set(rcdgen_SRCS ${rcdgen_SRCS}
    # Generated files
    "${BisonOutput}"
    "${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h"
    "${FlexOutput}"

    # Files in parent directory
    "${CMAKE_SOURCE_DIR}/src/getoptdata.cpp"
    "${CMAKE_SOURCE_DIR}/src/language_definitions.cpp"
    "${CMAKE_SOURCE_DIR}/src/stdafx.h"
    "${CMAKE_SOURCE_DIR}/src/string_func.cpp"
)

add_executable(rcdgen ${rcdgen_SRCS})

# Library detection
IF(APPLE)
	# Avoid searching for headers in frameworks (like Mono), and libraries in LIBDIR.
	set(CMAKE_FIND_FRAMEWORK LAST)
ELSEIF(MSVC)
	# Accept zlib from OpenTTD useful compiled by previous Visual Studio version.
	set_target_properties(rcdgen PROPERTIES LINK_FLAGS "/SAFESEH:NO")
ENDIF()

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
	add_custom_command(OUTPUT "${BisonOutput}" "${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h"
	                   COMMAND ${BISON_EXECUTABLE}
	                   --defines=${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h
	                   --output=${BisonOutput}
	                   ${CMAKE_SOURCE_DIR}/src/rcdgen/parser.y
	                   DEPENDS ${CMAKE_SOURCE_DIR}/src/rcdgen/parser.y
	                   COMMENT "Generating parser.cpp"
	)
	set_source_files_properties("${BisonOutput}" GENERATED)
	set_source_files_properties("${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h" GENERATED)
ELSE()
	message(STATUS "No bison executable found, using pregenerated parser")
	configure_file("${CMAKE_SOURCE_DIR}/src/rcdgen/parser.cpp.pregen" "${BisonOutput}")
	configure_file("${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h.pregen" "${CMAKE_SOURCE_DIR}/src/rcdgen/tokens.h")
ENDIF()

find_package(FLEX)
IF(FLEX_FOUND)
	add_custom_command(OUTPUT "${FlexOutput}"
	                   COMMAND ${FLEX_EXECUTABLE} -o${FlexOutput}
	                   ${CMAKE_SOURCE_DIR}/src/rcdgen/scanner.l
	                   DEPENDS ${CMAKE_SOURCE_DIR}/src/rcdgen/scanner.l
	                   COMMENT "Generating scanner.cpp"
	)
	set_source_files_properties("${FlexOutput}" GENERATED)
ELSE()
	message(STATUS "No flex executable found, using pregenerated scanner")
	configure_file("${CMAKE_SOURCE_DIR}/src/rcdgen/scanner.cpp.pregen" "${FlexOutput}")
ENDIF()

IF(NOT WIN32 AND BISON_FOUND AND FLEX_FOUND)
	add_custom_target(exportscanner
	                  COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/CMake/ExportRcdgenScanner.cmake
	                  COMMENT "Generating scanner files"
	)
ENDIF()

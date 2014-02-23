# $Id$
#
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
     ${CMAKE_SOURCE_DIR}/src/*.cpp
     ${CMAKE_SOURCE_DIR}/src/*.h
)

get_target_property(RCDGEN_EXE rcdgen LOCATION)

set(STRINGFILE ${CMAKE_SOURCE_DIR}/src/generated/coasters_strings)
add_custom_command(OUTPUT ${STRINGFILE}.cpp ${STRINGFILE}.h
                   COMMAND ${RCDGEN_EXE} --header ${STRINGFILE}.h --code ${STRINGFILE}.cpp --prefix COASTERS --base STR_GENERIC_COASTER_START
                   COMMENT "Generating COASTERS string files"
)
set(STRINGFILE ${CMAKE_SOURCE_DIR}/src/generated/shops_strings)
add_custom_command(OUTPUT ${STRINGFILE}.cpp ${STRINGFILE}.h
                   COMMAND ${RCDGEN_EXE} --header ${STRINGFILE}.h --code ${STRINGFILE}.cpp --prefix SHOPS --base STR_GENERIC_SHOP_START
                   COMMENT "Generating SHOPS string files"
)
set(STRINGFILE ${CMAKE_SOURCE_DIR}/src/generated/gui_strings)
add_custom_command(OUTPUT ${STRINGFILE}.cpp ${STRINGFILE}.h
                   COMMAND ${RCDGEN_EXE} --header ${STRINGFILE}.h --code ${STRINGFILE}.cpp --prefix GUI --base STR_GUI_START
                   COMMENT "Generating GUI string files"
)

# Add generated files
set(freerct_SRCS ${freerct_SRCS}
  ${CMAKE_SOURCE_DIR}/src/rev.cpp
  ${CMAKE_SOURCE_DIR}/src/generated/coasters_strings.h
  ${CMAKE_SOURCE_DIR}/src/generated/gui_strings.h
  ${CMAKE_SOURCE_DIR}/src/generated/shops_strings.h
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/src/rev.cpp GENERATED)
set_source_files_properties(${CMAKE_SOURCE_DIR}/src/generated/coasters_strings.cpp GENERATED)
set_source_files_properties(${CMAKE_SOURCE_DIR}/src/generated/coasters_strings.h GENERATED)
set_source_files_properties(${CMAKE_SOURCE_DIR}/src/generated/gui_strings.cpp GENERATED)
set_source_files_properties(${CMAKE_SOURCE_DIR}/src/generated/gui_strings.h GENERATED)
set_source_files_properties(${CMAKE_SOURCE_DIR}/src/generated/shops_strings.cpp GENERATED)
set_source_files_properties(${CMAKE_SOURCE_DIR}/src/generated/shops_strings.h GENERATED)

add_executable(freerct ${freerct_SRCS})
add_dependencies(freerct rcd)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)

# Library detection
find_package(SDL2 REQUIRED)
IF(SDL2_FOUND)
	include_directories(${SDL2_INCLUDE_DIR})
	target_link_libraries(freerct ${SDL2_LIBRARY})
ENDIF()

find_package(SDL2_ttf REQUIRED)
# Legacy variable names
IF(SDL2TTF_FOUND)
	include_directories(${SDL2TTF_INCLUDE_DIR})
	target_link_libraries(freerct ${SDL2TTF_LIBRARY})
ENDIF()

# Translated messages are bad
set(SAVED_LC_ALL "$ENV{LC_ALL}")
set(ENV{LC_ALL} C)

# Hg check only exists in 2.8.10
IF(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.hg" AND ${CMAKE_VERSION} STRLESS "2.8.10")
	message(FATAL_ERROR "Hg detection requires CMake 2.8.10 or higher. Please upgrade CMake and try again.")
ELSE()
	find_package(Hg)
ENDIF()

# Determine version string
find_package(Subversion)
find_package(Git)
IF(SUBVERSION_FOUND AND IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.svn")
	# svn checkout
	set(SHORT_VCS "r")
	message(STATUS "Detected svn checkout")
	execute_process(COMMAND ${Subversion_SVN_EXECUTABLE} info --xml ${CMAKE_SOURCE_DIR}
	                OUTPUT_VARIABLE INFO_XML
	)

	set(REGEX_STRING "<date>([0-9][0-9][0-9][0-9])-([0-9][0-9])-([0-9][0-9])T([0-9][0-9]):([0-9][0-9]):[0-9][0-9].[0-9]*Z</date>")
	string(REGEX MATCH   ${REGEX_STRING} INFO_XML "${INFO_XML}")
	string(REGEX REPLACE ${REGEX_STRING} "\\1\\2\\3-\\4\\5" DATE_STRING "${INFO_XML}")
	IF(DATE_STRING STREQUAL "")
		message(FATAL_ERROR "No date found")
	ENDIF()
	Subversion_WC_INFO(${CMAKE_SOURCE_DIR} co)
	set(REV ${co_WC_LAST_CHANGED_REV})

ELSEIF(GIT_FOUND AND IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.git")
	# Assume UTC timezone (how svn stores it)
	execute_process(COMMAND ${GIT_EXECUTABLE} log -1 "--pretty=%ci" OUTPUT_VARIABLE GITLOG WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
	set(REGEX_STRING "([0-9][0-9][0-9][0-9])-([0-9][0-9])-([0-9][0-9]) ([0-9][0-9]):([0-9][0-9]):[0-9][0-9] .*")
	string(REGEX REPLACE ${REGEX_STRING} "\\1\\2\\3-\\4\\5" DATE_STRING ${GITLOG})

	# Assume git svn
	execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
	                OUTPUT_VARIABLE CHASH
	                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	)
	execute_process(COMMAND ${GIT_EXECUTABLE} svn HEAD find-rev ${CHASH}
	                OUTPUT_VARIABLE REV
	                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	)

	file(GLOB GITFILES ${CMAKE_SOURCE_DIR}/.git/svn/*)
	IF(GITFILES STREQUAL "" OR REV STREQUAL "")
		# git checkout
		message(STATUS "Detected git checkout")
		execute_process(COMMAND ${GIT_EXECUTABLE} rev-list HEAD --count
		                OUTPUT_VARIABLE REV
		                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		)
		set(SHORT_VCS "g")
	ELSE()
		# git svn checkout
		message(STATUS "Detected git svn checkout")
		set(SHORT_VCS "r")
	ENDIF()

	string(REGEX REPLACE "([0-9]+)\n" "\\1" REV ${REV}) # Get rid of a newline

ELSEIF(HG_FOUND AND IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.hg")
	# WARNING: hg < 2.4 don't like date method
	execute_process(COMMAND ${HG_EXECUTABLE} parent "--template={date(date, '%Y%m%d-%H%M')}"
	                OUTPUT_VARIABLE DATE_STRING
	)
	execute_process(COMMAND ${HG_EXECUTABLE} parent "--template={svnrev}"
	                OUTPUT_VARIABLE REV
	)

	IF(REV STREQUAL "")
		# mercurial checkout
		message(STATUS "Detected mercurial checkout!")
		execute_process(COMMAND ${HG_EXECUTABLE} parent "--template={rev}"
		                OUTPUT_VARIABLE REV
		)
		set(SHORT_VCS "h")
	ELSE()
		# hgsubversion checkout
		message(STATUS "Detected hgsubversion checkout!")
		set(SHORT_VCS "r")
	ENDIF()
ELSE()
	# Only do this if you really want to
	IF(OVERRIDE_VCS)
		message(WARNING "WARNING: No VCS checkout detected but overridden by user")
		set(DATE_STRING "norev")
		set(SHORT_VCS "n")
		set(REV "000")
	ELSE()
		message(FATAL_ERROR "No VCS checkout detected. Enable the OVERRIDE_VCS option if this is intended. This is not recommended.")
	ENDIF()
ENDIF()

set(VERSION_STRING ${DATE_STRING}.${SHORT_VCS}${REV})

message(STATUS "Version string: ${VERSION_STRING}")
message(STATUS "Generating src/rev.cpp")
configure_file(${CMAKE_SOURCE_DIR}/src/rev.cpp.in ${CMAKE_SOURCE_DIR}/src/rev.cpp)

# Restore
set(ENV{LC_ALL} ${SAVED_LC_ALL})

# Compiler flags
# TODO: Extend for debug/release versions
IF(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W2")
	# Note: SDL must be dynamic
ELSE()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -pedantic -Wall")
ENDIF()

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
	message(FATAL_ERROR "Please run cmake in the top-level directory, not this one.")
ENDIF()

PROJECT(freerct)

# Source files
# TODO: Use explicit list instead?
FILE(GLOB freerct_SRCS
  ${CMAKE_SOURCE_DIR}/src/*.cpp
  ${CMAKE_SOURCE_DIR}/src/*.h
  ${CMAKE_SOURCE_DIR}/src/objects/table/*.h # ?
  ${CMAKE_SOURCE_DIR}/src/table/*.h
)

# List of rcdgen generated files (without extension)
LIST(APPEND GENSTRINGFILES
  ${CMAKE_SOURCE_DIR}/src/objects/table/coasters_strings
  ${CMAKE_SOURCE_DIR}/src/objects/table/gui_strings
  ${CMAKE_SOURCE_DIR}/src/objects/table/shops_strings
)

GET_TARGET_PROPERTY(RCDGEN_EXE rcdgen LOCATION)

SET(STRINGFILE ${CMAKE_SOURCE_DIR}/src/objects/table/coasters_strings)
ADD_CUSTOM_COMMAND(
      OUTPUT ${STRINGFILE}.cpp ${STRINGFILE}.h
      COMMAND ${RCDGEN_EXE} --header ${STRINGFILE}.h --code ${STRINGFILE}.cpp --prefix COASTERS --base STR_GENERIC_COASTER_START
      COMMENT "Generating COASTERS string files"
)
SET(STRINGFILE ${CMAKE_SOURCE_DIR}/src/objects/table/shops_strings)
ADD_CUSTOM_COMMAND(
      OUTPUT ${STRINGFILE}.cpp ${STRINGFILE}.h
      COMMAND ${RCDGEN_EXE} --header ${STRINGFILE}.h --code ${STRINGFILE}.cpp --prefix SHOPS --base STR_GENERIC_SHOP_START
      COMMENT "Generating SHOPS string files"
)
SET(STRINGFILE ${CMAKE_SOURCE_DIR}/src/objects/table/gui_strings)
ADD_CUSTOM_COMMAND(
      OUTPUT ${STRINGFILE}.cpp ${STRINGFILE}.h
      COMMAND ${RCDGEN_EXE} --header ${STRINGFILE}.h --code ${STRINGFILE}.cpp --prefix GUI --base STR_GUI_START
      COMMENT "Generating GUI string files"
)


# Add generated files
SET(freerct_SRCS ${freerct_SRCS}
  ${CMAKE_SOURCE_DIR}/src/rev.cpp
  ${CMAKE_SOURCE_DIR}/src/objects/table/coasters_strings.h
  ${CMAKE_SOURCE_DIR}/src/objects/table/gui_strings.h
  ${CMAKE_SOURCE_DIR}/src/objects/table/shops_strings.h
)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/src/rev.cpp GENERATED)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/src/objects/table/coasters_strings.h GENERATED)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/src/objects/table/gui_strings.h GENERATED)
SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/src/objects/table/shops_strings.h GENERATED)

ADD_EXECUTABLE(freerct ${freerct_SRCS})
add_dependencies(freerct rcd)

# Library detection
FIND_PACKAGE(SDL REQUIRED)
IF(SDL_FOUND)
	INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(freerct ${SDL_LIBRARY})
ENDIF()

FIND_PACKAGE(SDL_ttf REQUIRED)
# Legacy variable names
IF(SDLTTF_FOUND)
	INCLUDE_DIRECTORIES(${SDLTTF_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(freerct ${SDLTTF_LIBRARY})
ENDIF()

# Translated messages are bad
SET(SAVED_LC_ALL "$ENV{LC_ALL}")
SET(ENV{LC_ALL} C)

# Hg check only exists in 2.8.10
IF(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.hg" AND ${CMAKE_VERSION} STRLESS "2.8.10")
	message(FATAL_ERROR "Hg detection requires CMake 2.8.10 or higher. Please upgrade CMake and try again.")
ELSE()
	FIND_PACKAGE(Hg)
ENDIF()

# Determine version string
FIND_PACKAGE(Subversion)
FIND_PACKAGE(Git)
IF(SUBVERSION_FOUND AND IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.svn")
	# svn checkout
	SET(SHORT_VCS "r")
	message(STATUS "Detected svn checkout")
	execute_process(COMMAND ${Subversion_SVN_EXECUTABLE} info --xml ${CMAKE_SOURCE_DIR}
	                OUTPUT_VARIABLE INFO_XML)
	SET(REGEX_STRING "<date>([0-9][0-9][0-9][0-9])-([0-9][0-9])-([0-9][0-9])T([0-9][0-9]):([0-9][0-9]):[0-9][0-9].[0-9]*Z</date>")
	string(REGEX MATCH   ${REGEX_STRING} INFO_XML "${INFO_XML}")
	string(REGEX REPLACE ${REGEX_STRING} "\\1\\2\\3-\\4\\5" DATE_STRING "${INFO_XML}")
	IF(DATE_STRING STREQUAL "")
		message(FATAL_ERROR "No date found")
	ENDIF()
	Subversion_WC_INFO(${CMAKE_SOURCE_DIR} co)
	SET(REV ${co_WC_LAST_CHANGED_REV})
ELSEIF(GIT_FOUND AND IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.git")
	# Assume UTC timezone (how svn stores it)
	execute_process(COMMAND ${GIT_EXECUTABLE} log -1 "--pretty=%ci" OUTPUT_VARIABLE GITLOG)
	SET(REGEX_STRING "([0-9][0-9][0-9][0-9])-([0-9][0-9])-([0-9][0-9]) ([0-9][0-9]):([0-9][0-9]):[0-9][0-9] .*")
	string(REGEX REPLACE ${REGEX_STRING} "\\1\\2\\3-\\4\\5" DATE_STRING ${GITLOG})

	FILE(GLOB GITFILES ${CMAKE_SOURCE_DIR}/.git/svn/*)
	IF(GITFILES STREQUAL "")
		# git checkout
		message(STATUS "Detected git checkout")
		execute_process(COMMAND ${GIT_EXECUTABLE} rev-list HEAD --count
		                OUTPUT_VARIABLE REV)
		SET(SHORT_VCS "g")
	ELSE()
		# git svn checkout
		message(STATUS "Detected git svn checkout")
		execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
		                OUTPUT_VARIABLE CHASH)
		execute_process(COMMAND ${GIT_EXECUTABLE} svn HEAD find-rev ${CHASH}
		                OUTPUT_VARIABLE REV)
		string(REGEX REPLACE "([0-9]+)\n" "\\1" REV ${REV}) # Get rid of a newline
		SET(SHORT_VCS "r")
	ENDIF()
ELSEIF(HG_FOUND AND IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.hg")
	# WARNING: hg < 2.4 don't like date method
	execute_process(COMMAND ${HG_EXECUTABLE} parent "--template={date(date, '%Y%m%d-%H%M')}" OUTPUT_VARIABLE DATE_STRING)
	execute_process(COMMAND ${HG_EXECUTABLE} parent "--template={svnrev}"
	                OUTPUT_VARIABLE REV)

	IF(REV STREQUAL "")
		# mercurial checkout
		message(STATUS "Detected mercurial checkout!")
		execute_process(COMMAND ${HG_EXECUTABLE} parent "--template={rev}"
		                OUTPUT_VARIABLE REV)
		SET(SHORT_VCS "h")
	ELSE()
		# hgsubversion checkout
		message(STATUS "Detected hgsubversion checkout!")
		SET(SHORT_VCS "r")
	ENDIF()
ELSE()
	# Only do this if you really want to
	IF(OVERRIDE_VCS)
		message(WARNING "WARNING: No VCS checkout detected but overridden by user")
		SET(DATE_STRING "norev")
		SET(SHORT_VCS "n")
		SET(REV "000")
	ELSE()
		message(FATAL_ERROR "No VCS checkout detected. Enable the OVERRIDE_VCS option if this is intended. This is not recommended.")
	ENDIF()
ENDIF()

SET(VERSION_STRING ${DATE_STRING}.${SHORT_VCS}${REV})

message(STATUS "Version string: ${VERSION_STRING}")
message(STATUS "Generating src/rev.cpp")
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/src/rev.cpp.in ${CMAKE_SOURCE_DIR}/src/rev.cpp)

# Restore
SET(ENV{LC_ALL} ${SAVED_LC_ALL})

# Extra include directories
# TODO: Remove?
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/objects)

# Compiler flags
# TODO: Extend for release versions
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall")

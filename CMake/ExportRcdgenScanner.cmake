# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

# CMake script for exporting scanner files

IF(NOT DEFINED SOURCE_DIR)
	message(FATAL_ERROR "Variable 'SOURCE_DIR' must be provided")
ENDIF()

set(SCANNER_FILE "${SOURCE_DIR}/src/rcdgen/scanner.cpp")
set(PARSER_FILE  "${SOURCE_DIR}/src/rcdgen/parser.cpp")
set(TOKENS_FILE  "${SOURCE_DIR}/src/rcdgen/tokens.h")

IF(EXISTS ${SCANNER_FILE})
	file(READ ${SCANNER_FILE} SCANNER)
	# Remove personal stuff from the defines
	string(REGEX REPLACE "\"[^\n]+src/rcdgen/(scanner.(cpp|l))\"" "\"\\1\""  SCANNER "${SCANNER}")
	file(WRITE ${SCANNER_FILE}.pregen "${SCANNER}")
	unset(SCANNER) # Don't store in memory
ENDIF()

IF(EXISTS ${PARSER_FILE} AND EXISTS ${TOKENS_FILE})
	file(READ ${PARSER_FILE} PARSER)
	# Remove personal stuff from the defines
	string(REGEX REPLACE "\"[^\n]+src/rcdgen/(parser.(cpp|y))\"" "\"\\1\""  PARSER "${PARSER}")
	string(REGEX REPLACE "(YY_YY_)[^\n]*(SRC_RCDGEN)" "\\1\\2"  PARSER "${PARSER}")
	file(WRITE ${PARSER_FILE}.pregen "${PARSER}")
	unset(PARSER) # Don't store in memory

	file(READ ${TOKENS_FILE} TOKENS)
	# Remove personal stuff from the defines
	string(REGEX REPLACE "(YY_YY_)[^\n]*(SRC_RCDGEN)" "\\1\\2"  TOKENS "${TOKENS}")
	file(WRITE ${TOKENS_FILE}.pregen "${TOKENS}")
	unset(TOKENS) # Don't store in memory
ENDIF()

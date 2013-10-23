# $Id$
#
# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

# CMake script for exporting scanner files

IF(EXISTS scanner.cpp)
	FILE(READ scanner.cpp SCANNER)
	# Remove personal stuff from the defines
	STRING(REGEX REPLACE "\"[^\n]+src/rcdgen/(scanner.(cpp|l))\"" "\"\\1\""  SCANNER "${SCANNER}")
	FILE(WRITE scanner.cpp.pregen "${SCANNER}")
	UNSET(SCANNER) # Don't store in memory
ENDIF()

IF(EXISTS parser.cpp AND EXISTS tokens.h)
	FILE(READ parser.cpp PARSER)
	# Remove personal stuff from the defines
	STRING(REGEX REPLACE "\"[^\n]+src/rcdgen/(parser.(cpp|y))\"" "\"\\1\""  PARSER "${PARSER}")
	STRING(REGEX REPLACE "(YY_YY_)[^\n]*(SRC_RCDGEN)" "\\1\\2"  PARSER "${PARSER}")
	FILE(WRITE parser.cpp.pregen "${PARSER}")
	UNSET(PARSER) # Don't store in memory

	FILE(READ tokens.h TOKENS)
	# Remove personal stuff from the defines
	STRING(REGEX REPLACE "(YY_YY_)[^\n]*(SRC_RCDGEN)" "\\1\\2"  TOKENS "${TOKENS}")
	FILE(WRITE tokens.h.pregen "${TOKENS}")
	UNSET(TOKENS) # Don't store in memory
ENDIF()

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

PROJECT(rcd_files)

# Make declaring the list shorter/nicer
SET(FP "${CMAKE_SOURCE_DIR}/graphics/rcd/")

# Output file                     Source file
LIST(APPEND RCDFILES
  ${FP}borderhighlight_8bpp64.rcd ${FP}freerct.txt ""
  ${FP}cornerhighlight_8bpp64.rcd ${FP}freerct.txt ""
  ${FP}groundtile_8bpp64.rcd      ${FP}freerct.txt ""
  ${FP}people_animation.rcd       ${FP}freerct.txt ""
  ${FP}path_8bpp64.rcd            ${FP}paths.txt   ""
  ${FP}shops.rcd                  ${FP}shops.txt   "shops"
  ${FP}gui.rcd                    ${FP}gui.txt     "gui"
  ${FP}tracks.rcd                 ${FP}tracks.txt  "tracks"
)

GET_TARGET_PROPERTY(RCDGEN_EXE rcdgen LOCATION)

LIST(LENGTH RCDFILES RCDFILES_LENGTH)
math(EXPR RCDFILES_LENGTH_DIV_3 "${RCDFILES_LENGTH} / 3 - 1")

FOREACH(RCDFILE RANGE ${RCDFILES_LENGTH_DIV_3})
	math(EXPR INDEX_1 "${RCDFILE} * 3")
	math(EXPR INDEX_2 "${INDEX_1} + 1")
	math(EXPR INDEX_3 "${INDEX_1} + 2")
	LIST(GET RCDFILES ${INDEX_1} OUTFILE)
	LIST(GET RCDFILES ${INDEX_2} SRCFILE)
	LIST(GET RCDFILES ${INDEX_3} DEPTYPE)

	IF(DEPTYPE)
		file(GLOB LANGLIST ${FP}lang/*_${DEPTYPE}.txt)
	ENDIF()

	ADD_CUSTOM_COMMAND(
	      OUTPUT ${OUTFILE}
	      COMMAND rcdgen ${SRCFILE}
	      COMMENT "Generating rcd files from ${SRCFILE}"
	      DEPENDS ${SRCFILE} ${RCDGEN_EXE} ${LANGLIST}
	      WORKING_DIRECTORY ${FP}
	)

	LIST(APPEND OUTFILES ${OUTFILE})
ENDFOREACH(RCDFILE)

add_custom_target(rcd ALL DEPENDS ${OUTFILES})

IF(NOT MSVC)
	# Warning: doesn't check for rst2html's existence
	add_custom_target(data_format.html COMMAND rst2html data_format.rst data_format.html)
ENDIF()
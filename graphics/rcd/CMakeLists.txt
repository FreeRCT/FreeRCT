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

PROJECT(rcd_files)

GET_TARGET_PROPERTY(RCDGEN_EXE rcdgen LOCATION)

# Make declaring the list shorter/nicer
SET(FP "${CMAKE_SOURCE_DIR}/graphics/rcd/")

# Output file                     Source file
LIST(APPEND RCDFILES
  ${FP}borderhighlight_8bpp64.rcd ${FP}freerct.txt
  ${FP}cornerhighlight_8bpp64.rcd ${FP}freerct.txt
  ${FP}groundtile_8bpp64.rcd      ${FP}freerct.txt
  ${FP}people_animation.rcd       ${FP}freerct.txt
  ${FP}path_8bpp64.rcd            ${FP}paths.txt
  ${FP}shops.rcd                  ${FP}shops.txt
  ${FP}gui.rcd                    ${FP}gui.txt
  ${FP}tracks.rcd                 ${FP}tracks.txt
)

LIST(LENGTH RCDFILES RCDFILES_LENGTH)
math(EXPR RCDFILES_LENGTH_DIV_2 "${RCDFILES_LENGTH} / 2 - 1")

FOREACH(RCDFILE RANGE ${RCDFILES_LENGTH_DIV_2})
	math(EXPR INDEX_1 "${RCDFILE} * 2")
	math(EXPR INDEX_2 "${INDEX_1} + 1")
        LIST(GET RCDFILES ${INDEX_1} OUTFILE)
        LIST(GET RCDFILES ${INDEX_2} SRCFILE)

	ADD_CUSTOM_COMMAND(
	      OUTPUT ${OUTFILE}
	      COMMAND ${RCDGEN_EXE} ${SRCFILE}
	      COMMENT "Generating rcd files from ${SRCFILE}"
	)

	LIST(APPEND OUTFILES ${OUTFILE})
ENDFOREACH(RCDFILE)

add_custom_target(rcd ALL DEPENDS ${OUTFILES})

add_dependencies(rcd rcdgen)

# Warning: doesn't check for rst2html's existence
add_custom_target(data_format.html COMMAND rst2html data_format.rst data_format.html)

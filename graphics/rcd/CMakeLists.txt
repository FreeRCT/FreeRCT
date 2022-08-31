# This file is part of FreeRCT.
# FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
# FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
#

# Sanity check
IF(NOT FREERCT_DONE_TOP_LEVEL_CMAKE)
	message(FATAL_ERROR "Please run cmake on the top-level directory, not this one.")
ENDIF()

project(rcd_files)

# Make declaring the list shorter/nicer
set(FP "${CMAKE_SOURCE_DIR}/graphics/rcd")

# Get FreeRCT Markup Language files
file(GLOB_RECURSE RCDFILES RELATIVE "${FP}" "*.fml")

# Get lang files
file(GLOB LANGFILES "${FP}/lang/*.yml")

FOREACH(SRCFILE ${RCDFILES})
	get_filename_component(SRC_FILE_PATH "${SRCFILE}" DIRECTORY)
	get_filename_component(SRC_FILE_NAME "${SRCFILE}" NAME)
	string(REPLACE ".fml" ".rcd" OUTFILE ${SRC_FILE_NAME})
	string(REPLACE ".fml" ".fpp" PREPROCESSED_FILE ${SRC_FILE_NAME})

	IF(NOT "${SRC_FILE_PATH}" STREQUAL "")
		file(GLOB_RECURSE DEPENDENCIES RELATIVE "${FP}" "*.fms")
		list(FILTER DEPENDENCIES INCLUDE REGEX "${SRC_FILE_PATH}/.*")
	ELSE()
		set(DEPENDENCIES "")
	ENDIF()

	set(OUT_DIR "${FRCT_BINARY_DIR}/rcd")

	# First, use the C preprocessor to process #include directives and macros.
	add_custom_command(OUTPUT "${OUT_DIR}/${PREPROCESSED_FILE}"
			COMMAND cpp "${FP}/${SRCFILE}" "${OUT_DIR}/${PREPROCESSED_FILE}"
			COMMENT "Preprocessing ${SRCFILE}"
			DEPENDS ${SRCFILE} ${DEPENDENCIES}
			WORKING_DIRECTORY "${FP}"
	)

	# Then generate the actual RCD file.
	add_custom_command(OUTPUT "${OUT_DIR}/${OUTFILE}"
			COMMAND rcdgen ${LANGFILES} "${OUT_DIR}/${PREPROCESSED_FILE}"
			COMMAND ${CMAKE_COMMAND} -E rename ${FP}/${OUTFILE} ${OUT_DIR}/${OUTFILE}
			COMMENT "Generating RCD file from ${PREPROCESSED_FILE}"
			DEPENDS "${OUT_DIR}/${PREPROCESSED_FILE}" ${LANGFILES} rcdgen
			WORKING_DIRECTORY "${FP}"
	)

	list(APPEND OUTFILES "${OUT_DIR}/${OUTFILE}")
	set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${OUT_DIR}/${PREPROCESSED_FILE}")
	set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${OUT_DIR}/${OUTFILE}")
ENDFOREACH()

add_custom_target(rcd ALL DEPENDS ${OUTFILES})

install(DIRECTORY DESTINATION "${PACKAGING_DATA_DIR}/rcd")
install(FILES  ${OUTFILES} DESTINATION "${PACKAGING_DATA_DIR}/rcd")

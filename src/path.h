/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file path.h Path definitions. */

#ifndef PATH_H
#define PATH_H

/**
 * Available path sprites.
 * The list of sprites for drawing a path cover. Conceptually, a path can connect to each of the four
 * edges (NE, NW, SE, and SW). If both edges of a corner are present, the corner itself may also be
 * covered (N, E, S, W). This leads to #PATH_COUNT sprites listed below.
 *
 * This list is good for drawing and listing sprites, but is hard for editing path coverage.
 * For this reason #_path_expand and its reverse operation #_path_implode exist. They translate the
 * sprite number to/from a bitwise representation which is easier for manipulation.
 */
enum Paths {
	PATH_EMPTY, ///< Path without edges or corners.
	PATH_NE,
	PATH_SE,
	PATH_NE_SE,
	PATH_NE_SE_E,
	PATH_SW,
	PATH_NE_SW,
	PATH_SE_SW,
	PATH_SE_SW_S,
	PATH_NE_SE_SW,
	PATH_NE_SE_SW_E,
	PATH_NE_SE_SW_S,
	PATH_NE_SE_SW_E_S,
	PATH_NW,
	PATH_NE_NW,
	PATH_NE_NW_N,
	PATH_NW_SE,
	PATH_NE_NW_SE,
	PATH_NE_NW_SE_N,
	PATH_NE_NW_SE_E,
	PATH_NE_NW_SE_N_E,
	PATH_NW_SW,
	PATH_NW_SW_W,
	PATH_NE_NW_SW,
	PATH_NE_NW_SW_N,
	PATH_NE_NW_SW_W,
	PATH_NE_NW_SW_N_W,
	PATH_NW_SE_SW,
	PATH_NW_SE_SW_S,
	PATH_NW_SE_SW_W,
	PATH_NW_SE_SW_S_W,
	PATH_NE_NW_SE_SW,
	PATH_NE_NW_SE_SW_N,
	PATH_NE_NW_SE_SW_E,
	PATH_NE_NW_SE_SW_N_E,
	PATH_NE_NW_SE_SW_S,
	PATH_NE_NW_SE_SW_N_S,
	PATH_NE_NW_SE_SW_E_S,
	PATH_NE_NW_SE_SW_N_E_S,
	PATH_NE_NW_SE_SW_W,
	PATH_NE_NW_SE_SW_N_W,
	PATH_NE_NW_SE_SW_E_W,
	PATH_NE_NW_SE_SW_N_E_W,
	PATH_NE_NW_SE_SW_S_W,
	PATH_NE_NW_SE_SW_N_S_W,
	PATH_NE_NW_SE_SW_E_S_W,
	PATH_NE_NW_SE_SW_N_E_S_W,

	PATH_COUNT,         ///< Number of path sprites.
	PATH_INVALID = 255, ///< Invalid path.

	PATHBIT_N  = 0,     ///< Bit number for north corner in expanded notation.
	PATHBIT_E  = 1,     ///< Bit number for east corner in expanded notation.
	PATHBIT_S  = 2,     ///< Bit number for south corner in expanded notation.
	PATHBIT_W  = 3,     ///< Bit number for west corner in expanded notation.
	PATHBIT_NE = 4,     ///< Bit number for north-east edge in expanded notation.
	PATHBIT_SE = 5,     ///< Bit number for south-east edge in expanded notation.
	PATHBIT_SW = 6,     ///< Bit number for south-west edge in expanded notation.
	PATHBIT_NW = 7,     ///< Bit number for north-west edge in expanded notation.
};

extern const uint8 _path_expand[];
extern const uint8 _path_implode[];

#endif


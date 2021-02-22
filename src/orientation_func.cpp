/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file orientation_func.cpp Orientation functions. */

#include "orientation.h"

/**
 * The direction factor (\c 1 or \c -1) by which the x world coordinate changes depending on the
 * viewport orientation when stepping down a visual line that is orthogonal to the viewport.
 */
const int8 _orientation_signum_dx[VOR_NUM_ORIENT] = {
	+1, ///< VOR_NORTH
	+1, ///< VOR_EAST
	-1, ///< VOR_SOUTH
	-1, ///< VOR_WEST
};
/**
 * The direction factor (\c 1 or \c -1) by which the y world coordinate changes depending on the
 * viewport orientation when stepping down a visual line that is orthogonal to the viewport.
 */
const int8 _orientation_signum_dy[VOR_NUM_ORIENT] = {
	+1, ///< VOR_NORTH
	-1, ///< VOR_EAST
	-1, ///< VOR_SOUTH
	+1, ///< VOR_WEST
};

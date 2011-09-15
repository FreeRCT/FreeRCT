/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile.h %Tile definition.
 *
 * A tile is defined as follows
 *
 * \verbatim
 *      N     North corner is at the top,
 *     / \    west corner at the left, and east
 *    /   \   corner at the right. South corner
 *   W     E  is at the bottom of the tile.
 *    \   /
 *     \ /
 *      S
 * \endverbatim
*/

#ifndef TILE_H
#define TILE_H

/**
 * Slope description of a surface tile.
 * If not #TCB_STEEP, at most three of the four #TCB_NORTH, #TCB_EAST,
 * #TCB_SOUTH, and #TCB_WEST may be set. If #TCB_STEEP, the top corner is
 * indicated by a corner bit.
 */
enum Slope {
	SL_FLAT = 0,  ///< Flat slope.

	TC_NORTH = 0, ///< North corner bit number.
	TC_EAST,      ///< East corner bit number.
	TC_SOUTH,     ///< South corner bit number.
	TC_WEST,      ///< West corner bit number.
	TC_STEEP,     ///< Steep slope.

	TCB_NORTH = 1 << TC_NORTH, ///< Bit denoting north corner is raised.
	TCB_EAST  = 1 << TC_EAST,  ///< Bit denoting east corner is raised.
	TCB_SOUTH = 1 << TC_SOUTH, ///< Bit denoting south corner is raised.
	TCB_WEST  = 1 << TC_WEST,  ///< Bit denoting west corner is raised.

	TCB_STEEP = 1 << TC_STEEP, ///< Bit denoting it is a steep slope.
};


#endif


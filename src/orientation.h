/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file orientation.h Orientation of the viewport. */

#ifndef ORIENTATION_H
#define ORIENTATION_H

#include "tile.h"

/** Direction of view. */
enum ViewOrientation {
	VOR_NORTH = TC_NORTH, ///< View with top of the world to the north.
	VOR_EAST  = TC_EAST,  ///< View with top of the world to the east.
	VOR_SOUTH = TC_SOUTH, ///< View with top of the world to the south.
	VOR_WEST  = TC_WEST,  ///< View with top of the world to the west.

	VOR_NUM_ORIENT = 4,   ///< Number of orientations.
	VOR_INVALID = 4,      ///< Invalid orientation.
};

/**
 * Rotate view 90 degrees clockwise.
 * @param vor Input view orientation.
 * @return Rotated view orientation.
 */
static inline ViewOrientation RotateClockwise(const ViewOrientation &vor)
{
	return (ViewOrientation)((vor + 1) & 3);
}

/**
 * Rotate view 90 degrees counter clockwise.
 * @param vor Input view orientation.
 * @return Rotated view orientation.
 */
static inline ViewOrientation RotateCounterClockwise(const ViewOrientation &vor)
{
	return (ViewOrientation)((vor + 3) & 3);
}

/**
 * Add two view orientations together.
 * @param vor1 First orientation.
 * @param vor2 Second orientation.
 * @return Summed orientation.
 */
static inline ViewOrientation AddOrientations(const ViewOrientation &vor1, const ViewOrientation &vor2)
{
	return (ViewOrientation)((vor1 + vor2) & 3);
}

/**
 * Subtract two view orientations from each other.
 * @param vor1 First orientation.
 * @param vor2 Second orientation.
 * @return Resulting orientation.
 */
static inline ViewOrientation SubtractOrientations(const ViewOrientation &vor1, const ViewOrientation &vor2)
{
	return (ViewOrientation)((vor1 + 4 - vor2) & 3);
}

#endif

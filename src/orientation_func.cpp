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

/**
 * Determine at which voxel in the world a piece of a multi-voxel object should be located.
 * @param orientation Orientation of the object.
 * @param x Unrotated x coordinate of the object piece, relative to the object's base voxel.
 * @param y Unrotated y coordinate of the object piece, relative to the object's base voxel.
 * @param z Z offset coordinate.
 * @return Rotated location of the object piece, relative to the object's base voxel.
 */
XYZPoint16 OrientatedOffset(const uint8 orientation, const int x, const int y, const int z)
{
	switch (orientation % VOR_NUM_ORIENT) {
		case VOR_EAST: return XYZPoint16(x, y, z);
		case VOR_WEST: return XYZPoint16(-x, -y, z);
		case VOR_NORTH: return XYZPoint16(-y, x, z);
		case VOR_SOUTH: return XYZPoint16(y, -x, z);
	}
	NOT_REACHED();
}
/**
 * Determine at which voxel in the world a object piece should be located.
 * @param orientation Orientation of the fixed object.
 * @param x Rotated x coordinate of the object piece, relative to the object's base voxel.
 * @param x Rotated y coordinate of the object piece, relative to the object's base voxel.
 * @return Unrotated location of the object piece, relative to the object's base voxel.
 */
XYZPoint16 UnorientatedOffset(const uint8 orientation, const int x, const int y)
{
	switch (orientation % VOR_NUM_ORIENT) {
		case VOR_EAST: return XYZPoint16(x, y, 0);
		case VOR_WEST: return XYZPoint16(-x, -y, 0);
		case VOR_SOUTH: return XYZPoint16(-y, x, 0);
		case VOR_NORTH: return XYZPoint16(y, -x, 0);
	}
	NOT_REACHED();
}

/**
 * Get the zoom scale with a given tile width.
 * @param w Desired tile width.
 * @return The scale index, or \c -1 if not found.
 */
int GetZoomScaleByWidth(int w)
{
	for (int i = 0; i < ZOOM_SCALES_COUNT; ++i) if (_zoom_scales[i].tile_width == w) return i;
	return -1;
}

/**
 * Get the zoom scale with a given tile height.
 * @param w Desired tile height.
 * @return The scale index, or \c -1 if not found.
 */
int GetZoomScaleByHeight(int h)
{
	for (int i = 0; i < ZOOM_SCALES_COUNT; ++i) if (_zoom_scales[i].tile_height == h) return i;
	return -1;
}

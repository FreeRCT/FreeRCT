/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file orientation.h Orientation of the viewport. */

#ifndef ORIENTATION_H
#define ORIENTATION_H

#include "stdafx.h"
#include "tile.h"

/**
 * Direction of view.
 * @ingroup map_group
 */
enum ViewOrientation {
	VOR_NORTH = TC_NORTH, ///< View with top of the world to the north.
	VOR_EAST  = TC_EAST,  ///< View with top of the world to the east.
	VOR_SOUTH = TC_SOUTH, ///< View with top of the world to the south.
	VOR_WEST  = TC_WEST,  ///< View with top of the world to the west.

	VOR_NUM_ORIENT = 4,   ///< Number of orientations.
	VOR_INVALID = 0xFF,   ///< Invalid orientation.
};

extern const int8 _orientation_signum_dx[VOR_NUM_ORIENT];
extern const int8 _orientation_signum_dy[VOR_NUM_ORIENT];

/**
 * Rotate view 90 degrees clockwise.
 * @param vor Input view orientation.
 * @return Rotated view orientation.
 * @ingroup map_group
 */
static inline ViewOrientation RotateClockwise(const ViewOrientation &vor)
{
	return (ViewOrientation)((vor + 1) & 3);
}

/**
 * Rotate view 90 degrees counter clockwise.
 * @param vor Input view orientation.
 * @return Rotated view orientation.
 * @ingroup map_group
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
 * @ingroup map_group
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
 * @ingroup map_group
 */
static inline ViewOrientation SubtractOrientations(const ViewOrientation &vor1, const ViewOrientation &vor2)
{
	return (ViewOrientation)((vor1 + 4 - vor2) & 3);
}

/**
 * Works out if a tile edge is at the back of tile, depending on orientation.
 * @param orient Orientation of the viewport.
 * @param edge The tile edge to test.
 * @return If edge is at the back of a tile.
 */
static inline bool IsBackEdge(ViewOrientation orient, TileEdge edge)
{
	static const uint16 BACK = (((1u << VOR_NORTH) | (1u << VOR_WEST)) << (EDGE_NW * 4)) |
			(((1u << VOR_NORTH) | (1u << VOR_EAST)) << (EDGE_NE * 4)) |
			(((1u << VOR_SOUTH) | (1u << VOR_EAST)) << (EDGE_SE * 4)) |
			(((1u << VOR_SOUTH) | (1u << VOR_WEST)) << (EDGE_SW * 4));

	return (BACK & (1u << (orient + 4 * edge))) != 0;
}

XYZPoint16 OrientatedOffset(const uint8 orientation, const int x, const int y, const int z = 0);
XYZPoint16 UnorientatedOffset(const uint8 orientation, const int x, const int y);

/** Information about the graphics sizes at a zoom scale. */
struct ZoomScale {
	/**
	 * Constructor.
	 * @param h Tile height.
	 * @note Tile width is computed automatically.
	 */
	constexpr ZoomScale(int h) : tile_width(4 * h), tile_height(h)
	{
	}

	int tile_width;   ///< Width  of a tile in pixels.
	int tile_height;  ///< Height of a tile in pixels.
};

/** Available zoom scales, sorted from smallest to biggest. */
constexpr ZoomScale _zoom_scales[] = {
	{ 4},
	{ 8},
	{16},
	{24},
	{32},
	{48},
	{64},
};
constexpr int ZOOM_SCALES_COUNT = lengthof(_zoom_scales);  ///< Number of available zoom scales.
constexpr int DEFAULT_ZOOM = 2;                            ///< Default zoom scale index.

int GetZoomScaleByWidth(int w);
int GetZoomScaleByHeight(int h);

/**
 * Get the tile width at a zoom scale.
 * @param zoom Zoom scale index.
 * @return The tile width.
 */
inline int TileWidth(int zoom)
{
	return _zoom_scales[zoom].tile_width;
}

/**
 * Get the tile height at a zoom scale.
 * @param zoom Zoom scale index.
 * @return The tile height.
 */
inline int TileHeight(int zoom)
{
	return _zoom_scales[zoom].tile_height;
}

#endif

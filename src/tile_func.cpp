/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tile_func.cpp Tile functions. */

#include "stdafx.h"
#include "tile.h"

/** Tile corners adjacent to an edge. */
const uint8 _corners_at_edge[EDGE_COUNT] = {
	TSB_NORTHEAST, ///< EDGE_NE
	TSB_SOUTHEAST, ///< EDGE_SE
	TSB_SOUTHWEST, ///< EDGE_SW
	TSB_NORTHWEST, ///< EDGE_NW
};

/** Corner dx/dy (relative to northern corner) of a corner of a tile. */
const Point16 _corner_dxy[4] = {
	{0, 0}, ///< TC_NORTH
	{0, 1}, ///< TC_EAST
	{1, 1}, ///< TC_SOUTH
	{1, 0}, ///< TC_WEST
};

/** Tile dx/dy of the tile connected to the given edge. */
const Point16 _tile_dxy[EDGE_COUNT] = {
	{-1,  0}, ///< EDGE_NE
	{ 0,  1}, ///< EDGE_SE
	{ 1,  0}, ///< EDGE_SW
	{ 0, -1}, ///< EDGE_NW
};

/**
 * Compute the height of the corners of an expanded ground tile.
 * @param slope Expanded slope.
 * @param base_height Height of the voxel containing the ground.
 * @param output [out] Height of the four corners of the slope.
 */
void ComputeCornerHeight(TileSlope slope, uint8 base_height, uint8 *output)
{
	if ((slope & TSB_STEEP) != 0) {
		uint i;
		for (i = TC_NORTH; i < TC_END; i++) {
			if ((slope & (1 << i)) != 0) break;
		}
		assert(i < TC_END);

		if ((slope & TSB_TOP) != 0) base_height--; // Move to the base height.

		output[i] = base_height + 2;
		output[(i + 2) % 4] = base_height;
		output[(i + 1) % 4] = base_height + 1;
		output[(i + 3) % 4] = base_height + 1;
		return;
	}
	for (uint i = TC_NORTH; i < TC_END; i++) {
		output[i] = ((slope & (1 << i)) == 0) ? base_height : base_height + 1;
	}
}

/**
 * Compute a tile slope and a base height from the height of the four corners.
 * @param corners [in] Array with the corner heights.
 * @param slope [out] Computed slope. For steep slopes, the base value is given.
 * @param base [out] Base height of the slope.
 */
void ComputeSlopeAndHeight(uint8 *corners, TileSlope *slope, uint8 *base)
{
	uint8 max_h = 0;
	uint8 min_h = 255;
	for (uint i = 0; i < 4; i++) {
		if (max_h < corners[i]) max_h = corners[i];
		if (min_h > corners[i]) min_h = corners[i];
	}
	*base = min_h;

	if (max_h - min_h <= 1) {
		/* Normal slope. */
		*slope = static_cast<TileSlope>(ImplodeTileSlope(((corners[TC_NORTH] > min_h) ? TSB_NORTH : SL_FLAT)
				| ((corners[TC_EAST]  > min_h) ? TSB_EAST  : SL_FLAT)
				| ((corners[TC_SOUTH] > min_h) ? TSB_SOUTH : SL_FLAT)
				| ((corners[TC_WEST]  > min_h) ? TSB_WEST  : SL_FLAT)));
	} else {
		assert(max_h - min_h == 2);
		uint i;
		for (i = 0; i < 4; i++) {
			if (corners[i] == max_h) break;
		}
		*slope = static_cast<TileSlope>(ImplodeTileSlope(TSB_STEEP | static_cast<TileSlope>(1 << i)));
	}
}

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
	TCB_NORTHEAST, ///< EDGE_NE
	TCB_SOUTHEAST, ///< EDGE_SE
	TCB_SOUTHWEST, ///< EDGE_SW
	TCB_NORTHWEST, ///< EDGE_NW
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

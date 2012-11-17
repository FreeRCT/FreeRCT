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

#include "enum_type.h"
#include "geometry.h"

/**
 * Slope description of a surface tile.
 * If not #TCB_STEEP, at most three of the four #TCB_NORTH, #TCB_EAST,
 * #TCB_SOUTH, and #TCB_WEST may be set. If #TCB_STEEP, the top corner is
 * indicated by a corner bit.
 * @ingroup map_group
 */
enum TileSlope {
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

	TCB_NORTHEAST = TCB_NORTH | TCB_EAST, ///< Both north and east corners are raised.
	TCB_NORTHWEST = TCB_NORTH | TCB_WEST, ///< Both north and west corners are raised.
	TCB_SOUTHEAST = TCB_SOUTH | TCB_EAST, ///< Both south and east corners are raised.
	TCB_SOUTHWEST = TCB_SOUTH | TCB_WEST, ///< Both south and west corners are raised.

};
DECLARE_ENUM_AS_BIT_SET(TileSlope)

static const uint8 NUM_SLOPE_SPRITES = 19; ///< Number of sprites for defining a surface tile.

/**
 * Edges of a tile, starts at NE direction, and rotates clockwise.
 * @ingroup map_group
 */
enum TileEdge {
	EDGE_BEGIN, ///< First valid edge.
	EDGE_NE = EDGE_BEGIN, ///< North-east edge.
	EDGE_SE,    ///< South-east edge.
	EDGE_SW,    ///< South-west edge.
	EDGE_NW,    ///< North-west edge.

	EDGE_COUNT, ///< Number of edges.
	INVALID_EDGE = EDGE_COUNT, ///< Invalid edge (to denote lack of an edge).
};
DECLARE_POSTFIX_INCREMENT(TileEdge)


/**
 * Expand a slope sprite number to its bit-encoded form for easier manipulating.
 * @param v %Sprite slope number.
 * @return Expanded slope.
 * @ingroup map_group
 */
inline TileSlope ExpandTileSlope(uint8 v)
{
	if (v < 15) return (TileSlope)v;
	return TCB_STEEP | (TileSlope)(1 << (v-15));
}

/**
 * Implode an expanded slope back to its sprite number.
 * @param s Expanded slope to implode.
 * @return Equivalent sprite number.
 * @ingroup map_group
 */
inline uint8 ImplodeTileSlope(TileSlope s)
{
	if ((s & TCB_STEEP) == 0) return s;
	if ((s & TCB_NORTH) != 0) return 15;
	if ((s & TCB_EAST)  != 0) return 16;
	if ((s & TCB_SOUTH) != 0) return 17;
	return 18;
}

/**
 * Available ground types.
 * @ingroup map_group
 */
enum GroundType {
	GTP_GRASS0,      ///< Short grass type.
	GTP_GRASS1,      ///< Normal grass type.
	GTP_GRASS2,      ///< Long grass type.
	GTP_GRASS3,      ///< Rough grass type.
	GTP_DESERT,      ///< Desert ground type.
	GTP_CURSOR_TEST, ///< Test tile for hit-testing of ground-tiles.

	GTP_COUNT,       ///< Number of ground types.

	GTP_INVALID = GTP_COUNT, ///< Invalid ground type.
};

/**
 * Types of foundations.
 * @ingroup map_group
 */
enum FoundationType {
	FDT_GROUND, ///< Bare (ground) foundation type.
	FDT_WOOD,   ///< %Foundation is covered with wood.
	FDT_BRICK,  ///< %Foundation is made of bricks.

	FDT_COUNT,  ///< Number of foundation types.

	FDT_INVALID = FDT_COUNT, ///< Invalid foundation type.
};

/**
 * Path and track slopes.
 * @ingroup map_group
 */
enum TrackSlope {
	TSL_BEGIN,                           ///< First track slope.
	TSL_DOWN = TSL_BEGIN,                ///< Gently down.
	TSL_FLAT,                            ///< Horizontal slope.
	TSL_UP,                              ///< Gently up.
	TSL_COUNT_GENTLE,                    ///< Number of gentle slopes.

	TSL_STEEP_DOWN = TSL_COUNT_GENTLE,   ///< Steeply downwards.
	TSL_STEEP_UP,                        ///< Steeply upwards.
	TSL_COUNT_STEEP,                     ///< Number of gentle and steep slopes.

	TSL_STRAIGHT_DOWN = TSL_COUNT_STEEP, ///< Vertically down.
	TSL_STRAIGHT_UP,                     ///< Vertically up.
	TSL_COUNT_VERTICAL,                  ///< Number of slopes if also going straight up and down.

	TSL_INVALID = TSL_COUNT_VERTICAL,    ///< Invalid track slope.
};
DECLARE_POSTFIX_INCREMENT(TrackSlope)

extern const uint8 _corners_at_edge[EDGE_COUNT];
extern const Point16 _corner_dxy[4];
extern const Point16 _tile_dxy[EDGE_COUNT];

#endif


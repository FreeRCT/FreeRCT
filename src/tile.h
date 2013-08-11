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

/** Corners of a tile. */
enum TileCorner {
	TC_NORTH = 0, ///< North corner.
	TC_EAST,      ///< East corner.
	TC_SOUTH,     ///< South corner.
	TC_WEST,      ///< West corner.

	TC_END,       ///< End of all corners.

	TCB_NORTH = 1 << TC_NORTH, ///< Bit denoting north corner.
	TCB_EAST  = 1 << TC_EAST,  ///< Bit denoting east corner.
	TCB_SOUTH = 1 << TC_SOUTH, ///< Bit denoting south corner.
	TCB_WEST  = 1 << TC_WEST,  ///< Bit denoting west corner.
};

/**
 * Slope description of a surface tile.
 * - If not #TSB_STEEP, at most three of the four #TSB_NORTH, #TSB_EAST, * #TSB_SOUTH, and #TSB_WEST may be set.
 * - If #TSB_STEEP, the top corner is indicated by a corner bit.
 *   Note that steep slopes have 2 sprites. The #TSB_TOP bit differentiates between both.
 *
 * @ingroup map_group
 */
enum TileSlope {
	SL_FLAT = 0,  ///< Flat slope.

	TS_NORTH = TC_NORTH,   ///< North corner bit number.
	TS_EAST  = TC_EAST,    ///< East corner bit number.
	TS_SOUTH = TC_SOUTH,   ///< South corner bit number.
	TS_WEST  = TC_WEST,    ///< West corner bit number.
	TS_STEEP = TC_END,     ///< Steep slope bit number.
	TS_TOP   = TC_END + 1, ///< Top-part of a steep slope bit number.

	TSB_NORTH = 1 << TS_NORTH, ///< Bit denoting north corner is raised.
	TSB_EAST  = 1 << TS_EAST,  ///< Bit denoting east corner is raised.
	TSB_SOUTH = 1 << TS_SOUTH, ///< Bit denoting south corner is raised.
	TSB_WEST  = 1 << TS_WEST,  ///< Bit denoting west corner is raised.

	TSB_STEEP = 1 << TS_STEEP, ///< Bit denoting it is a steep slope.
	TSB_TOP   = 1 << TS_TOP,   ///< Bit denoting it is the top of a steep slope.

	TSB_NORTHEAST = TSB_NORTH | TSB_EAST, ///< Both north and east corners are raised.
	TSB_NORTHWEST = TSB_NORTH | TSB_WEST, ///< Both north and west corners are raised.
	TSB_SOUTHEAST = TSB_SOUTH | TSB_EAST, ///< Both south and east corners are raised.
	TSB_SOUTHWEST = TSB_SOUTH | TSB_WEST, ///< Both south and west corners are raised.

};
DECLARE_ENUM_AS_BIT_SET(TileSlope)

static const uint8 NUM_SLOPE_SPRITES = 15 + 4 + 4; ///< Number of sprites for defining a surface tile.

/** Sprites for supports available for use. */
enum SupportSprites {
	SSP_FLAT_SINGLE_NS, ///< Single height for flat terrain, north and south view.
	SSP_FLAT_SINGLE_EW, ///< Single height for flat terrain, east and west view.
	SSP_FLAT_DOUBLE_NS, ///< Double height for flat terrain, north and south view.
	SSP_FLAT_DOUBLE_EW, ///< Double height for flat terrain, east and west view.
	SSP_FLAT_PATH_NS,   ///< Double height for paths, north and south view.
	SSP_FLAT_PATH_EW,   ///< Double height for paths, east and west view.
	SSP_N,              ///< Single height, north leg up.
	SSP_E,              ///< Single height, east leg up.
	SSP_NE,             ///< Single height, north, east legs up.
	SSP_S,              ///< Single height, south leg up.
	SSP_NS,             ///< Single height, north, south legs up.
	SSP_ES,             ///< Single height, east, south legs up.
	SSP_NES,            ///< Single height, north, east, south legs up.
	SSP_W,              ///< Single height, west leg up.
	SSP_NW,             ///< Single height, west, north legs up.
	SSP_EW,             ///< Single height, west, east legs up.
	SSP_NEW,            ///< Single height, west, north, east legs up.
	SSP_SW,             ///< Single height, west, south legs up.
	SSP_NSW,            ///< Single height, west, north, south legs up.
	SSP_ESW,            ///< Single height, west, east, south legs up.
	SSP_STEEP_N,        ///< Double height for steep north slope.
	SSP_STEEP_E,        ///< Double height for steep east slope.
	SSP_STEEP_S,        ///< Double height for steep south slope.
	SSP_STEEP_W,        ///< Double height for steep west slope.

	SSP_COUNT,          ///< Number of support sprites.
	SSP_NONFLAT_BASE = SSP_FLAT_PATH_EW, ///< Base sprite for non-flat ground slopes.
};

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

	EDGE_ALL = 0xF, ///< Bit set of all edges.

	INVALID_EDGE = 0xFF, ///< Invalid edge (to denote lack of an edge).
};
DECLARE_POSTFIX_INCREMENT(TileEdge)

/**
 * Does the imploded tile slope represent a steep slope?
 * @param ts Imploded tile slope to examine.
 * @return \c true if the given slope is a steep slope, \c false otherwise.
 */
static inline bool IsImplodedSteepSlope(uint8 ts)
{
	return ts >= 15;
}

/**
 * Does the imploded tile slope represent the top of a steep slope?
 * @param ts Imploded tile slope to examine.
 * @return \c true if the given slope is the top of a steep slope, \c false otherwise.
 */
static inline bool IsImplodedSteepSlopeTop(uint8 ts)
{
	return ts >= 19;
}

/**
 * Expand a slope sprite number to its bit-encoded form for easier manipulating.
 * @param v %Sprite slope number.
 * @return Expanded slope.
 * @ingroup map_group
 */
static inline TileSlope ExpandTileSlope(uint8 v)
{
	if (v < 15) return (TileSlope)v;
	if (v < 19) return TSB_STEEP | (TileSlope)(1 << (v-15));
	return TSB_STEEP | TSB_TOP | (TileSlope)(1 << (v-19));
}

/**
 * Implode an expanded slope back to its sprite number.
 * @param s Expanded slope to implode.
 * @return Equivalent sprite number.
 * @ingroup map_group
 */
static inline uint8 ImplodeTileSlope(TileSlope s)
{
	if ((s & TSB_STEEP) == 0) return s;

	uint8 base = ((s & TSB_TOP) == 0) ? 15 : 19;
	if ((s & TSB_NORTH) != 0) return base;
	if ((s & TSB_EAST)  != 0) return base + 1;
	if ((s & TSB_SOUTH) != 0) return base + 2;
	return base + 3;
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

	GTP_INVALID = 0xF, ///< Invalid ground type.
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

	FDT_INVALID = 0xF, ///< Invalid foundation type.
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

	TSL_INVALID = 0xFF,                  ///< Invalid track slope.
};
DECLARE_POSTFIX_INCREMENT(TrackSlope)

extern const uint8 _corners_at_edge[EDGE_COUNT];
extern const Point16 _corner_dxy[4];
extern const Point16 _tile_dxy[EDGE_COUNT];

void ComputeCornerHeight(TileSlope slope, uint8 base_height, uint8 *output);
void ComputeSlopeAndHeight(uint8 *corners, TileSlope *slope, uint8 *base);

#endif


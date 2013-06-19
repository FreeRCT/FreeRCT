/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file path.h %Path definitions. */

#ifndef PATH_H
#define PATH_H

#include "tile.h"

/**
 * Available path sprites.
 * The list of sprites for drawing a path cover. Conceptually, a path can connect to each of the four
 * edges (NE, NW, SE, and SW). If both edges of a corner are present, the corner itself may also be
 * covered (N, E, S, W). This leads to #PATH_FLAT_COUNT sprites listed below.
 *
 * This list is good for drawing and listing sprites, but is hard for editing path coverage.
 * For this reason #_path_expand and its reverse operation #_path_implode exist. They translate the
 * sprite number to/from a bitwise representation which is easier for manipulation.
 * @todo Have ramps with path sprites without edges.
 * @ingroup map_group
 */
enum PathSprites {
	PATH_EMPTY, ///< %Path without edges or corners.
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
	PATH_FLAT_COUNT,    ///< Number of flat path sprites.

	PATH_RAMP_NE = PATH_FLAT_COUNT, ///< Ramp from NE up to SW.
	PATH_RAMP_NW,                   ///< Ramp from NW up to SE.
	PATH_RAMP_SE,                   ///< Ramp from SE up to NW.
	PATH_RAMP_SW,                   ///< Ramp from SW up to NE.
	PATH_COUNT,                     ///< Number of path sprites.

	PATH_INVALID = 63,  ///< Invalid path. Also used to indicate reserved voxels above paths.

	PATHBIT_N  = 0,     ///< Bit number for north corner in expanded notation.
	PATHBIT_E  = 1,     ///< Bit number for east corner in expanded notation.
	PATHBIT_S  = 2,     ///< Bit number for south corner in expanded notation.
	PATHBIT_W  = 3,     ///< Bit number for west corner in expanded notation.
	PATHBIT_NE = 4,     ///< Bit number for north-east edge in expanded notation.
	PATHBIT_SE = 5,     ///< Bit number for south-east edge in expanded notation.
	PATHBIT_SW = 6,     ///< Bit number for south-west edge in expanded notation.
	PATHBIT_NW = 7,     ///< Bit number for north-west edge in expanded notation.
};

/**
 * Available path types.
 * @ingroup map_group
 */
enum PathRideType {
	PT_INVALID = 0x3FF,     ///< Invalid path type and invalid ride type.
	PT_CONCRETE = 0x3FE,    ///< Concrete path type.
	PT_START = PT_CONCRETE, ///< First valid path type.
	/* Values 0 .. PT_START-1 are ride instance numbers. */
};

extern const PathSprites _path_up_from_edge[EDGE_COUNT];
extern const PathSprites _path_down_from_edge[EDGE_COUNT];
extern const uint8 _path_expand[];
extern const uint8 _path_implode[256];
extern const uint8 _path_rotation[PATH_COUNT][4];

bool PathExistsAtBottomEdge(int xpos, int ypos, int zpos, TileEdge edge);

uint8 SetPathEdge(uint8 slope, TileEdge edge, bool connect);
uint8 GetPathSprite(TrackSlope tsl, TileEdge edge);
uint8 AddRemovePathEdges(uint16 xpos, uint16 ypos, uint8 zpos, uint8 slope, uint8 dirs, bool use_additions, bool add_edges);

#endif


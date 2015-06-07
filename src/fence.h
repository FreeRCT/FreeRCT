/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fence.h %Fence declarations. */

#ifndef FENCE_H
#define FENCE_H

/**
 * Available fence sprites.
 */
enum FenceSprites {
	FENCE_NE_FLAT,     ///< Fence on north east edge with flat ground.
	FENCE_NE_N_RAISED, ///< Fence on north east edge with north corner raised.
	FENCE_NE_E_RAISED, ///< Fence on north east edge with east corner raised.
	FENCE_SE_FLAT,     ///< Fence on south east edge with flat ground.
	FENCE_SE_E_RAISED, ///< Fence on south east edge with east corner raised.
	FENCE_SE_S_RAISED, ///< Fence on south east edge with south corner raised.
	FENCE_SW_FLAT,     ///< Fence on south west edge with flat ground.
	FENCE_SW_S_RAISED, ///< Fence on south west edge with south corner raised.
	FENCE_SW_W_RAISED, ///< Fence on south west edge with west corner raised.
	FENCE_NW_FLAT,     ///< Fence on north west edge with flat ground.
	FENCE_NW_W_RAISED, ///< Fence on north west edge with west corner raised.
	FENCE_NW_N_RAISED, ///< Fence on north west edge with north corner raised.

	FENCE_COUNT, ///< Number of fence sprites.

	FENCE_EDGE_COUNT = 3, ///< Number of fence sprites for each edge.
};

/**
 * Available fence types.
 * @ingroup map_group
 */
enum FenceType {
	FENCE_TYPE_LAND_BORDER,     ///< Special fence for the border of owned land
	FENCE_TYPE_BUILDABLE_BEGIN, ///< Fence types >= this value can be built in the game
	FENCE_TYPE_WOODEN = FENCE_TYPE_BUILDABLE_BEGIN, ///< Wooden fence
	FENCE_TYPE_CONIFER_HEDGE,   ///< Conifer hedge
	FENCE_TYPE_BRICK_WALL,      ///< Brick wall

	FENCE_TYPE_COUNT,           ///< Number of fence types.
	FENCE_TYPE_INVALID = 0xF,   ///< Value representing an invalid/non-existing fence type
};

/** %Fence data with #FENCE_TYPE_INVALID at all four edges. */
static const uint16 ALL_INVALID_FENCES = FENCE_TYPE_INVALID | (FENCE_TYPE_INVALID << 4) | (FENCE_TYPE_INVALID << 8) | (FENCE_TYPE_INVALID << 12);

#endif

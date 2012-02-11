/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file path_build.h %Path building manager declarations. */

#ifndef PATH_BUILD_H
#define PATH_BUILD_H

#include "tile.h"

struct SurfaceVoxelData;

/** Possible states of the path building process. */
enum PathBuildState {
	PBS_IDLE,       ///< Waiting for the path-gui to be opened.
	PBS_WAIT_VOXEL, ///< Wait for a voxel to be selected.
	PBS_WAIT_ARROW, ///< Wait for an arrow direction to be selected.
	PBS_WAIT_SLOPE, ///< Wait for a slope to be selected.
	PBS_WAIT_BUY,   ///< Path-building has a tile and a slope proposed in #_additions, wait for 'buy'.
	PBS_LONG_BUILD, ///< Build a 'long' path.
	PBS_LONG_BUY,   ///< Wait for buying the long path.
};

/** Helper for storing data and state about the path building process. */
class PathBuildManager {
public:
	PathBuildManager();

	void Reset();
	void SetPathGuiState(bool opened);

	void TileClicked(uint16 xpos, uint16 ypos, uint8 zpos);
	void SelectArrow(TileEdge direction);
	void SelectSlope(TrackSlope slope);
	void SelectMovement(bool move_forward);
	void SelectLong();
	void SelectBuyRemove(bool buy);

	inline uint8 GetAllowedArrows() const;
	inline TileEdge GetSelectedArrow() const;
	inline uint8 GetAllowedSlopes() const;
	inline TrackSlope GetSelectedSlope() const;
	inline bool GetBuyIsEnabled() const;
	       bool GetRemoveIsEnabled() const;
	inline bool GetForwardIsEnabled() const;
	inline bool GetBackwardIsEnabled() const;

	PathBuildState state;      ///< State of the path building process.

private:
	uint16 xpos;               ///< X coordinate of the selected voxel.
	uint16 ypos;               ///< Y coordinate of the selected voxel.
	uint8  zpos;               ///< Z coordinate of the selected voxel.
	uint8 allowed_arrows;      ///< Allowed build directions from the voxel (bitset of #TileEdge, lower 4 bits are at bottom, upper 4 bits at top).
	TileEdge selected_arrow;   ///< Last selected build direction (#INVALID_EDGE if none has been chosen).
	uint8 allowed_slopes;      ///< Allowed slope directions for the build direction.
	TrackSlope selected_slope; ///< Last selected slope (#TSL_INVALID if none has been chosen).

	void ComputeArrowCursorPosition(uint16 *xpos, uint16 *ypos, uint8 *zpos);
	bool ComputeWorldAdditions(SurfaceVoxelData *svd, uint16 *xpos, uint16 *ypos, uint8 *zpos);
	bool TryMove(TileEdge direction, int delta_z, bool need_path);
	void MoveCursor(TileEdge edge, bool move_up);
	void UpdateState();
};

/**
 * Get the allowed directions of building from the selected tile.
 * @return Allowed directions as a bitset of tile edges.
 */
inline uint8 PathBuildManager::GetAllowedArrows() const
{
	if (this->state < PBS_WAIT_ARROW || this->state > PBS_WAIT_BUY) return 0;
	return this->allowed_arrows | (this->allowed_arrows >> 4);
}

/**
 * Get the selected arrow, if available.
 * @return The last selected arrow, or #INVALID_EDGE if none is available.
 */
inline TileEdge PathBuildManager::GetSelectedArrow() const
{
	if (this->state <= PBS_WAIT_ARROW || this->state > PBS_WAIT_BUY) return INVALID_EDGE;
	return this->selected_arrow;
}

/**
 * Get the allowed directions of building from the selected tile.
 * @return Allowed directions as a bitset of tile edges.
 */
inline uint8 PathBuildManager::GetAllowedSlopes() const
{
	if (this->state < PBS_WAIT_SLOPE || this->state > PBS_WAIT_BUY) return 0;
	return this->allowed_slopes;
}

/**
 * Get the selected slope, if available.
 * @return The last selected slope, or #TSL_INVALID if no valid slope is available.
 */
inline TrackSlope PathBuildManager::GetSelectedSlope() const
{
	if (this->state <= PBS_WAIT_SLOPE || this->state > PBS_WAIT_BUY) return TSL_INVALID;
	return this->selected_slope;
}

/**
 * Can the user press the 'buy' button at the path gui?
 * @return Button is enabled.
 */
inline bool PathBuildManager::GetBuyIsEnabled() const
{
	return this->state == PBS_WAIT_BUY;
}

/**
 * Can the user press the 'forward' button at the path gui?
 * @return Button is enabled.
 */
inline bool PathBuildManager::GetForwardIsEnabled() const
{
	return this->state > PBS_WAIT_ARROW && this->state <= PBS_WAIT_BUY;
}

/**
 * Can the user press the 'back' button at the path gui?
 * @return Button is enabled.
 */
inline bool PathBuildManager::GetBackwardIsEnabled() const
{
	return this->state > PBS_WAIT_ARROW && this->state <= PBS_WAIT_BUY;
}

extern PathBuildManager _path_builder;

#endif

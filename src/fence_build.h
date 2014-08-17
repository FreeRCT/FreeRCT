/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fence_build.h %Fence building manager declarations. */

#ifndef FENCE_BUILD_H
#define FENCE_BUILD_H

#include "viewport.h"
#include "fence.h"

struct SurfaceVoxelData;
class Viewport;

/** State of the fence build manager. */
enum FenceBuildState {
	FBS_OFF,      ///< %Window closed.
	FBS_NO_MOUSE, ///< %Window opened, but no mouse mode active.
	FBS_ON,       ///< Active.
};

/** Helper for storing data and state about the fence building process. */
class FenceBuildManager : public MouseMode {
public:
	FenceBuildManager();

	void OpenWindow();
	void CloseWindow();

	void SetCursors();

	bool MayActivateMode() override;
	void ActivateMode(const Point16 &pos) override;
	void LeaveMode() override;
	bool EnableCursors() override;

	void OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos) override;
	void OnMouseButtonEvent(Viewport *vp, uint8 state) override;
	void OnMouseWheelEvent(Viewport *vp, int direction) override;

	void SelectFenceType(FenceType fence_type);
	inline FenceType GetSelectedFenceType() const;


private:
	FenceBuildState state;     ///< Own state.
	uint8 mouse_state;         ///< State of the mouse buttons.

	uint16 xpos;               ///< X coordinate of the selected voxel.
	uint16 xlong;              ///< X coordinate of the long fence destination voxel.
	uint16 ypos;               ///< Y coordinate of the selected voxel.
	uint16 ylong;              ///< Y coordinate of the long fence destination voxel.
	uint8  zpos;               ///< Z coordinate of the selected voxel.
	uint8  zlong;              ///< Z coordinate of the long fence destination voxel.

	FenceType selected_fence_type; ///< Selected fence type or FENCE_TYPE_INVALID if no fence type is selected
};

/**
 * Get the selected fence type
 * @return The selected fence type or FENCE_TYPE_INVALID if no fence type is selected.
 */
inline FenceType FenceBuildManager::GetSelectedFenceType() const
{
	return this->selected_fence_type;
}

extern FenceBuildManager _fence_builder;

#endif

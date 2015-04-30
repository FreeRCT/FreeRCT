/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fence_build.cpp %Fence building manager code. */

#include "stdafx.h"
#include "fence_build.h"
#include "map.h"
#include "window.h"
#include "math_func.h"
#include "gamemode.h"
#include <math.h>

FenceBuildManager _fence_builder; ///< %Fence build manager.

/** Default constructor. */
FenceBuildManager::FenceBuildManager() : MouseMode(WC_FENCE, MM_FENCE_BUILDING)
{
	this->state = FBS_OFF;
	this->mouse_state = 0;
	this->selected_fence_type = FENCE_TYPE_INVALID;
}

/** Fence GUI window just opened. */
void FenceBuildManager::OpenWindow()
{
	if (this->state == FBS_OFF) {
		this->state = FBS_NO_MOUSE;
		_mouse_modes.SetMouseMode(this->mode);
	}
}

/** Fence GUI window just closed. */
void FenceBuildManager::CloseWindow()
{
	if (this->state == FBS_ON) {
		this->state = FBS_OFF; // Prevent enabling again.
		_mouse_modes.SetViewportMousemode();
	}
	this->state = FBS_OFF; // In case the mouse mode was not active.
}

bool FenceBuildManager::MayActivateMode()
{
	return this->state != FBS_OFF;
}

/**
 * Restart the fence build interaction sequence.
 * @param pos New mouse position.
 * @return The mouse mode should be enabled.
 */
void FenceBuildManager::ActivateMode(const Point16 &pos)
{
	this->mouse_state = 0;
	this->state = FBS_ON;
	this->SetCursors();
}


/** Set/modify the cursors of the viewport. */
void FenceBuildManager::SetCursors()
{
	Viewport *vp = GetViewport();
	if (vp == nullptr) return;

	FinderData fdata(CS_GROUND_EDGE, FW_EDGE);
	if (vp->ComputeCursorPosition(&fdata) != CS_NONE && fdata.cursor >= CUR_TYPE_EDGE_NE && fdata.cursor <= CUR_TYPE_EDGE_NW) {
		const Voxel *v = _world.GetVoxel(fdata.voxel_pos);
		assert(v->GetGroundType() != GTP_INVALID);
		if (IsImplodedSteepSlopeTop(v->GetGroundSlope())) fdata.voxel_pos.z--; // Select base of the ground for the edge cursor.
		vp->edge_cursor.SetCursor(fdata.voxel_pos, fdata.cursor);
	}
}

/** Notification that the mouse mode has been disabled. */
void FenceBuildManager::LeaveMode()
{
	Viewport *vp = GetViewport();
	if (vp != nullptr) {
		vp->tile_cursor.SetInvalid();
		vp->area_cursor.SetInvalid();
	}
	if (this->state == FBS_ON) this->state = FBS_NO_MOUSE;
}

bool FenceBuildManager::EnableCursors()
{
	return this->state != FBS_OFF;
}

void FenceBuildManager::OnMouseWheelEvent(Viewport *vp, int direction)
{
}

void FenceBuildManager::OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos)
{
	this->SetCursors();
}

void FenceBuildManager::OnMouseButtonEvent(Viewport *vp, uint8 state)
{
	this->mouse_state = state & MB_CURRENT;

	if ((this->mouse_state & MB_LEFT) != 0) { // Left-click -> build fence
		EdgeCursor *c = &vp->edge_cursor;
		if (_game_mode_mgr.InPlayMode() && _world.GetTileOwner(c->cursor_pos.x, c->cursor_pos.y) != OWN_PARK) return;

		TileEdge edge = (TileEdge)((uint8)c->type - (uint8)CUR_TYPE_EDGE_NE);
		VoxelStack *vs = _world.GetModifyStack(c->cursor_pos.x, c->cursor_pos.y);
		uint16 fences = GetGroundFencesFromMap(vs, c->cursor_pos.z);
		fences = SetFenceType(fences, edge, this->selected_fence_type);
		AddGroundFencesToMap(fences, vs, c->cursor_pos.z);
		vp->MarkVoxelDirty(c->cursor_pos);
	}
}

/**
 * Set the selected fence type.
 * @param fence_type Fence type to select or #FENCE_TYPE_INVALID.
 */
void FenceBuildManager::SelectFenceType(FenceType fence_type)
{
	this->selected_fence_type = fence_type;
	this->SetCursors();
}


/* $Id$ */

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
		TileEdge edge = (TileEdge)((uint8)fdata.cursor - (uint8)CUR_TYPE_EDGE_NE);
		const Voxel *v = _world.GetVoxel(fdata.voxel_pos.x, fdata.voxel_pos.y, fdata.voxel_pos.z);
		assert(v != nullptr);
		TileSlope slope = ExpandTileSlope(v->GetGroundSlope());
		/*
		 * Fence for steep slopes and fully raised edges go into the voxel above.
		 * Click on steep slope already have the upper edge z given in fdata, but
		 * for clicking on upper edge, we need to get the voxel above the one
		 * clicked.
		 */
		int32 extra_z = 0;
		if ((slope & TSB_TOP) == 0 && IsRaisedEdge(edge, slope)) {
			extra_z = 1;
			v = _world.GetCreateVoxel(fdata.voxel_pos.x, fdata.voxel_pos.y, fdata.voxel_pos.z + extra_z, true);
			assert(v != nullptr);
		}
		const SpriteStorage *ss = _sprite_manager.GetSprites(vp->tile_width);
		assert(ss != nullptr);
		const ImageData *sprite = ss->GetFenceSprite(this->selected_fence_type, edge, ExpandTileSlope(v->GetGroundSlope()), vp->orientation);
		/*
		 * The lower edges of steep slope needs an yoffset to be rendered
		 * as if they belonged to the lower voxel.
		 */
		bool steep_lower_edge = (slope & TSB_STEEP) != 0 &&
			(slope & (1 << edge)) == 0 &&
			(slope & (1 << ((edge + 1) % 4))) == 0;
		fdata.voxel_pos.z += extra_z;
		vp->edge_cursor.SetCursor(fdata.voxel_pos, fdata.cursor, sprite, steep_lower_edge ? vp->tile_height : 0);
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
		Voxel *v = vs->GetCreate(c->cursor_pos.z, false);
		assert(v != nullptr);
		TileSlope slope = ExpandTileSlope(v->GetGroundSlope());
		int32 extra_z = 0;
		if ((slope & TSB_TOP) == 0 && IsRaisedEdge(edge, slope)) {
			extra_z = 1;
			v = vs->GetCreate(c->cursor_pos.z + extra_z, true);
			assert(v != nullptr);
		}
		v->SetFenceType(edge, this->selected_fence_type);
		vp->MarkVoxelDirty(c->cursor_pos.x, c->cursor_pos.y, c->cursor_pos.z + extra_z);
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


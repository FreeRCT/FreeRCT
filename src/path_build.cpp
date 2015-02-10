/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file path_build.cpp %Path building manager code. */

#include "stdafx.h"
#include "path_build.h"
#include "map.h"
#include "gamemode.h"
#include "window.h"
#include "math_func.h"

PathBuildManager _path_builder; ///< %Path build manager.

/**
 * Build a path at a tile, and claim the voxels above it as well.
 * @param voxel_pos Coordinate of the voxel.
 * @param path_type The type of path to build.
 * @param path_spr Imploded sprite number.
 * @see RemovePathAtTile
 */
static void BuildPathAtTile(const XYZPoint16 &voxel_pos, PathType path_type, uint8 path_spr)
{
	VoxelStack *avs = _additions.GetModifyStack(voxel_pos.x, voxel_pos.y);

	Voxel *av = avs->GetCreate(voxel_pos.z, true);
	av->SetInstance(SRI_PATH);
	uint8 slope = AddRemovePathEdges(voxel_pos, path_spr, EDGE_ALL, true, _sprite_manager.GetPathStatus(path_type));
	av->SetInstanceData(MakePathInstanceData(slope, path_type));

	av = avs->GetCreate(voxel_pos.z + 1, true);
	av->ClearVoxel();
	av->SetInstance(SRI_PATH);
	av->SetInstanceData(PATH_INVALID);

	if (path_spr >= PATH_FLAT_COUNT) { // For non-flat sprites, add another voxel.
		av = avs->GetCreate(voxel_pos.z + 2, true);
		av->ClearVoxel();
		av->SetInstance(SRI_PATH);
		av->SetInstanceData(PATH_INVALID);
	}

	MarkVoxelDirty(voxel_pos);
}

/**
 * Remove a path from a tile, and free the voxels above it as well.
 * @param voxel_pos Coordinate of the voxel.
 * @param path_spr Imploded sprite number.
 * @see BuildPathAtTile
 */
static void RemovePathAtTile(const XYZPoint16 &voxel_pos, uint8 path_spr)
{
	VoxelStack *avs = _additions.GetModifyStack(voxel_pos.x, voxel_pos.y);

	Voxel *av = avs->GetCreate(voxel_pos.z, false);
	av->SetInstance(SRI_FREE);
	av->SetInstanceData(0);
	AddRemovePathEdges(voxel_pos, path_spr, EDGE_ALL, true, PAS_UNUSED);
	MarkVoxelDirty(voxel_pos);

	av = avs->GetCreate(voxel_pos.z + 1, false);
	av->SetInstance(SRI_FREE);
	av->SetInstanceData(0);

	if (path_spr >= PATH_FLAT_COUNT) {
		av = avs->GetCreate(voxel_pos.z + 2, false);
		av->SetInstance(SRI_FREE);
		av->SetInstanceData(0);
	}
}

/**
 * Does a path run at/to the bottom the given voxel in the neighbouring voxel?
 * @param voxel_pos Coordinate of the voxel.
 * @param edge Direction to move to get the neighbouring voxel.
 * @return Whether a path exists at the bottom of the neighbouring voxel.
 * @pre voxel coordinate must be valid in the world.
 * @todo Merge with path computations in the path placement.
 */
bool PathExistsAtBottomEdge(XYZPoint16 voxel_pos, TileEdge edge)
{
	voxel_pos.x += _tile_dxy[edge].x;
	voxel_pos.y += _tile_dxy[edge].y;
	if (!IsVoxelstackInsideWorld(voxel_pos.x, voxel_pos.y)) return false;

	const Voxel *vx = _world.GetVoxel(voxel_pos);
	if (vx == nullptr || !HasValidPath(vx)) {
		/* No path here, check the voxel below. */
		if (voxel_pos.z == 0) return false;
		voxel_pos.z--;
		vx = _world.GetVoxel(voxel_pos);
		if (vx == nullptr || !HasValidPath(vx)) return false;
		/* Path must end at the top of the voxel. */
		return GetImplodedPathSlope(vx) == _path_up_from_edge[edge];
	}
	/* Path must end at the bottom of the voxel. */
	return GetImplodedPathSlope(vx) < PATH_FLAT_COUNT || GetImplodedPathSlope(vx) == _path_down_from_edge[edge];
}

/**
 * In the given voxel, can an upward path be build in the voxel from the bottom at the given edge?
 * @param voxel_pos Coordinate of the voxel.
 * @param edge Entry edge.
 * @param path_type For building (ie not \a test_only), the type of path to build.
 * @param test_only Only test whether it could be created.
 * @return Whether the path is or could be built.
 */
static bool BuildUpwardPath(const XYZPoint16 &voxel_pos, TileEdge edge, PathType path_type, bool test_only)
{
	/* xy position should be valid, and allow path building. */
	if (!IsVoxelstackInsideWorld(voxel_pos.x, voxel_pos.y)) return false;
	if (_game_mode_mgr.InPlayMode() && _world.GetTileOwner(voxel_pos.x, voxel_pos.y) != OWN_PARK) return false;

	if (voxel_pos.z < 0 || voxel_pos.z > WORLD_Z_SIZE - 3) return false; // Z range should be valid.

	const VoxelStack *vs = _world.GetStack(voxel_pos.x, voxel_pos.y);
	const Voxel *v = vs->Get(voxel_pos.z + 1); // Voxel above the path should be empty.
	if (v != nullptr && !v->IsEmpty()) return false;

	v = vs->Get(voxel_pos.z + 2); // 2 voxels higher should also be empty.
	if (v != nullptr && !v->IsEmpty()) return false;

	v = vs->Get(voxel_pos.z);
	if (v != nullptr) {
		if (v->GetInstance() != SRI_FREE) return false; // Voxel should have no other rides.
		if (v->GetGroundType() != GTP_INVALID) {
			TileSlope slope = ExpandTileSlope(v->GetGroundSlope());
			if ((slope & (TSB_STEEP | TSB_TOP)) == TSB_STEEP) return false;
			if ((slope & _corners_at_edge[edge]) != 0) return false; // A raised corner at 'edge'.
		} else {
			/* No surface, but a foundation suggests a nearby hill.
			 * Currently simply deny building here, in the future, consider making a tunnel.
			 */
			if (v->GetFoundationType() != FDT_INVALID) return false;
		}
	}

	if (!test_only) BuildPathAtTile(voxel_pos, path_type, _path_up_from_edge[edge]);
	return true;
}

/**
 * In the given voxel, can a flat path be build in the voxel?
 * @param voxel_pos Coordinate of the voxel.
 * @param path_type For building (ie not \a test_only), the type of path to build.
 * @param test_only Only test whether it could be created.
 * @return Whether the path is or could be built.
 */
static bool BuildFlatPath(const XYZPoint16 &voxel_pos, PathType path_type, bool test_only)
{
	/* xy position should be valid, and allow path building. */
	if (!IsVoxelstackInsideWorld(voxel_pos.x, voxel_pos.y)) return false;
	if (_game_mode_mgr.InPlayMode() && _world.GetTileOwner(voxel_pos.x, voxel_pos.y) != OWN_PARK) return false;

	if (voxel_pos.z < 0 || voxel_pos.z > WORLD_Z_SIZE - 2) return false; // Z range should be valid.

	const VoxelStack *vs = _world.GetStack(voxel_pos.x, voxel_pos.y);
	const Voxel *v = vs->Get(voxel_pos.z + 1); // Voxel above the path should be empty.
	if (v != nullptr && !v->IsEmpty()) return false;
	v = vs->Get(voxel_pos.z);
	if (v != nullptr) {
		if (v->GetInstance() != SRI_FREE) return false; // Voxel should have no other rides.
		if (v->GetGroundType() != GTP_INVALID) {
			if (v->GetGroundSlope() >= PATH_FLAT_COUNT) return false; // Non-flat surface.
		} else {
			/* No surface, but a foundation suggests a nearby hill.
			 * Currently simply deny building here, in the future, consider making a tunnel.
			 */
			if (v->GetFoundationType() != FDT_INVALID) return false;
		}
	}

	if (!test_only) BuildPathAtTile(voxel_pos, path_type, PATH_EMPTY);
	return true;
}

/**
 * In the given voxel, can an downward path be build in the voxel from the bottom at the given edge?
 * @param voxel_pos Coordinate of the voxel.
 * @param edge Entry edge.
 * @param path_type For building (ie not \a test_only), the type of path to build.
 * @param test_only Only test whether it could be created.
 * @return Whether the path is or could be built.
 */
static bool BuildDownwardPath(XYZPoint16 voxel_pos, TileEdge edge, PathType path_type, bool test_only)
{
	/* xy position should be valid, and allow path building. */
	if (!IsVoxelstackInsideWorld(voxel_pos.x, voxel_pos.y)) return false;
	if (_game_mode_mgr.InPlayMode() && _world.GetTileOwner(voxel_pos.x, voxel_pos.y) != OWN_PARK) return false;

	if (voxel_pos.z <= 0 || voxel_pos.z > WORLD_Z_SIZE - 3) return false; // Z range should be valid.

	const VoxelStack *vs = _world.GetStack(voxel_pos.x, voxel_pos.y);
	const Voxel *v = vs->Get(voxel_pos.z); // Voxel above the path should be empty.
	if (v != nullptr && !v->IsEmpty()) return false;

	v = vs->Get(voxel_pos.z + 1); // 1 voxel higher should also be empty.
	if (v != nullptr && !v->IsEmpty()) return false;

	v = vs->Get(voxel_pos.z - 1);
	if (v != nullptr) {
		if (v->GetInstance() != SRI_FREE) return false; // Voxel should have no other rides.
		if (v->GetGroundType() != GTP_INVALID) {
			TileSlope slope = ExpandTileSlope(v->GetGroundSlope());
			if ((slope & (TSB_STEEP | TSB_TOP)) == TSB_STEEP) return false;
			if ((slope & _corners_at_edge[(edge + 2) & 3]) != 0) return false; // A raised corner at the opposite 'edge'.
		} else {
			/* No surface, but a foundation suggests a nearby hill.
			 * Currently simply deny building here, in the future, consider making a tunnel.
			 */
			if (v->GetFoundationType() != FDT_INVALID) return false;
		}
	}

	if (!test_only) {
		voxel_pos.z--;
		BuildPathAtTile(voxel_pos, path_type, _path_down_from_edge[edge]);
	}
	return true;
}

/**
 * (Try to) remove a path from the given voxel.
 * @param voxel_pos Coordinate of the voxel.
 * @param test_only Only test whether it could be removed.
 * @return Whether the path is or could be removed.
 */
static bool RemovePath(const XYZPoint16 &voxel_pos, bool test_only)
{
	/* xy position should be valid, and allow path building. */
	if (!IsVoxelstackInsideWorld(voxel_pos.x, voxel_pos.y)) return false;
	if (_game_mode_mgr.InPlayMode() && _world.GetTileOwner(voxel_pos.x, voxel_pos.y) != OWN_PARK) return false;

	if (voxel_pos.z <= 0 || voxel_pos.z > WORLD_Z_SIZE - 2) return false; // Z range should be valid.

	const VoxelStack *vs = _world.GetStack(voxel_pos.x, voxel_pos.y);
	const Voxel *v = vs->Get(voxel_pos.z);
	if (v == nullptr || !HasValidPath(v)) return false;
	PathSprites ps = GetImplodedPathSlope(v);
	assert(ps < PATH_COUNT);

	v = vs->Get(voxel_pos.z + 1);
	assert(v->GetInstance() == SRI_PATH && !HasValidPath(v->GetInstanceData()));
	if (ps >= PATH_FLAT_COUNT) {
		v = vs->Get(voxel_pos.z + 2);
		assert(v->GetInstance() == SRI_PATH && !HasValidPath(v->GetInstanceData()));
	}

	if (!test_only) RemovePathAtTile(voxel_pos, ps);
	return true;
}

/**
 * In the given voxel, can a path be build in the voxel from the bottom at the given edge?
 * @param voxel_pos Coordinate of the voxel.
 * @param edge Entry edge.
 * @return Bit-set of track slopes, indicating the directions of building paths.
 */
static uint8 CanBuildPathFromEdge(const XYZPoint16 &voxel_pos, TileEdge edge)
{
	if (!IsVoxelstackInsideWorld(voxel_pos.x, voxel_pos.y)) return 0;
	if (voxel_pos.z < 0 || voxel_pos.z >= WORLD_Z_SIZE - 1) return 0;

	/* If other side of the edge is not on-world or not owned, don't compute path options. */
	Point16 dxy = _tile_dxy[edge];
	if (!IsVoxelstackInsideWorld(voxel_pos.x + dxy.x, voxel_pos.y + dxy.y) ||
			(_game_mode_mgr.InPlayMode() && _world.GetTileOwner(voxel_pos.x + dxy.x, voxel_pos.y + dxy.y) != OWN_PARK)) return 0;

	const Voxel *v = _world.GetVoxel(voxel_pos);
	if (v != nullptr && HasValidPath(v)) {
		PathSprites ps = GetImplodedPathSlope(v);
		if (ps < PATH_FLAT_COUNT) return 1 << TSL_FLAT;
		if (ps == _path_up_from_edge[edge]) return 1 << TSL_UP;
	}
	if (voxel_pos.z > 0) {
		v = _world.GetVoxel(voxel_pos + XYZPoint16(0, 0, -1));
		if (v != nullptr &&  HasValidPath(v) && GetImplodedPathSlope(v) == _path_down_from_edge[edge]) return 1 << TSL_DOWN;
	}

	uint8 slopes = 0;
	// XXX Check for already existing paths.
	if (BuildDownwardPath(voxel_pos, edge, PAT_INVALID, true)) slopes |= 1 << TSL_DOWN;
	if (BuildFlatPath(voxel_pos, PAT_INVALID, true)) slopes |= 1 << TSL_FLAT;
	if (BuildUpwardPath(voxel_pos, edge, PAT_INVALID, true)) slopes |= 1 << TSL_UP;
	return slopes;
}

/**
 * Compute the attach points of a path in a voxel.
 * @param voxel_pos Coordinate of the voxel.
 * @return Attach points for paths starting from the given voxel coordinates.
 *         Upper 4 bits are the edges at the top of the voxel, lower 4 bits are the attach points for the bottom of the voxel.
 */
static uint8 GetPathAttachPoints(const XYZPoint16 &voxel_pos)
{
	if (!IsVoxelstackInsideWorld(voxel_pos.x, voxel_pos.y)) return 0;
	if (voxel_pos.z >= WORLD_Z_SIZE - 1) return 0; // The voxel containing the flat path, and one above it.

	const Voxel *v = _world.GetVoxel(voxel_pos);
	if (v == nullptr) return 0;

	uint8 edges = 0;
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		uint16 x = voxel_pos.x + _tile_dxy[edge].x;
		uint16 y = voxel_pos.y + _tile_dxy[edge].y;
		if (!IsVoxelstackInsideWorld(x, y)) continue;

		const XYZPoint16 new_pos(x, y, voxel_pos.z);
		if (HasValidPath(v)) {
			PathSprites ps = GetImplodedPathSlope(v);
			if (ps < PATH_FLAT_COUNT) {
				if (CanBuildPathFromEdge(new_pos, (TileEdge)((edge + 2) % 4)) != 0) edges |= 1 << edge;
			} else {
				const XYZPoint16 new_above(x, y, voxel_pos.z + 1);
				if (_path_up_from_edge[edge] == ps
						&& CanBuildPathFromEdge(new_pos, (TileEdge)((edge + 2) % 4)) != 0) edges |= 1 << edge;
				if (_path_down_from_edge[edge] == ps
						&& CanBuildPathFromEdge(new_above, (TileEdge)((edge + 2) % 4)) != 0) edges |= (1 << edge) << 4;
			}
			continue;
		}
		if (v->GetGroundType() != GTP_INVALID) {
			TileSlope ts = ExpandTileSlope(v->GetGroundSlope());
			if ((ts & TSB_STEEP) != 0) continue;
			if ((ts & _corners_at_edge[edge]) == 0) {
				if (CanBuildPathFromEdge(new_pos, (TileEdge)((edge + 2) % 4)) != 0) edges |= 1 << edge;
			} else {
				const XYZPoint16 new_above(x, y, voxel_pos.z + 1);
				if (CanBuildPathFromEdge(new_above, (TileEdge)((edge + 2) % 4)) != 0) edges |= (1 << edge) << 4;
			}
			continue;
		}
		/* No path and no ground -> Invalid, do next edge. */
	}
	return edges;
}

/** Default constructor. */
PathBuildManager::PathBuildManager() : MouseMode(WC_PATH_BUILDER, MM_PATH_BUILDING)
{
	this->path_type = PAT_INVALID; // Initialized by the path gui on first opening.
	this->state = PBS_IDLE;
	this->selected_arrow = INVALID_EDGE;
	this->selected_slope = TSL_INVALID;
	DisableWorldAdditions();
}

bool PathBuildManager::MayActivateMode()
{
	return true;
}

/**
 * Restart the path build interaction sequence.
 * @param pos New mouse position.
 * @return The mouse mode should be enabled.
 */
void PathBuildManager::ActivateMode(const Point16 &pos)
{
	this->selected_arrow = INVALID_EDGE;
	this->selected_slope = TSL_INVALID;
	if (this->state != PBS_IDLE) this->state = PBS_WAIT_VOXEL;
	this->UpdateState();

	this->mouse_state = 0;
}

/** Notification that the mouse mode has been disabled. */
void PathBuildManager::LeaveMode()
{
	DisableWorldAdditions();
}

bool PathBuildManager::EnableCursors()
{
	return true;
}

void PathBuildManager::OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos)
{
	if (this->state == PBS_LONG_BUILD) {
		this->ComputeNewLongPath(vp->ComputeHorizontalTranslation(vp->rect.width / 2 - pos.x, vp->rect.height / 2 - pos.y));
	} else if ((this->mouse_state & MB_RIGHT) != 0) {
		/* Drag the window if button is pressed down. */
		vp->MoveViewport(pos.x - old_pos.x, pos.y - old_pos.y);
	} else {
		/* Only update tile cursor if no tile selected yet. */
		if (this->state == PBS_WAIT_VOXEL) {
			FinderData fdata((CS_GROUND | CS_PATH), FW_TILE);
			if (vp->ComputeCursorPosition(&fdata) != CS_NONE) {
				vp->tile_cursor.SetCursor(fdata.voxel_pos, fdata.cursor);
			}
		}
	}
}

void PathBuildManager::OnMouseButtonEvent(Viewport *vp, uint8 state)
{
	this->mouse_state = state & MB_CURRENT;

	if ((this->mouse_state & MB_LEFT) != 0) { // Left-click -> select current tile.
		if (this->state == PBS_LONG_BUILD || this->state == PBS_LONG_BUY) {
			this->ConfirmLongPath();
		} else {
			FinderData fdata((CS_GROUND | CS_PATH), FW_TILE);
			if (vp->ComputeCursorPosition(&fdata) != CS_NONE) this->TileClicked(fdata.voxel_pos);
		}
	}
}

/**
 * Set the state of the path build GUI.
 * @param opened If \c true the path GUI just opened, else it closed.
 */
void PathBuildManager::SetPathGuiState(bool opened)
{
	this->state = opened ? PBS_WAIT_VOXEL : PBS_IDLE;
	this->UpdateState();
	Viewport *vp = GetViewport();
	if (opened || (vp != nullptr && _mouse_modes.GetMouseMode() == MM_PATH_BUILDING)) _mouse_modes.SetViewportMousemode();
}

/**
 * User clicked somewhere. Check whether it is a good tile or path, and if so, move the building process forward.
 * @param click_pos Coordinate of the voxel.
 */
void PathBuildManager::TileClicked(const XYZPoint16 &click_pos)
{
	if (this->state == PBS_IDLE || this->state > PBS_WAIT_BUY) return;
	uint8 dirs = GetPathAttachPoints(click_pos);
	if (dirs == 0) return;

	this->pos = click_pos;
	this->allowed_arrows = dirs;
	this->state = PBS_WAIT_ARROW;
	this->UpdateState();
}

/**
 * User selected a build direction. Check it, and if so, move the building process forward.
 * @param direction Direction of build.
 */
void PathBuildManager::SelectArrow(TileEdge direction)
{
	if (this->state < PBS_WAIT_ARROW || this->state > PBS_WAIT_BUY || direction >= EDGE_COUNT) return;
	if ((this->allowed_arrows & (0x11 << direction)) == 0) return;
	this->selected_arrow = direction;
	this->state = PBS_WAIT_SLOPE;
	this->UpdateState();
}

/**
 * A new path type should be used for building paths.
 * @param pt New path type to use.
 */
void PathBuildManager::SelectPathType(PathType pt)
{
	if (this->path_type == pt) return;

	this->path_type = pt;
	if (this->state < PBS_WAIT_BUY) return; // Nothing else to update.
	this->UpdateState();
}

/**
 * Try to move the tile cursor to a new tile.
 * @param edge Direction of movement.
 * @param move_up Current tile seems to move up.
 */
void PathBuildManager::MoveCursor(TileEdge edge, bool move_up)
{
	if (this->state <= PBS_WAIT_ARROW || this->state > PBS_WAIT_BUY || edge == INVALID_EDGE) return;

	/* Test whether we can move in the indicated direction. */
	Point16 dxy = _tile_dxy[edge];
	if ((dxy.x < 0 && this->pos.x == 0) || (dxy.x > 0 && this->pos.x == _world.GetXSize() - 1)) return;
	if ((dxy.y < 0 && this->pos.y == 0) || (dxy.y > 0 && this->pos.y == _world.GetYSize() - 1)) return;
	if (_game_mode_mgr.InPlayMode() && _world.GetTileOwner(this->pos.x + dxy.x, this->pos.y + dxy.y) != OWN_PARK) return;

	const Voxel *v_top, *v_bot;
	if (move_up) {
		/* Exit of current tile is at the top. */
		v_top = (this->pos.z > WORLD_Z_SIZE - 2) ? nullptr : _world.GetVoxel(this->pos + XYZPoint16(dxy.x, dxy.y, 1));
		v_bot = _world.GetVoxel(this->pos + XYZPoint16(dxy.x, dxy.y, 0));
	} else {
		/* Exit of current tile is at the bottom. */
		v_top = _world.GetVoxel(this->pos + XYZPoint16(dxy.x, dxy.y, 0));
		v_bot = (this->pos.z == 0) ? nullptr : _world.GetVoxel(this->pos + XYZPoint16(dxy.x, dxy.y, -1));
	}

	/* Try to find a voxel with a path. */
	if (v_top != nullptr && HasValidPath(v_top)) {
		this->pos.x += dxy.x;
		this->pos.y += dxy.y;
		if (move_up) this->pos.z++;
		this->state = PBS_WAIT_ARROW;
		this->UpdateState();
		return;
	}
	if (v_bot != nullptr && HasValidPath(v_bot)) {
		this->pos.x += dxy.x;
		this->pos.y += dxy.y;
		if (!move_up) this->pos.z--;
		this->state = PBS_WAIT_ARROW;
		this->UpdateState();
		return;
	}

	/* Try to find a voxel with surface. */
	if (v_top != nullptr && v_top->GetGroundType() != GTP_INVALID && !IsImplodedSteepSlope(v_top->GetGroundSlope())) {
		this->pos.x += dxy.x;
		this->pos.y += dxy.y;
		if (move_up) this->pos.z++;
		this->state = PBS_WAIT_ARROW;
		this->UpdateState();
		return;
	}
	if (v_bot != nullptr && v_bot->GetGroundType() != GTP_INVALID && !IsImplodedSteepSlope(v_bot->GetGroundSlope())) {
		this->pos.x += dxy.x;
		this->pos.y += dxy.y;
		if (!move_up) this->pos.z--;
		this->state = PBS_WAIT_ARROW;
		this->UpdateState();
		return;
	}
}

/**
 * User clicked 'forward' or 'back'. Figure out which direction to move in.
 * @param move_forward Move forward (if \c false, move back).
 */
void PathBuildManager::SelectMovement(bool move_forward)
{
	if (this->state <= PBS_WAIT_ARROW || this->state > PBS_WAIT_BUY) return;

	TileEdge edge = (move_forward) ? this->selected_arrow : (TileEdge)((this->selected_arrow + 2) % 4);

	bool move_up;
	const Voxel *v = _world.GetVoxel(this->pos);
	if (v == nullptr) return;
	if (HasValidPath(v)) {
		move_up = (GetImplodedPathSlope(v) == _path_down_from_edge[edge]);
	} else if (v->GetGroundType() != GTP_INVALID) {
		TileSlope ts = ExpandTileSlope(v->GetGroundSlope());
		if ((ts & TSB_STEEP) != 0) return;
		move_up = ((ts & _corners_at_edge[edge]) != 0);
	} else {
		return; // Surface type but no valid ground/path surface.
	}

	this->MoveCursor(edge, move_up);
}

/**
 * Compute the voxel to display the arrow cursor.
 * @return Computed position of the voxel that should contain the arrow cursor.
 */
XYZPoint16 PathBuildManager::ComputeArrowCursorPosition()
{
	assert(this->state > PBS_WAIT_ARROW && this->state <= PBS_WAIT_BUY);
	assert(this->selected_arrow != INVALID_EDGE);

	Point16 dxy = _tile_dxy[this->selected_arrow];
	XYZPoint16 arr_pos = this->pos + XYZPoint16(dxy.x, dxy.y, 0);

	uint8 bit = 1 << this->selected_arrow;
	if ((bit & this->allowed_arrows) == 0) { // Build direction is not at the bottom of the voxel.
		assert(((bit << 4) &  this->allowed_arrows) != 0); // Should be available at the top of the voxel.
		arr_pos.z++;
	}

	/* Do some paranoia checking. */
	assert(IsVoxelInsideWorld(arr_pos));
	return arr_pos;
}

/** Compute the new contents of the voxel where the path should be added from the #_world. */
void PathBuildManager::ComputeWorldAdditions()
{
	assert(this->state == PBS_WAIT_BUY); // It needs selected_arrow and selected_slope.
	assert(this->selected_slope != TSL_INVALID);

	if (((1 << this->selected_slope) & this->allowed_slopes) == 0) return;

	XYZPoint16 arrow_pos = this->ComputeArrowCursorPosition();
	TileEdge edge = (TileEdge)((this->selected_arrow + 2) % 4);
	switch (this->selected_slope) {
		case TSL_DOWN:
			BuildDownwardPath(arrow_pos, edge, this->path_type, false);
			break;

		case TSL_FLAT:
			BuildFlatPath(arrow_pos, this->path_type, false);
			break;

		case TSL_UP:
			BuildUpwardPath(arrow_pos, edge, this->path_type, false);
			break;

		default:
			NOT_REACHED();
	}
}

/** Update the state of the path build process. */
void PathBuildManager::UpdateState()
{
	Viewport *vp = GetViewport();

	if (this->state == PBS_IDLE) {
		this->selected_arrow = INVALID_EDGE;
		this->selected_slope = TSL_INVALID;
	}

	/* The tile cursor is controlled by the viewport if waiting for a voxel or earlier. */
	if (vp != nullptr && this->state > PBS_WAIT_VOXEL && this->state <= PBS_WAIT_BUY) {
		vp->tile_cursor.SetCursor(this->pos, CUR_TYPE_TILE);
	}

	/* See whether the PBS_WAIT_ARROW state can be left automatically. */
	if (this->state == PBS_WAIT_ARROW) {
		this->allowed_arrows = GetPathAttachPoints(this->pos);

		/* If a valid selection has been made, or if only one choice exists, take it. */
		if (this->selected_arrow != INVALID_EDGE && ((0x11 << this->selected_arrow) & this->allowed_arrows) != 0) {
			this->state = PBS_WAIT_SLOPE;
		} else if (this->allowed_arrows == (1 << EDGE_NE) || this->allowed_arrows == (0x10 << EDGE_NE)) {
			this->selected_arrow = EDGE_NE;
			this->state = PBS_WAIT_SLOPE;
		} else if (this->allowed_arrows == (1 << EDGE_NW) || this->allowed_arrows == (0x10 << EDGE_NW)) {
			this->selected_arrow = EDGE_NW;
			this->state = PBS_WAIT_SLOPE;
		} else if (this->allowed_arrows == (1 << EDGE_SE) || this->allowed_arrows == (0x10 << EDGE_SE)) {
			this->selected_arrow = EDGE_SE;
			this->state = PBS_WAIT_SLOPE;
		} else if (this->allowed_arrows == (1 << EDGE_SW) || this->allowed_arrows == (0x10 << EDGE_SW)) {
			this->selected_arrow = EDGE_SW;
			this->state = PBS_WAIT_SLOPE;
		}
	}

	/* Set the arrow cursor. Note that display is controlled later. */
	if (vp != nullptr) {
		if (this->state > PBS_WAIT_ARROW && this->state <= PBS_WAIT_BUY) {
			XYZPoint16 arrow_pos = this->ComputeArrowCursorPosition();
			vp->arrow_cursor.SetCursor(arrow_pos, (CursorType)(CUR_TYPE_ARROW_NE + this->selected_arrow));
		} else {
			vp->arrow_cursor.SetInvalid();
		}
	}

	/* See whether the PBS_WAIT_SLOPE state can be left automatically. */
	if (this->state == PBS_WAIT_SLOPE) {
		/* Compute allowed slopes. */
		XYZPoint16 arrow_pos = this->ComputeArrowCursorPosition();
		this->allowed_slopes = CanBuildPathFromEdge(arrow_pos, (TileEdge)((this->selected_arrow + 2) % 4));

		/* If a valid selection has been made, or if only one choice exists, take it. */
		if (this->selected_slope != TSL_INVALID && ((1 << this->selected_slope) & this->allowed_slopes) != 0) {
			this->state = PBS_WAIT_BUY;
		} else if (this->allowed_slopes == (1 << TSL_DOWN)) {
			this->selected_slope = TSL_DOWN;
			this->state = PBS_WAIT_BUY;
		} else if (this->allowed_slopes == (1 << TSL_FLAT)) {
			this->selected_slope = TSL_FLAT;
			this->state = PBS_WAIT_BUY;
		} else if (this->allowed_slopes == (1 << TSL_UP)) {
			this->selected_slope = TSL_UP;
			this->state = PBS_WAIT_BUY;
		}
	}

	/* Handle _additions display. */
	if (vp != nullptr) {
		if (this->state == PBS_WAIT_SLOPE) {
			_additions.Clear();
			vp->EnableWorldAdditions();
		} else if (this->state == PBS_WAIT_BUY) {
			_additions.Clear();

			this->ComputeWorldAdditions();
			vp->EnableWorldAdditions();
			vp->EnsureAdditionsAreVisible();
		} else {
			if (this->state != PBS_LONG_BUILD && this->state != PBS_LONG_BUY) vp->DisableWorldAdditions();
		}
	}

	NotifyChange(WC_PATH_BUILDER, ALL_WINDOWS_OF_TYPE, CHG_UPDATE_BUTTONS, 0);
}

/**
 * Can the user press the 'remove' button at the path GUI?
 * @return Button is enabled.
 */
bool PathBuildManager::GetRemoveIsEnabled() const
{
	if (this->state == PBS_IDLE || this->state == PBS_WAIT_VOXEL) return false;
	/* If current tile has a path, it can be removed. */
	const Voxel *v = _world.GetVoxel(this->pos);
	if (v != nullptr && HasValidPath(v)) return true;
	return this->state == PBS_WAIT_BUY;
}

/**
 * Select a slope from the allowed slopes.
 * @param slope Slope to select.
 */
void PathBuildManager::SelectSlope(TrackSlope slope)
{
	if (this->state < PBS_WAIT_SLOPE || slope >= TSL_COUNT_GENTLE) return;
	if ((this->allowed_slopes & (1 << slope)) != 0) {
		this->selected_slope = slope;
		this->state = PBS_WAIT_SLOPE;
		this->UpdateState();
	}
}

/** Enter long path building mode. */
void PathBuildManager::SelectLong()
{
	if (this->state == PBS_LONG_BUY || this->state == PBS_LONG_BUILD) {
		/* Disable long mode. */
		this->state = PBS_WAIT_ARROW;
		this->UpdateState();
	} else if (this->state < PBS_WAIT_ARROW) {
		return;
	} else {
		/* Enable long mode. */
		this->state = PBS_LONG_BUILD;
		this->long_pos.x = _world.GetXSize();
		this->long_pos.y = _world.GetYSize();
		this->long_pos.z = WORLD_Z_SIZE;
		this->UpdateState();
	}
}

/**
 * Is the \a change a good direction to get from \a src to \a dst?
 * @param change Direction of change (\c -1, \c 0, or \c +1).
 * @param src Starting point.
 * @param dst Destination.
 * @return The \a change direction is good.
 */
static bool GoodDirection(int16 change, uint16 src, uint16 dst) {
	if (change > 0  && src >= dst) return false;
	if (change == 0 && src != dst) return false;
	if (change < 0  && src <= dst) return false;
	return true;
}

/**
 * Build a path from #_path_builder xpos/ypos to the mouse cursor position.
 * @param mousexy Mouse position.
 */
void PathBuildManager::ComputeNewLongPath(const Point32 &mousexy)
{
	static const TrackSlope slope_prios_down[] = {TSL_DOWN, TSL_FLAT, TSL_UP,   TSL_INVALID}; // Order of preference when going down.
	static const TrackSlope slope_prios_flat[] = {TSL_FLAT, TSL_UP,   TSL_DOWN, TSL_INVALID}; // Order of preference when at the right height.
	static const TrackSlope slope_prios_up[]   = {TSL_UP,   TSL_FLAT, TSL_DOWN, TSL_INVALID}; // Order of preference when going up.

	Viewport *vp = GetViewport();
	if (vp == nullptr) return;

	int c1, c2, c3;
	switch (vp->orientation) {
		case VOR_NORTH: c1 =  1; c2 =  2; c3 =  2; break;
		case VOR_EAST:  c1 = -1; c2 = -2; c3 =  2; break;
		case VOR_SOUTH: c1 =  1; c2 = -2; c3 = -2; break;
		case VOR_WEST:  c1 = -1; c2 =  2; c3 = -2; break;
		default: NOT_REACHED();
	}

	XYZPoint16 path_pos(0, 0, 0);
	path_pos.y = this->pos.y * 256 + 128;
	int32 lambda_y = path_pos.y - mousexy.y; // Distance to constant Y plane at current tile cursor.
	path_pos.x = this->pos.x * 256 + 128;
	int32 lambda_x = path_pos.x - mousexy.x; // Distance to constant X plane at current tile cursor.

	if (abs(lambda_x) < abs(lambda_y)) {
		/* X constant. */
		path_pos.x /= 256;
		path_pos.y = Clamp<int32>(mousexy.y + c1 * lambda_x, 0, _world.GetYSize() * 256 - 1) / 256;
		path_pos.z = Clamp<int32>(vp->view_pos.z + c3 * lambda_x, 0, WORLD_Z_SIZE * 256 - 1) / 256;
	} else {
		/* Y constant. */
		path_pos.x = Clamp<int32>(mousexy.x + c1 * lambda_y, 0, _world.GetXSize() * 256 - 1) / 256;
		path_pos.y /= 256;
		path_pos.z = Clamp<int32>(vp->view_pos.z + c2 * lambda_y, 0, WORLD_Z_SIZE * 256 - 1) / 256;
	}

	if (this->long_pos != path_pos) {
		this->long_pos = path_pos;

		_additions.Clear();
		path_pos = this->pos;
		/* Find the right direction from the selected tile to the current cursor location. */
		TileEdge direction;
		Point16 dxy;
		for (direction = EDGE_BEGIN; direction < EDGE_COUNT; direction++) {
			dxy = _tile_dxy[direction];
			if (!GoodDirection(dxy.x, path_pos.x, this->long_pos.x) || !GoodDirection(dxy.y, path_pos.y, this->long_pos.y)) continue;
			break;
		}
		if (direction == EDGE_COUNT) return;

		/* 'Walk' to the cursor as long as possible. */
		while (path_pos.x != this->long_pos.x || path_pos.y != this->long_pos.y) {
			uint8 slopes = CanBuildPathFromEdge(path_pos, direction);
			const TrackSlope *slope_prio;
			/* Get order of slope preference. */
			if (path_pos.z > this->long_pos.z) {
				slope_prio = slope_prios_down;
			} else if (path_pos.z == this->long_pos.z) {
				slope_prio = slope_prios_flat;
			} else {
				slope_prio = slope_prios_up;
			}
			/* Find best slope, and take it. */
			while (*slope_prio != TSL_INVALID && (slopes & (1 << *slope_prio)) == 0) slope_prio++;
			if (*slope_prio == TSL_INVALID) break;

			path_pos.x += dxy.x;
			path_pos.y += dxy.y;
			if (*slope_prio == TSL_UP) {
				if (!BuildUpwardPath(path_pos, static_cast<TileEdge>((direction + 2) & 3), this->path_type, false)) break;
				path_pos.z++;
			} else if (*slope_prio == TSL_DOWN) {
				if (!BuildDownwardPath(path_pos, static_cast<TileEdge>((direction + 2) & 3), this->path_type, false)) break;
				path_pos.z--;
			} else {
				if (!BuildFlatPath(path_pos, this->path_type, false)) break;
			}

		}
		vp->EnableWorldAdditions();
		vp->EnsureAdditionsAreVisible();
	}
}

/** Confirm that the current long path is the selection to build. */
void PathBuildManager::ConfirmLongPath()
{
	if (this->state == PBS_LONG_BUY || this->state != PBS_LONG_BUILD) return;
	this->state = PBS_LONG_BUY; // Switch to 'buy' mode.
	this->UpdateState();
}

/**
 * User selected 'buy' or 'remove'. Perform the action, and update the path build state.
 * @param buying If \c true, user selected 'buy'.
 */
void PathBuildManager::SelectBuyRemove(bool buying)
{
	if (buying) {
		// Buy a long path.
		if (this->state == PBS_LONG_BUY) {
			_additions.Commit();
			this->pos = this->long_pos;
			this->state = PBS_WAIT_ARROW;
			this->UpdateState();
			return;
		}
		// Buying a path tile.
		if (this->state != PBS_WAIT_BUY) return;
		_additions.Commit();
		this->SelectMovement(true);
	} else {
		// Removing a path tile.
		if (this->state <= PBS_WAIT_VOXEL || this->state > PBS_WAIT_BUY) return;
		TileEdge edge = (TileEdge)((this->selected_arrow + 2) % 4);
		const Voxel *v = _world.GetVoxel(this->pos);
		if (v == nullptr || !HasValidPath(v)) {
			this->MoveCursor(edge, false);
			this->UpdateState();
			return;
		}
		PathSprites ps = GetImplodedPathSlope(v);

		_additions.Clear();
		if (RemovePath(this->pos, false)) {
			_additions.Commit();
		}

		/* Short-cut version of this->SelectMovement(false), as that function fails after removing the path. */
		bool move_up = (ps == _path_down_from_edge[edge]);
		this->MoveCursor(edge, move_up);
		this->UpdateState();
	}
}

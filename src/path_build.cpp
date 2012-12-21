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
#include "window.h"
#include "math_func.h"

PathBuildManager _path_builder; ///< %Path build manager.

/**
 * In the given voxel, can a path be build in the voxel from the bottom at the given edge?
 * @param xpos X coordinate of the voxel.
 * @param ypos Y coordinate of the voxel.
 * @param zpos Z coordinate of the voxel.
 * @param edge Entry edge.
 * @return Bit-set of track slopes, indicating the directions of building paths.
 */
static uint8 CanBuildPathFromEdge(int16 xpos, int16 ypos, int8 zpos, TileEdge edge)
{
	if (zpos < 0 || zpos >= WORLD_Z_SIZE - 1) return 0;

	const VoxelStack *vs = _world.GetStack(xpos, ypos);

	const Voxel *above = (zpos < WORLD_Z_SIZE) ? vs->Get(zpos + 1) : NULL;
	if (above != NULL && above->GetType() != VT_EMPTY) return 0; // Not empty just above us -> path will not work here.

	/* A path just below us won't work either. */
	const Voxel *below = (zpos > 0) ? vs->Get(zpos - 1) : NULL;
	if (below != NULL && below->GetType() == VT_SURFACE && HasValidPath(below)) return 0;

	const Voxel *level = vs->Get(zpos);
	if (level != NULL) {
		switch (level->GetType()) {
			case VT_SURFACE: {
				if (level->GetFoundationType() != FDT_INVALID) return 0;
				if (level->GetPathRideNumber() != PT_INVALID) {
					if (HasValidPath(level)) {
						if (level->GetPathRideFlags() < PATH_FLAT_COUNT) return 1 << TSL_FLAT; // Already a flat path there.
						if (_path_down_from_edge[edge] == level->GetPathRideFlags()) return 1 << TSL_UP; // Already a sloped path up.
					}
					return 0; // A path but cannot connect to it, or a ride.
				}
				if (level->GetGroundType() != GTP_INVALID) {
					TileSlope ts = ExpandTileSlope(level->GetGroundSlope());
					if ((ts & TCB_STEEP) != 0) return 0; // Steep slope in the way.
					if ((ts & _corners_at_edge[edge]) != 0) return 0; // Some hilly stuff at the entry.
				}
				break;
			}

			case VT_EMPTY:
				break;

			case VT_REFERENCE:
				// XXX return 0 is a too brute-force.
				return 0;

			default:
				NOT_REACHED();
		}
	}

	/* No trivial cases or show stoppers, do a more precise check for each slope.
	 * Above: empty
	 * Below: Does not contain a surface + path.
	 * Level: Is not a coaster or a reference, does not contain a path or foundations, has no steep ground nor raised corners at the entrance edge.
	 */
	uint8 result = 0;

	/* Try building a upward slope.
	 * Works if not at the top, and the voxel at z+2 is also empty.
	 */
	if (zpos < WORLD_Z_SIZE - 2) {
		const Voxel *v = vs->Get(zpos + 2);
		if (v == NULL || v->GetType() == VT_EMPTY) result |= 1 << TSL_UP;
	}

	/* Try building a level slope. */
	{
		if (level == NULL) {
			result |= 1 << TSL_FLAT;
		} else {
			VoxelType vt = level->GetType();
			if (vt == VT_EMPTY) {
				result |= 1 << TSL_FLAT;
			} else if (vt == VT_SURFACE &&
					level->GetPathRideNumber() == PT_INVALID &&
					level->GetFoundationType() == FDT_INVALID &&
					level->GetGroundType() != GTP_INVALID && level->GetGroundSlope() == 0) {
				result |= 1 << TSL_FLAT;
			}
		}
	}

	/* Try building a downward slope. */
	if ((level == NULL || level->GetType() == VT_EMPTY) && zpos > 0) {
		VoxelType vt = (below != NULL) ? below->GetType() : VT_EMPTY;
		if (vt == VT_EMPTY) {
			result |= 1 << TSL_DOWN;
		} else if (vt == VT_SURFACE) {
			if (below->GetFoundationType() == FDT_INVALID && below->GetPathRideNumber() == PT_INVALID) {
				/* No foundations and paths. */
				if (below->GetGroundType() == GTP_INVALID) {
					result |= 1 << TSL_DOWN;
				} else {
					TileSlope ts = ExpandTileSlope(below->GetGroundSlope());
					if (((TCB_STEEP | _corners_at_edge[(edge + 2) % 4]) & ts) == 0) result |= 1 << TSL_DOWN;
				}
			}
		}
	}

	return result;
}

/**
 * Compute the attach points of a path in a voxel.
 * @param xpos X coordinate of the voxel.
 * @param ypos Y coordinate of the voxel.
 * @param zpos Z coordinate of the voxel.
 * @return Attach points for paths starting from the given voxel coordinates.
 *         Upper 4 bits are the edges at the top of the voxel, lower 4 bits are the attach points for the bottom of the voxel.
 */
static uint8 GetPathAttachPoints(int16 xpos, int16 ypos, int8 zpos)
{
	if (xpos >= _world.GetXSize()) return 0;
	if (ypos >= _world.GetYSize()) return 0;
	if (zpos >= WORLD_Z_SIZE - 1) return 0; // the voxel containing the flat path, and one above it.

	const Voxel *v = _world.GetVoxel(xpos, ypos, zpos);
	if (v == NULL || v->GetType() != VT_SURFACE) return 0; // XXX Maybe also handle referenced surface voxels?

	uint8 edges = 0;
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		uint16 x = xpos + _tile_dxy[edge].x;
		uint16 y = ypos + _tile_dxy[edge].y;
		if (x >= _world.GetXSize() || y >= _world.GetYSize()) continue;

		if (HasValidPath(v)) {
			if (v->GetPathRideFlags() < PATH_FLAT_COUNT) {
				if (CanBuildPathFromEdge(x, y, zpos, (TileEdge)((edge + 2) % 4)) != 0) edges |= 1 << edge;
			} else {
				if (_path_up_from_edge[edge] == v->GetPathRideFlags()
						&& CanBuildPathFromEdge(x, y, zpos, (TileEdge)((edge + 2) % 4)) != 0) edges |= 1 << edge;
				if (_path_down_from_edge[edge] == v->GetPathRideFlags()
						&& CanBuildPathFromEdge(x, y, zpos + 1, (TileEdge)((edge + 2) % 4)) != 0) edges |= (1 << edge) << 4;
			}
			continue;
		}
		if (v->GetGroundType() != GTP_INVALID) {
			TileSlope ts = ExpandTileSlope(v->GetGroundSlope());
			if ((ts & TCB_STEEP) != 0) continue;
			if ((ts & _corners_at_edge[edge]) == 0) {
				if (CanBuildPathFromEdge(x, y, zpos, (TileEdge)((edge + 2) % 4)) != 0) edges |= 1 << edge;
			} else {
				if (CanBuildPathFromEdge(x, y, zpos + 1, (TileEdge)((edge + 2) % 4)) != 0) edges |= (1 << edge) << 4;
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
	this->state = PBS_IDLE;
	this->selected_arrow = INVALID_EDGE;
	this->selected_slope = TSL_INVALID;
	DisableWorldAdditions();
}

/* virtual */ bool PathBuildManager::MayActivateMode()
{
	return true;
}

/**
 * Restart the path build interaction sequence.
 * @param pos New mouse position.
 * @return The mouse mode should be enabled.
 */
/* virtual */ void PathBuildManager::ActivateMode(const Point16 &pos)
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
			FinderData fdata((SO_GROUND | SO_PATH), false);
			if (vp->ComputeCursorPosition(&fdata) != SO_NONE) {
				vp->tile_cursor.SetCursor(fdata.xvoxel, fdata.yvoxel, fdata.zvoxel, fdata.cursor);
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
			FinderData fdata((SO_GROUND | SO_PATH), false);
			if (vp->ComputeCursorPosition(&fdata) != SO_NONE) {
				this->TileClicked(fdata.xvoxel, fdata.yvoxel, fdata.zvoxel);
			}
		}
	}
}

/**
 * Set the state of the path build GUI.
 * @param opened If \c true the path gui just opened, else it closed.
 */
void PathBuildManager::SetPathGuiState(bool opened)
{
	this->state = opened ? PBS_WAIT_VOXEL : PBS_IDLE;
	this->UpdateState();
	Viewport *vp = GetViewport();
	if (opened || (vp != NULL && _mouse_modes.GetMouseMode() == MM_PATH_BUILDING)) _mouse_modes.SetViewportMousemode();
}

/**
 * User clicked somewhere. Check whether it is a good tile or path, and if so, move the building process forward.
 * @param xpos X coordinate of the voxel.
 * @param ypos Y coordinate of the voxel.
 * @param zpos Z coordinate of the voxel.
 */
void PathBuildManager::TileClicked(uint16 xpos, uint16 ypos, uint8 zpos)
{
	if (this->state == PBS_IDLE || this->state > PBS_WAIT_BUY) return;
	uint8 dirs = GetPathAttachPoints(xpos, ypos, zpos);
	if (dirs == 0) return;

	this->xpos = xpos;
	this->ypos = ypos;
	this->zpos = zpos;
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
	if (this->state < PBS_WAIT_ARROW || this->state > PBS_WAIT_BUY || direction >= INVALID_EDGE) return;
	if ((this->allowed_arrows & (0x11 << direction)) == 0) return;
	this->selected_arrow = direction;
	this->state = PBS_WAIT_SLOPE;
	this->UpdateState();
}

/**
 * See whether moving in the indicated direction of the tile position is possible/makes sense.
 * @param direction Direction of movement.
 * @param delta_z Proposed change of Z height.
 * @param need_path %Voxel should contain a path.
 * @return Tile cursor position was moved.
 */
bool PathBuildManager::TryMove(TileEdge direction, int delta_z, bool need_path)
{
	Point16 dxy = _tile_dxy[direction];
	if ((dxy.x < 0 && this->xpos == 0) || (dxy.x > 0 && this->xpos == _world.GetXSize() - 1)) return false;
	if ((dxy.y < 0 && this->ypos == 0) || (dxy.y > 0 && this->ypos == _world.GetYSize() - 1)) return false;
	if ((delta_z < 0 && this->zpos == 0) || (delta_z > 0 && this->zpos == WORLD_Z_SIZE - 1)) return false;
	const Voxel *v = _world.GetVoxel(this->xpos + dxy.x, this->ypos + dxy.y, this->zpos + delta_z);
	/* Fail if the new voxel is a reference voxel, or it contains a ride. */
	if (v != NULL && (v->GetType() == VT_REFERENCE || (v->GetType() == VT_SURFACE && v->GetPathRideNumber() < PT_START))) return false;

	if (need_path) {
		if (v == NULL || v->GetType() != VT_SURFACE) return false;
		if (v->GetPathRideNumber() == PT_INVALID) return false;
	}

	this->xpos += dxy.x;
	this->ypos += dxy.y;
	this->zpos += delta_z;
	this->state = PBS_WAIT_ARROW;
	this->UpdateState();
	return true;
}

/**
 * Try to move the tile cursor to a new tile.
 * @param edge Direction of movement.
 * @param move_up Current tile seems to move up.
 */
void PathBuildManager::MoveCursor(TileEdge edge, bool move_up)
{
	if (this->state <= PBS_WAIT_ARROW || this->state > PBS_WAIT_BUY || edge == INVALID_EDGE) return;

	/* First try to find a voxel with a path at it. */
	if (this->TryMove(edge, 0, true)) return;
	if ( move_up && this->TryMove(edge,  1, true)) return;
	if (!move_up && this->TryMove(edge, -1, true)) return;

	/* Otherwise just settle for a surface. */
	if (this->TryMove(edge, 0, false)) return;
	if ( move_up && this->TryMove(edge,  1, false)) return;
	if (!move_up && this->TryMove(edge, -1, false)) return;
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
	const Voxel *v = _world.GetVoxel(this->xpos, this->ypos, this->zpos);
	if (v == NULL || v->GetType() != VT_SURFACE) return;
	if (HasValidPath(v)) {
		move_up = (v->GetPathRideFlags() == _path_down_from_edge[edge]);
	} else if (v->GetGroundType() != GTP_INVALID) {
		TileSlope ts = ExpandTileSlope(v->GetGroundSlope());
		if ((ts & TCB_STEEP) != 0) return;
		move_up = ((ts & _corners_at_edge[edge]) != 0);
	} else {
		return; // Surface type but no valid ground/path surface.
	}

	this->MoveCursor(edge, move_up);
}

/**
 * Compute the voxel to display the arrow cursor.
 * @param xpos [out] Computed X position of the voxel that should contain the arrow cursor.
 * @param ypos [out] Computed Y position of the voxel that should contain the arrow cursor.
 * @param zpos [out] Computed Z position of the voxel that should contain the arrow cursor.
 */
void PathBuildManager::ComputeArrowCursorPosition(uint16 *xpos, uint16 *ypos, uint8 *zpos)
{
	assert(this->state > PBS_WAIT_ARROW && this->state <= PBS_WAIT_BUY);
	assert(this->selected_arrow != INVALID_EDGE);

	Point16 dxy = _tile_dxy[this->selected_arrow];
	*xpos = this->xpos + dxy.x;
	*ypos = this->ypos + dxy.y;

	uint8 bit = 1 << this->selected_arrow;
	*zpos = this->zpos;
	if ((bit & this->allowed_arrows) == 0) { // Build direction is not at the bottom of the voxel.
		assert(((bit << 4) &  this->allowed_arrows) != 0); // Should be available at the top of the voxel.
		(*zpos)++;
	}

	/* Do some paranoia checking. */
	assert(*xpos < _world.GetXSize());
	assert(*ypos < _world.GetYSize());
	assert(*zpos < WORLD_Z_SIZE);
}

/** Compute the new contents of the voxel where the path should be added from the #_world. */
void PathBuildManager::ComputeWorldAdditions()
{
	assert(this->state == PBS_WAIT_BUY); // It needs selected_arrow and selected_slope.

	if (((1 << this->selected_slope) & this->allowed_slopes) == 0) return;

	uint16 xpos, ypos;
	uint8 zpos;
	this->ComputeArrowCursorPosition(&xpos, &ypos, &zpos);
	if (this->selected_slope == TSL_DOWN) zpos--;
	const Voxel *v = _world.GetVoxel(xpos, ypos, zpos);
	if (v == NULL || v->GetType() == VT_EMPTY) {
		Voxel *vx = _additions.GetCreateVoxel(xpos, ypos, zpos, true);
		vx->SetFoundationType(FDT_INVALID);
		vx->SetGroundType(GTP_INVALID);
		vx->SetPathRideNumber(PT_CONCRETE);
		uint8 slope = GetPathSprite(this->selected_slope, (TileEdge)((this->selected_arrow + 2) % 4));
		slope = AddRemovePathEdges(xpos, ypos, zpos, slope, EDGE_ALL, true, true); // Change the neighbouring edges too.
		vx->SetPathRideFlags(slope);
		return;
	} else if (v->GetType() == VT_SURFACE) {
		Voxel *vx = _additions.GetCreateVoxel(xpos, ypos, zpos, true);
		vx->SetFoundationType(v->GetFoundationType()); // Copy foundations
		vx->SetFoundationSlope(v->GetFoundationSlope());
		vx->SetGroundType(v->GetGroundType()); // Copy ground
		vx->SetGroundSlope(v->GetGroundSlope());
		vx->SetPathRideNumber(PT_CONCRETE);
		uint8 slope = GetPathSprite(this->selected_slope, (TileEdge)((this->selected_arrow + 2) % 4));
		slope = AddRemovePathEdges(xpos, ypos, zpos, slope, EDGE_ALL, true, true); // Change the neighbouring edges too.
		vx->SetPathRideFlags(slope);
		return;
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
	if (vp != NULL && this->state > PBS_WAIT_VOXEL && this->state <= PBS_WAIT_BUY) {
		vp->tile_cursor.SetCursor(this->xpos, this->ypos, this->zpos, CUR_TYPE_TILE);
	}

	/* See whether the PBS_WAIT_ARROW state can be left automatically. */
	if (this->state == PBS_WAIT_ARROW) {
		this->allowed_arrows = GetPathAttachPoints(this->xpos, this->ypos, this->zpos);

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
	if (vp != NULL) {
		if (this->state > PBS_WAIT_ARROW && this->state <= PBS_WAIT_BUY) {
			uint16 x_arrow, y_arrow;
			uint8 z_arrow;
			this->ComputeArrowCursorPosition(&x_arrow, &y_arrow, &z_arrow);
			vp->arrow_cursor.SetCursor(x_arrow, y_arrow, z_arrow, (CursorType)(CUR_TYPE_ARROW_NE + this->selected_arrow));
		} else {
			vp->arrow_cursor.SetInvalid();
		}
	}

	/* See whether the PBS_WAIT_SLOPE state can be left automatically. */
	if (this->state == PBS_WAIT_SLOPE) {
		/* Compute allowed slopes. */
		uint16 x_arrow, y_arrow;
		uint8 z_arrow;
		this->ComputeArrowCursorPosition(&x_arrow, &y_arrow, &z_arrow);
		this->allowed_slopes = CanBuildPathFromEdge(x_arrow, y_arrow, z_arrow, (TileEdge)((this->selected_arrow + 2) % 4));

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
	if (vp != NULL) {
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
 * Can the user press the 'remove' button at the path gui?
 * @return Button is enabled.
 */
bool PathBuildManager::GetRemoveIsEnabled() const
{
	if (this->state == PBS_IDLE || this->state == PBS_WAIT_VOXEL) return false;
	/* If current tile has a path, it can be removed. */
	const Voxel *v = _world.GetVoxel(this->xpos, this->ypos, this->zpos);
	if (v != NULL && v->GetType() == VT_SURFACE && HasValidPath(v)) return true;
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
		this->xlong = _world.GetXSize();
		this->ylong = _world.GetYSize();
		this->zlong = WORLD_Z_SIZE;
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
 * @todo Also allow laying path tiles on a #VT_SURFACE voxel with ground. It just needs careful checking that the path is at or above the ground.
 * @todo The path/ground check probably already exists somewhere, re-factor it for re-use here.
 */
void PathBuildManager::ComputeNewLongPath(const Point32 &mousexy)
{
	static const TrackSlope slope_prios_down[] = {TSL_DOWN, TSL_FLAT, TSL_UP,   TSL_INVALID}; // Order of preference when going down.
	static const TrackSlope slope_prios_flat[] = {TSL_FLAT, TSL_UP,   TSL_DOWN, TSL_INVALID}; // Order of preference when at the right height.
	static const TrackSlope slope_prios_up[]   = {TSL_UP,   TSL_FLAT, TSL_DOWN, TSL_INVALID}; // Order of preference when going up.

	Viewport *vp = GetViewport();
	if (vp == NULL) return;

	int c1, c2, c3;
	switch (vp->orientation) {
		case VOR_NORTH: c1 =  1; c2 =  2; c3 =  2; break;
		case VOR_EAST:  c1 = -1; c2 = -2; c3 =  2; break;
		case VOR_SOUTH: c1 =  1; c2 = -2; c3 = -2; break;
		case VOR_WEST:  c1 = -1; c2 =  2; c3 = -2; break;
		default: NOT_REACHED();
	}

	int32 vx, vy, vz;
	vy = this->ypos * 256 + 128;
	int32 lambda_y = vy - mousexy.y; // Distance to constant Y plane at current tile cursor.
	vx = this->xpos * 256 + 128;
	int32 lambda_x = vx - mousexy.x; // Distance to constant X plane at current tile cursor.

	if (abs(lambda_x) < abs(lambda_y)) {
		/* X constant. */
		vx /= 256;
		vy = Clamp<int32>(mousexy.y + c1 * lambda_x, 0, _world.GetYSize() * 256 - 1) / 256;
		vz = Clamp<int32>(vp->zview + c3 * lambda_x, 0, WORLD_Z_SIZE * 256 - 1) / 256;
	} else {
		/* Y constant. */
		vx = Clamp<int32>(mousexy.x + c1 * lambda_y, 0, _world.GetXSize() * 256 - 1) / 256;
		vy /= 256;
		vz = Clamp<int32>(vp->zview + c2 * lambda_y, 0, WORLD_Z_SIZE * 256 - 1) / 256;
	}

	if (this->xlong != vx || this->ylong != vy || this->zlong != vz) {
		this->xlong = vx;
		this->ylong = vy;
		this->zlong = vz;

		_additions.Clear();
		vx = this->xpos;
		vy = this->ypos;
		vz = this->zpos;
		/* Find the right direction from the selected tile to the current cursor location. */
		TileEdge direction;
		Point16 dxy;
		for (direction = EDGE_BEGIN; direction < EDGE_COUNT; direction++) {
			dxy = _tile_dxy[direction];
			if (!GoodDirection(dxy.x, vx, this->xlong) || !GoodDirection(dxy.y, vy, this->ylong)) continue;
			break;
		}
		if (direction == EDGE_COUNT) return;

		/* 'Walk' to the cursor as long as possible. */
		while (vx != this->xlong || vy != this->ylong) {
			uint8 slopes = CanBuildPathFromEdge(vx, vy, vz, direction);
			const TrackSlope *slope_prio;
			/* Get order of slope preference. */
			if (vz > this->zlong) {
				slope_prio = slope_prios_down;
			} else if (vz == this->zlong) {
				slope_prio = slope_prios_flat;
			} else {
				slope_prio = slope_prios_up;
			}
			/* Find best slope, and take it. */
			while (*slope_prio != TSL_INVALID) {
				if ((slopes & (1 << *slope_prio)) != 0) {
					vx += dxy.x;
					vy += dxy.y;
					/* Decide path tile. */
					uint8 path_tile;
					if (*slope_prio == TSL_UP) {
						path_tile = _path_down_from_edge[direction];
					} else if (*slope_prio == TSL_DOWN) {
						path_tile = _path_up_from_edge[direction];
						vz--;
					} else {
						path_tile = PATH_EMPTY;
					}
					/* Add path tile to the voxel. */
					Voxel *v = _additions.GetCreateVoxel(vx, vy, vz, true);
					if (v->GetType() == VT_EMPTY) {
						v->SetFoundationType(FDT_INVALID);
						v->SetGroundType(GTP_INVALID);
						v->SetPathRideNumber(PT_CONCRETE);
						v->SetPathRideFlags(AddRemovePathEdges(vx, vy, vz, path_tile, EDGE_ALL, true, true));
					} else if (v->GetType() == VT_SURFACE) {
						// XXX Handle this more careful, paths may exist and/or ground may exist here.
						v->SetPathRideNumber(PT_CONCRETE);
						v->SetPathRideFlags(AddRemovePathEdges(vx, vy, vz, path_tile, EDGE_ALL, true, true));
					} else {
						vx = this->xlong; // Run into some weird voxel, abort path building.
						vy = this->ylong;
						break;
					}

					if (*slope_prio == TSL_UP) {
						vz++;
					}
					break; // End finding a good slope.
				}
				slope_prio++;
			}
			if (*slope_prio == TSL_INVALID) break;
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
			this->xpos = this->xlong;
			this->ypos = this->ylong;
			this->zpos = this->zlong;
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
		Voxel *v = _world.GetCreateVoxel(this->xpos, this->ypos, this->zpos, false);
		if (v == NULL || v->GetType() != VT_SURFACE || v->GetPathRideNumber() == PT_INVALID) return;

		v->SetPathRideNumber(PT_INVALID);
		AddRemovePathEdges(this->xpos, this->ypos, this->zpos, v->GetPathRideFlags(), EDGE_ALL, false, false); // Change the neighbouring paths too.

		Viewport *vp = GetViewport();
		if (vp) vp->MarkVoxelDirty(this->xpos, this->ypos, this->zpos);

		/* Short-cut version of this->SelectMovement(false), as that function fails after removing the path. */
		TileEdge edge = (TileEdge)((this->selected_arrow + 2) % 4);
		bool move_up = (v->GetPathRideFlags() == _path_down_from_edge[edge]);
		this->MoveCursor(edge, move_up);
		this->UpdateState();
	}
}


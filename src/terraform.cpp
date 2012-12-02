/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file terraform.cpp Terraforming code. */

#include "stdafx.h"
#include "map.h"
#include "viewport.h"
#include "terraform.h"

TileTerraformMouseMode _terraformer; ///< Terraform coordination object.

/**
 * Structure describing a corner at a voxel stack.
 * @ingroup map_group
 */
struct VoxelCorner {
	Point16 rel_xy;   ///< Relative voxel stack position.
	TileSlope corner; ///< Corner of the voxel (#TC_NORTH, #TC_EAST, #TC_SOUTH or #TC_WEST).
};

/**
 * Description of neighbouring corners of a corner at a ground tile.
 * #left_neighbour and #right_neighbour are neighbours at the same tile, while
 * #neighbour_tiles list neighbouring corners at the three tiles around the
 * corner.
 * @ingroup map_group
 */
struct CornerNeighbours {
	TileSlope left_neighbour;       ///< Left neighbouring corner.
	TileSlope right_neighbour;      ///< Right neighbouring corner.
	VoxelCorner neighbour_tiles[3]; ///< Neighbouring corners at other tiles.
};

/**
 * Neighbouring corners of each corner.
 * @ingroup map_group
 */
static const CornerNeighbours neighbours[4] = {
	{TC_EAST,  TC_WEST,  { {{-1, -1}, TC_SOUTH}, {{-1,  0}, TC_WEST }, {{ 0, -1}, TC_EAST }} }, // TC_NORTH
	{TC_NORTH, TC_SOUTH, { {{-1,  0}, TC_SOUTH}, {{-1,  1}, TC_WEST }, {{ 0,  1}, TC_NORTH}} }, // TC_EAST
	{TC_EAST,  TC_WEST,  { {{ 0,  1}, TC_WEST }, {{ 1,  1}, TC_NORTH}, {{ 1,  0}, TC_EAST }} }, // TC_SOUTH
	{TC_SOUTH, TC_NORTH, { {{ 0, -1}, TC_SOUTH}, {{ 1, -1}, TC_EAST }, {{ 1,  0}, TC_NORTH}} }, // TC_WEST
};

/**
 * Construct a #GroundData structure.
 * @param height Height of the voxel containing the surface.
 * @param orig_slope Original slope (that is, before the raise or lower).
 */
GroundData::GroundData(uint8 height, uint8 orig_slope)
{
	this->height = height;
	this->orig_slope = orig_slope;
	this->modified = 0;
}

/**
 * Get original height (before changing).
 * @param corner Corner to get height.
 * @return Original height of the indicated corner.
 */
uint8 GroundData::GetOrigHeight(TileSlope corner) const
{
	assert(corner == TC_NORTH || corner == TC_EAST || corner == TC_SOUTH || corner == TC_WEST);
	if ((this->orig_slope & TCB_STEEP) == 0) { // Normal slope.
		if ((this->orig_slope & (1 << corner)) == 0) return this->height;
		return this->height + 1;
	}
	// Steep slope.
	if ((this->orig_slope & (1 << corner)) != 0) return this->height + 2;
	corner = (TileSlope)((corner + 2) % 4);
	if ((this->orig_slope & (1 << corner)) != 0) return this->height;
	return this->height + 1;
}

/**
 * Set the given corner as modified.
 * @param corner Corner to set.
 * @return corner is modified.
 */
void GroundData::SetCornerModified(TileSlope corner)
{
	assert(corner == TC_NORTH || corner == TC_EAST || corner == TC_SOUTH || corner == TC_WEST);
	this->modified |= 1 << corner;
}

/**
 * Return whether the given corner is modified or not.
 * @param corner Corner to test.
 * @return corner is modified.
 */
bool GroundData::GetCornerModified(TileSlope corner) const
{
	assert(corner == TC_NORTH || corner == TC_EAST || corner == TC_SOUTH || corner == TC_WEST);
	return (this->modified & (1 << corner)) != 0;
}

/**
 * Terrain changes storage constructor.
 * @param base Base coordinate of the part of the world which is smoothly updated.
 * @param xsize Horizontal size of the world part.
 * @param ysize Vertical size of the world part.
 */
TerrainChanges::TerrainChanges(const Point32 &base, uint16 xsize, uint16 ysize)
{
	assert(base.x >= 0 && base.y >= 0 && xsize > 0 && ysize > 0
			&& base.x + xsize <= _world.GetXSize() && base.y + ysize <= _world.GetYSize());
	this->base = base;
	this->xsize = xsize;
	this->ysize = ysize;
}

/** Destructor. */
TerrainChanges::~TerrainChanges()
{
}

/**
 * Get ground data of a voxel stack.
 * @param pos Voxel stack position.
 * @return Pointer to the ground data, or \c NULL if outside the world.
 */
GroundData *TerrainChanges::GetGroundData(const Point32 &pos)
{
	if (pos.x < this->base.x || pos.x >= this->base.x + this->xsize) return NULL;
	if (pos.y < this->base.y || pos.y >= this->base.y + this->ysize) return NULL;

	GroundModificationMap::iterator iter = this->changes.find(pos);
	if (iter == this->changes.end()) {
		uint8 height = _world.GetGroundHeight(pos.x, pos.y);
		const Voxel *v = _world.GetVoxel(pos.x, pos.y, height);
		assert(v != NULL && v->GetType() == VT_SURFACE && v->GetGroundType() != GTP_INVALID);
		std::pair<Point32, GroundData> p(pos, GroundData(height, ExpandTileSlope(v->GetGroundSlope())));
		iter = this->changes.insert(p).first;
	}
	return &(*iter).second;
}


/**
 * Change the height of a corner. Call this function for every corner you want to change.
 * @param pos Position of the voxel stack.
 * @param corner Corner to change.
 * @param direction Direction of change.
 * @return Change is OK for the map.
 */
bool TerrainChanges::ChangeCorner(const Point32 &pos, TileSlope corner, int direction)
{
	assert(corner == TC_NORTH || corner == TC_EAST || corner == TC_SOUTH || corner == TC_WEST);
	assert(direction == 1 || direction == -1);

	GroundData *gd = this->GetGroundData(pos);
	if (gd == NULL) return true; // Out of the bounds in the world, silently ignore.
	if (gd->GetCornerModified(corner)) return true; // Corner already changed.

	uint8 old_height = gd->GetOrigHeight(corner);
	if (direction > 0 && old_height == WORLD_Z_SIZE) return false; // Cannot change above top.
	if (direction < 0 && old_height == 0) return false; // Cannot change below bottom.

	gd->SetCornerModified(corner); // Mark corner as modified.

	/* Change neighbouring corners at the same tile. */
	if (direction > 0) {
		uint8 h = gd->GetOrigHeight(neighbours[corner].left_neighbour);
		if (h < old_height && !this->ChangeCorner(pos, neighbours[corner].left_neighbour, direction)) return false;
		h = gd->GetOrigHeight(neighbours[corner].right_neighbour);
		if (h < old_height && !this->ChangeCorner(pos, neighbours[corner].right_neighbour, direction)) return false;
	} else {
		uint8 h = gd->GetOrigHeight(neighbours[corner].left_neighbour);
		if (h > old_height && !this->ChangeCorner(pos, neighbours[corner].left_neighbour, direction)) return false;
		h = gd->GetOrigHeight(neighbours[corner].right_neighbour);
		if (h > old_height && !this->ChangeCorner(pos, neighbours[corner].right_neighbour, direction)) return false;
	}

	for (uint i = 0; i < 3; i++) {
		const VoxelCorner &vc = neighbours[corner].neighbour_tiles[i];
		Point32 pos2;
		pos2.x = pos.x + vc.rel_xy.x;
		pos2.y = pos.y + vc.rel_xy.y;
		gd = this->GetGroundData(pos2);
		if (gd == NULL) continue;
		uint height = gd->GetOrigHeight(vc.corner);
		if (old_height == height && !this->ChangeCorner(pos2, vc.corner, direction)) return false;
	}
	return true;
}

/**
 * Perform the proposed changes.
 * @param direction Direction of change.
 */
void TerrainChanges::ChangeWorld(int direction)
{
	for (GroundModificationMap::iterator iter = this->changes.begin(); iter != this->changes.end(); iter++) {
		const Point32 &pos = (*iter).first;
		const GroundData &gd = (*iter).second;
		if (gd.modified == 0) continue;

		VoxelStack *vs = _world.GetModifyStack(pos.x, pos.y);
		Voxel *v = vs->GetCreate(gd.height, false);
		GroundType gtype = v->GetGroundType();
		FoundationType ftype = v->GetFoundationType();
		uint16 ptype = v->GetPathRideNumber();
		/* Clear existing ground and foundations. */
		v->SetEmpty();
		if ((gd.orig_slope & TCB_STEEP) != 0) vs->GetCreate(gd.height + 1, false)->SetEmpty();

		uint8 new_heights[4];
		uint8 max_h = 0;
		uint8 min_h = 255;
		for (uint corner = 0; corner < 4; corner++) {
			uint8 height = gd.GetOrigHeight((TileSlope)corner);
			if (gd.GetCornerModified((TileSlope)corner)) {
				assert((direction > 0 && height < 255) || (direction < 0 && height > 0));
				height += direction;
			}
			new_heights[corner] = height;
			if (max_h < height) max_h = height;
			if (min_h > height) min_h = height;
		}
		v = vs->GetCreate(min_h, true);
		v->SetGroundType(gtype);
		v->SetFoundationType(ftype);
		v->SetFoundationSlope(0); // XXX Needs further work.
		v->SetPathRideNumber(ptype);
		v->SetPathRideFlags(0); // XXX Needs further work.
		if (max_h - min_h <= 1) {
			/* Normal slope. */
			v->SetGroundSlope(ImplodeTileSlope(((new_heights[TC_NORTH] > min_h) ? TCB_NORTH : SL_FLAT)
					| ((new_heights[TC_EAST]  > min_h) ? TCB_EAST  : SL_FLAT)
					| ((new_heights[TC_SOUTH] > min_h) ? TCB_SOUTH : SL_FLAT)
					| ((new_heights[TC_WEST]  > min_h) ? TCB_WEST  : SL_FLAT)));
		} else {
			assert(max_h - min_h == 2);
			int corner;
			for (corner = 0; corner < 4; corner++) {
				if (new_heights[corner] == max_h) break;
			}
			assert(corner <= TC_WEST);
			v->SetGroundSlope(ImplodeTileSlope(TCB_STEEP | (TileSlope)(1 << corner)));
			vs->GetCreate(min_h + 1, true)->SetReferencePosition(pos.x, pos.y, min_h);
		}
	}
}


TileTerraformMouseMode::TileTerraformMouseMode() : MouseMode(WC_NONE, MM_TILE_TERRAFORM)
{
	this->state = TFS_OFF;
}

/** Terraform gui window just opened. */
void TileTerraformMouseMode::OpenWindow()
{
	if (this->state == TFS_OFF) {
		this->state = TFS_NO_MOUSE;
		_mouse_modes.SetMouseMode(this->mode);
	}
}

/** Terraform gui window just closed. */
void TileTerraformMouseMode::CloseWindow()
{
	if (this->state == TFS_ON) {
		this->state = TFS_OFF; // Prevent enabling again.
		_mouse_modes.SetViewportMousemode();
	}
	this->state = TFS_OFF; // In case it did not have a mouse mode.
}

/* virtual */ bool TileTerraformMouseMode::MayActivateMode()
{
	return this->state != TFS_OFF;
}

/* virtual */ void TileTerraformMouseMode::ActivateMode(const Point16 &pos)
{
	this->mouse_state = 0;
}

/* virtual */ void TileTerraformMouseMode::LeaveMode()
{
	Viewport *vp = GetViewport();
	if (vp != NULL) vp->tile_cursor.SetInvalid();
	if (this->state == TFS_ON) this->state = TFS_NO_MOUSE;
}

/* virtual */ bool TileTerraformMouseMode::EnableCursors()
{
	return this->state != TFS_OFF;
}

/* virtual */ void TileTerraformMouseMode::OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos)
{
	uint16 xvoxel, yvoxel;
	uint8 zvoxel;
	CursorType cur_type;

	if ((this->mouse_state & MB_RIGHT) != 0) {
		/* Drag the window if button is pressed down. */
		vp->MoveViewport(pos.x - old_pos.x, pos.y - old_pos.y);
	} else {
		if (vp->ComputeCursorPosition(true, &xvoxel, &yvoxel, &zvoxel, &cur_type)) {
			vp->tile_cursor.SetCursor(xvoxel, yvoxel, zvoxel, cur_type);
		}
	}
}
/* virtual */ void TileTerraformMouseMode::OnMouseButtonEvent(Viewport *vp, uint8 state)
{
	this->mouse_state = state & MB_CURRENT;
}

/* virtual */ void TileTerraformMouseMode::OnMouseWheelEvent(Viewport *vp, int direction)
{
	Point32 p;
	p.x = 0;
	p.y = 0;
	TerrainChanges changes(p, _world.GetXSize(), _world.GetYSize());

	p.x = vp->tile_cursor.xpos;
	p.y = vp->tile_cursor.ypos;

	bool ok = false;
	switch (vp->tile_cursor.type) {
		case CUR_TYPE_NORTH:
		case CUR_TYPE_EAST:
		case CUR_TYPE_SOUTH:
		case CUR_TYPE_WEST:
			ok = changes.ChangeCorner(p, (TileSlope)(vp->tile_cursor.type - CUR_TYPE_NORTH), direction);
			break;

		case CUR_TYPE_TILE:
			ok = changes.ChangeCorner(p, TC_NORTH, direction);
			if (ok) ok = changes.ChangeCorner(p, TC_EAST, direction);
			if (ok) ok = changes.ChangeCorner(p, TC_SOUTH, direction);
			if (ok) ok = changes.ChangeCorner(p, TC_WEST, direction);
			break;

		default:
			NOT_REACHED();
	}

	if (ok) {
		changes.ChangeWorld(direction);
		/* Move voxel selection along with the terrain to allow another mousewheel event at the same place.
		 * Note that the mouse cursor position is not changed at all, it still points at the original position.
		 * The coupling is restored with the next mouse movement.
		 */
		vp->tile_cursor.zpos = _world.GetGroundHeight(vp->tile_cursor.xpos, vp->tile_cursor.ypos);
		GroundModificationMap::const_iterator iter;
		for (iter = changes.changes.begin(); iter != changes.changes.end(); iter++) {
			const Point32 &pt = (*iter).first;
			vp->MarkVoxelDirty(pt.x, pt.y, (*iter).second.height);
		}
	}
}


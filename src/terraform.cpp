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
#include "math_func.h"
#include "memory.h"

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
 * Test every corner of the given voxel for its original height, and find the extreme value.
 * @param pos %Voxel position.
 * @param direction Leveling direction (decides whether to find the lowest or highest corner).
 * @param height [inout] Extremest height found so far.
 */
void TerrainChanges::UpdateLevelingHeight(const Point32 &pos, int direction, uint8 *height)
{
	const GroundData *gd = this->GetGroundData(pos);

	if (direction > 0) { // Raising terrain, find the lowest corner.
		*height = min(*height, gd->GetOrigHeight(TC_NORTH));
		*height = min(*height, gd->GetOrigHeight(TC_EAST));
		*height = min(*height, gd->GetOrigHeight(TC_SOUTH));
		*height = min(*height, gd->GetOrigHeight(TC_WEST));
	} else { // Lowering terrain, find the highest corner.
		*height = max(*height, gd->GetOrigHeight(TC_NORTH));
		*height = max(*height, gd->GetOrigHeight(TC_EAST));
		*height = max(*height, gd->GetOrigHeight(TC_SOUTH));
		*height = max(*height, gd->GetOrigHeight(TC_WEST));
	}
}

/**
 * Change corners of a voxel if they are within the height constraint.
 * @param pos %Voxel position.
 * @param height Minimum or maximum height of the corners to modify.
 * @param direction Leveling direction (decides what constraint to use).
 * @return Change is OK for the map.
 */
bool TerrainChanges::ChangeVoxel(const Point32 &pos, uint8 height, int direction)
{
	GroundData *gd = this->GetGroundData(pos);
	bool ok = true;
	if (direction > 0) { // Raising terrain, raise everything at or below 'height'.
		if (gd->GetOrigHeight(TC_NORTH) <= height) ok &= this->ChangeCorner(pos, TC_NORTH, direction);
		if (gd->GetOrigHeight(TC_EAST)  <= height) ok &= this->ChangeCorner(pos, TC_EAST,  direction);
		if (gd->GetOrigHeight(TC_SOUTH) <= height) ok &= this->ChangeCorner(pos, TC_SOUTH, direction);
		if (gd->GetOrigHeight(TC_WEST)  <= height) ok &= this->ChangeCorner(pos, TC_WEST,  direction);
	} else { // Lowering terrain, lower everything above or at 'height'.
		if (gd->GetOrigHeight(TC_NORTH) >= height) ok &= this->ChangeCorner(pos, TC_NORTH, direction);
		if (gd->GetOrigHeight(TC_EAST)  >= height) ok &= this->ChangeCorner(pos, TC_EAST,  direction);
		if (gd->GetOrigHeight(TC_SOUTH) >= height) ok &= this->ChangeCorner(pos, TC_SOUTH, direction);
		if (gd->GetOrigHeight(TC_WEST)  >= height) ok &= this->ChangeCorner(pos, TC_WEST,  direction);
	}
	return ok;
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
 * Set an upper boundary of the height of each tile corner based on the contents of a voxel.
 * @param v %Voxel to examine.
 * @param height Height of the voxel.
 * @param bounds [inout] Updated constraints.
 * @note Length of the \a bounds array should be 4.
 */
static void SetUpperBoundary(const Voxel *v, uint8 height, uint8 *bounds)
{
	if (v == NULL || v->GetType() == VT_EMPTY) return;
	if (v->GetType() == VT_REFERENCE) {
		MemSetT(bounds, height, 4);
		return;
	}

	assert(v->GetType() == VT_SURFACE);
	assert(v->GetGroundType() == GTP_INVALID);
	if (v->GetPathRideNumber() == PT_INVALID) return;

	if (v->GetPathRideNumber() < PT_START) { // A ride needs the entire voxel.
		MemSetT(bounds, height, 4);
		return;
	}

	/* A path. */
	PathSprites ps = static_cast<PathSprites>(v->GetPathRideFlags());
	switch (ps) {
		case PATH_RAMP_NE:
			bounds[TC_NORTH] = height;     bounds[TC_EAST] = height;
			bounds[TC_SOUTH] = height + 1; bounds[TC_WEST] = height + 1;
			break;

		case PATH_RAMP_NW:
			bounds[TC_NORTH] = height;     bounds[TC_WEST] = height;
			bounds[TC_SOUTH] = height + 1; bounds[TC_EAST] = height + 1;
			break;

		case PATH_RAMP_SE:
			bounds[TC_SOUTH] = height;     bounds[TC_EAST] = height;
			bounds[TC_NORTH] = height + 1; bounds[TC_WEST] = height + 1;
			break;

		case PATH_RAMP_SW:
			bounds[TC_SOUTH] = height;     bounds[TC_WEST] = height;
			bounds[TC_NORTH] = height + 1; bounds[TC_EAST] = height + 1;
			break;

		default:
			assert(ps < PATH_FLAT_COUNT);
			MemSetT(bounds, height, 4);
			break;
	}
}

/**
 * Update the foundations in two #_additions voxel stacks along the SW edge of the first stack and the NE edge of the second stack.
 * @param xpos X position of the first voxel stack.
 * @param ypos Y position of the first voxel stack.
 * @note The first or the second voxel stack may be off-world.
 */
static void SetXFoundations(int xpos, int ypos)
{
	VoxelStack *first = (xpos < 0) ? NULL : _additions.GetModifyStack(xpos, ypos);
	VoxelStack *second = (xpos + 1 == _world.GetXSize()) ? NULL : _additions.GetModifyStack(xpos + 1, ypos);
	assert(first != NULL || second != NULL);

	/* Get ground height at all corners. */
	uint8 first_south = 0;
	uint8 first_west  = 0;
	if (first != NULL) {
		for (uint i = 0; i < first->height; i++) {
			const Voxel *v = &first->voxels[i];
			if (v->GetType() != VT_SURFACE || v->GetGroundType() == GTP_INVALID) continue;
			uint8 heights[4];
			ComputeCornerHeight(ExpandTileSlope(v->GetGroundSlope()), first->base + i, heights);
			first_south = heights[TC_SOUTH];
			first_west  = heights[TC_WEST];
			break;
		}
	}

	uint8 second_north = 0;
	uint8 second_east  = 0;
	if (second != NULL) {
		for (uint i = 0; i < second->height; i++) {
			const Voxel *v = &second->voxels[i];
			if (v->GetType() != VT_SURFACE || v->GetGroundType() == GTP_INVALID) continue;
			uint8 heights[4];
			ComputeCornerHeight(ExpandTileSlope(v->GetGroundSlope()), second->base + i, heights);
			second_north = heights[TC_NORTH];
			second_east  = heights[TC_EAST];
			break;
		}
	}

	/* Set foundations. */
	if (first != NULL) {
		for (uint i = 0; i < first->height; i++) {
			uint8 h = first->base + i;

			uint8 bits = 0;
			if (h >= second_east || h >= second_north) {
				if (h < first_south) bits |= 0x10;
				if (h < first_west)  bits |= 0x20;
			}

			Voxel *v = &first->voxels[i];
			if (bits == 0) { // Delete foundations.
				if (v->GetType() != VT_SURFACE || v->GetFoundationType() == FDT_INVALID) continue;
				bits = v->GetFoundationSlope() & ~0x30;
				v->SetFoundationSlope(bits);
				if (bits == 0) v->SetFoundationType(FDT_INVALID);
				continue;
			} else { // Add foundations.
				if (v->GetType() == VT_REFERENCE) continue; // XXX BUG, cannot add foundations to reference voxels.
				if (v->GetType() == VT_EMPTY) {
					v->SetPathRideNumber(PT_INVALID);
					v->SetGroundType(GTP_INVALID);
					v->SetFoundationType(FDT_GROUND); // XXX
				} else if (v->GetFoundationType() == FDT_INVALID) {
					v->SetFoundationType(FDT_GROUND); // XXX We have nice foundation types, but no space to get them from.
				} else {
					bits |= (v->GetFoundationSlope() & ~0x30);
				}
				v->SetFoundationSlope(bits);
				continue;
			}
		}
	}

	if (second != NULL) {
		for (uint i = 0; i < second->height; i++) {
			uint8 h = second->base + i;

			uint8 bits = 0;
			if (h >= first_south || h >= first_west) {
				if (h < second_north) bits |= 0x01;
				if (h < second_east)  bits |= 0x02;
			}

			Voxel *v = &second->voxels[i];
			if (bits == 0) { // Delete foundations.
				if (v->GetType() != VT_SURFACE || v->GetFoundationType() == FDT_INVALID) continue;
				bits = v->GetFoundationSlope() & ~0x03;
				v->SetFoundationSlope(bits);
				if (bits == 0) v->SetFoundationType(FDT_INVALID);
				continue;
			} else { // Add foundations.
				if (v->GetType() == VT_REFERENCE) continue; // XXX BUG, cannot add foundations to reference voxels.
				if (v->GetType() == VT_EMPTY) {
					v->SetPathRideNumber(PT_INVALID);
					v->SetGroundType(GTP_INVALID);
					v->SetFoundationType(FDT_GROUND); // XXX
				} else if (v->GetFoundationType() == FDT_INVALID) {
					v->SetFoundationType(FDT_GROUND); // XXX We have nice foundation types, but no space to get them from.
				} else {
					bits |= (v->GetFoundationSlope() & ~0x03);
				}
				v->SetFoundationSlope(bits);
				continue;
			}
		}
	}
}

/**
 * Update the foundations in two #_additions voxel stacks along the SE edge of the first stack and the NW edge of the second stack.
 * @param xpos X position of the first voxel stack.
 * @param ypos Y position of the first voxel stack.
 * @note The first or the second voxel stack may be off-world.
 */
static void SetYFoundations(int xpos, int ypos)
{
	VoxelStack *first = (ypos < 0) ? NULL : _additions.GetModifyStack(xpos, ypos);
	VoxelStack *second = (ypos + 1 == _world.GetYSize()) ? NULL : _additions.GetModifyStack(xpos, ypos + 1);
	assert(first != NULL || second != NULL);

	/* Get ground height at all corners. */
	uint8 first_south = 0;
	uint8 first_east  = 0;
	if (first != NULL) {
		for (uint i = 0; i < first->height; i++) {
			const Voxel *v = &first->voxels[i];
			if (v->GetType() != VT_SURFACE || v->GetGroundType() == GTP_INVALID) continue;
			uint8 heights[4];
			ComputeCornerHeight(ExpandTileSlope(v->GetGroundSlope()), first->base + i, heights);
			first_south = heights[TC_SOUTH];
			first_east  = heights[TC_EAST];
			break;
		}
	}

	uint8 second_north = 0;
	uint8 second_west  = 0;
	if (second != NULL) {
		for (uint i = 0; i < second->height; i++) {
			const Voxel *v = &second->voxels[i];
			if (v->GetType() != VT_SURFACE || v->GetGroundType() == GTP_INVALID) continue;
			uint8 heights[4];
			ComputeCornerHeight(ExpandTileSlope(v->GetGroundSlope()), second->base + i, heights);
			second_north = heights[TC_NORTH];
			second_west  = heights[TC_WEST];
			break;
		}
	}

	/* Set foundations. */
	if (first != NULL) {
		for (uint i = 0; i < first->height; i++) {
			uint8 h = first->base + i;

			uint8 bits = 0;
			if (h >= second_west || h >= second_north) {
				if (h < first_south) bits |= 0x08;
				if (h < first_east)  bits |= 0x04;
			}

			Voxel *v = &first->voxels[i];
			if (bits == 0) { // Delete foundations.
				if (v->GetType() != VT_SURFACE || v->GetFoundationType() == FDT_INVALID) continue;
				bits = v->GetFoundationSlope() & ~0x0C;
				v->SetFoundationSlope(bits);
				if (bits == 0) v->SetFoundationType(FDT_INVALID);
				continue;
			} else { // Add foundations.
				if (v->GetType() == VT_REFERENCE) continue; // XXX BUG, cannot add foundations to reference voxels.
				if (v->GetType() == VT_EMPTY) {
					v->SetPathRideNumber(PT_INVALID);
					v->SetGroundType(GTP_INVALID);
					v->SetFoundationType(FDT_GROUND); // XXX
				} else if (v->GetFoundationType() == FDT_INVALID) {
					v->SetFoundationType(FDT_GROUND); // XXX We have nice foundation types, but no space to get them from.
				} else {
					bits |= (v->GetFoundationSlope() & ~0x0C);
				}
				v->SetFoundationSlope(bits);
				continue;
			}
		}
	}

	if (second != NULL) {
		for (uint i = 0; i < second->height; i++) {
			uint8 h = second->base + i;

			uint8 bits = 0;
			if (h >= first_south || h >= first_east) {
				if (h < second_north) bits |= 0x80;
				if (h < second_west)  bits |= 0x40;
			}

			Voxel *v = &second->voxels[i];
			if (bits == 0) { // Delete foundations.
				if (v->GetType() != VT_SURFACE || v->GetFoundationType() == FDT_INVALID) continue;
				bits = v->GetFoundationSlope() & ~0xC0;
				v->SetFoundationSlope(bits);
				if (bits == 0) v->SetFoundationType(FDT_INVALID);
				continue;
			} else { // Add foundations.
				if (v->GetType() == VT_REFERENCE) continue; // XXX BUG, cannot add foundations to reference voxels.
				if (v->GetType() == VT_EMPTY) {
					v->SetPathRideNumber(PT_INVALID);
					v->SetGroundType(GTP_INVALID);
					v->SetFoundationType(FDT_GROUND); // XXX
				} else if (v->GetFoundationType() == FDT_INVALID) {
					v->SetFoundationType(FDT_GROUND); // XXX We have nice foundation types, but no space to get them from.
				} else {
					bits |= (v->GetFoundationSlope() & ~0xC0);
				}
				v->SetFoundationSlope(bits);
				continue;
			}
		}
	}
}

/**
 * Perform the proposed changes in #_additions (and committing them afterwards).
 * @param direction Direction of change.
 * @return Whether the change could actually be performed (else nothing is changed).
 */
bool TerrainChanges::ModifyWorld(int direction)
{
	assert(_mouse_modes.GetMouseMode() == MM_TILE_TERRAFORM);

	_additions.Clear();

	/* First iteration: Change the ground of the tiles, checking whether the change is actually allowed with the other game elements. */
	for (GroundModificationMap::iterator iter = this->changes.begin(); iter != this->changes.end(); iter++) {
		const Point32 &pos = (*iter).first;
		const GroundData &gd = (*iter).second;
		if (gd.modified == 0) continue;

		uint8 current[4]; // Height of each corner after applying modification.
		ComputeCornerHeight(static_cast<TileSlope>(gd.orig_slope), gd.height, current);

		/* Apply modification. */
		for (uint8 i = TC_NORTH; i < TC_END; i++) {
			if ((gd.modified & (1 << i)) == 0) continue; // Corner was not changed.
			current[i] += direction;
		}

		/* Clear the current ground from the stack. */
		VoxelStack *vs = _additions.GetModifyStack(pos.x, pos.y);
		Voxel *v = vs->GetCreate(gd.height, false);
		assert(v != NULL && v->GetType() == VT_SURFACE);
		GroundType gt = v->GetGroundType();
		assert(gt != GTP_INVALID);
		FoundationType ft = v->GetFoundationType();
		v->SetGroundType(GTP_INVALID);
		if ((v->GetGroundSlope() & TCB_STEEP) != 0) {
			Voxel *w = vs->GetCreate(gd.height + 1, false);
			assert(w != NULL && w->GetType() == VT_REFERENCE);
			w->SetEmpty();
		}
		v->SetGroundSlope(0);
		if (ft == FDT_INVALID && v->GetPathRideNumber() == PT_INVALID) { // Voxel is empty, change its type.
			v->SetEmpty();
		}

		if (direction > 0) {
			/* Moving upwards, compute upper bound on corner heights. */
			uint8 max_above[4];
			MemSetT(max_above, gd.height + 3, lengthof(max_above));

			for (int i = 2; i >= 0; i--) {
				SetUpperBoundary(vs->Get(gd.height + i), gd.height + i, max_above);
			}

			/* Check boundaries. */
			for (uint i = 0; i < 4; i++) {
				if (current[i] > max_above[i]) return false;
			}
		} /* else: Moving downwards always works, since there is nothing underground yet. */

		/* Add new ground to the stack. */
		TileSlope new_slope;
		uint8 height;
		ComputeSlopeAndHeight(current, &new_slope, &height);

		v = vs->GetCreate(height, true);
		assert(v->GetType() != VT_REFERENCE);
		if (v->GetType() != VT_SURFACE) v->SetPathRideNumber(PT_INVALID);
		v->SetGroundSlope(new_slope);
		v->SetGroundType(gt);
		v->SetFoundationType(ft);
		v->SetFoundationSlope(0);

		if ((new_slope & TCB_STEEP)) {
			v = vs->GetCreate(height + 1, true);
			v->SetReferencePosition(pos.x, pos.y, height);
		}
	}

	/* Second iteration: Add foundations to every changed tile edge.
	 * The general idea is that each modified voxel handles adding of foundation to its SE and SW edge.
	 * If the NE or NW voxel is not modified, the voxel will have to perform adding of foundations there as well.
	 */
	for (GroundModificationMap::iterator iter = this->changes.begin(); iter != this->changes.end(); iter++) {
		const Point32 &pos = (*iter).first;
		const GroundData &gd = (*iter).second;
		if (gd.modified == 0) continue;

		SetXFoundations(pos.x, pos.y);
		SetYFoundations(pos.x, pos.y);

		Point32 pt;
		pt.x = pos.x - 1;
		pt.y = pos.y;
		GroundModificationMap::const_iterator iter2 = this->changes.find(pt);
		if (iter2 == this->changes.end()) {
			SetXFoundations(pt.x, pt.y);
		} else {
			const GroundData &gd = (*iter).second;
			if (gd.modified == 0) SetXFoundations(pt.x, pt.y);
		}

		pt.x = pos.x;
		pt.y = pos.y - 1;
		iter2 = this->changes.find(pt);
		if (iter2 == this->changes.end()) {
			SetYFoundations(pt.x, pt.y);
		} else {
			const GroundData &gd = (*iter).second;
			if (gd.modified == 0) SetYFoundations(pt.x, pt.y);
		}
	}

	_additions.Commit();
	return true;
}

TileTerraformMouseMode::TileTerraformMouseMode() : MouseMode(WC_NONE, MM_TILE_TERRAFORM)
{
	this->state = TFS_OFF;
	this->mouse_state = 0;
	this->xsize = 1;
	this->ysize = 1;
	this->leveling = true;
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
	this->state = TFS_OFF; // In case the mouse mode was not active.
}

/**
 * Update the size of the terraform area.
 * @param xsize Horizontal length of the area. May be \c 0.
 * @param ysize Vertical length of the area. May be \c 0.
 */
void TileTerraformMouseMode::SetSize(int xsize, int ysize)
{
	this->xsize = xsize;
	this->ysize = ysize;
	if (this->state != TFS_ON) return;
	this->SetCursors();
}

/**
 * Set the 'leveling' mode. This has no further visible effect until a raise/lower action is performed.
 * @param leveling The new leveling setting.
 */
void TileTerraformMouseMode::SetLeveling(bool leveling)
{
	this->leveling = leveling;
}

/* virtual */ bool TileTerraformMouseMode::MayActivateMode()
{
	return this->state != TFS_OFF;
}

/* virtual */ void TileTerraformMouseMode::ActivateMode(const Point16 &pos)
{
	this->mouse_state = 0;
	this->state = TFS_ON;
	this->SetCursors();
}

/** Set/modify the cursors of the viewport. */
void TileTerraformMouseMode::SetCursors()
{
	Viewport *vp = GetViewport();
	if (vp == NULL) return;

	uint16 xvoxel, yvoxel;
	uint8 zvoxel;
	CursorType cur_type;

	bool single = this->xsize <= 1 && this->ysize <= 1;
	if (vp->ComputeCursorPosition(single, &xvoxel, &yvoxel, &zvoxel, &cur_type)) {
		if (single) {
			vp->tile_cursor.SetCursor(xvoxel, yvoxel, zvoxel, cur_type);
			vp->area_cursor.SetInvalid();
		} else {
			Rectangle32 rect(xvoxel - this->xsize / 2, yvoxel - this->ysize / 2, this->xsize, this->ysize);
			vp->tile_cursor.SetInvalid();
			vp->area_cursor.SetCursor(rect, CUR_TYPE_TILE);
		}
	}
}

/* virtual */ void TileTerraformMouseMode::LeaveMode()
{
	Viewport *vp = GetViewport();
	if (vp != NULL) {
		vp->tile_cursor.SetInvalid();
		vp->area_cursor.SetInvalid();
	}
	if (this->state == TFS_ON) this->state = TFS_NO_MOUSE;
}

/* virtual */ bool TileTerraformMouseMode::EnableCursors()
{
	return this->state != TFS_OFF;
}

/* virtual */ void TileTerraformMouseMode::OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos)
{
	if ((this->mouse_state & MB_RIGHT) != 0) {
		/* Drag the window if button is pressed down. */
		vp->MoveViewport(pos.x - old_pos.x, pos.y - old_pos.y);
	} else {
		this->SetCursors();
	}
}
/* virtual */ void TileTerraformMouseMode::OnMouseButtonEvent(Viewport *vp, uint8 state)
{
	this->mouse_state = state & MB_CURRENT;
}

/**
 * Change the terrain while in 'dot' mode (ie a single corner or a single tile changing the entire world).
 * @param vp %Viewport displaying the world.
 * @param leveling If \c true, use leveling mode (only change the lowest/highest corners of a tile), else move every corner.
 * @param direction Direction of change.
 * @param dot_mode Using dot-mode (infinite world changes).
 */
static void ChangeTileCursorMode(Viewport *vp, bool leveling, int direction, bool dot_mode)
{
	Cursor *c = &vp->tile_cursor;

	Point32 p;
	uint16 w, h;

	if (dot_mode) { // Change entire world.
		p.x = 0;
		p.y = 0;
		w = _world.GetXSize();
		h = _world.GetYSize();
	} else { // Single tile mode.
		p.x = c->xpos;
		p.y = c->ypos;
		w = 1;
		h = 1;
	}
	TerrainChanges changes(p, w, h);

	p.x = c->xpos;
	p.y = c->ypos;

	bool ok;
	switch (c->type) {
		case CUR_TYPE_NORTH:
		case CUR_TYPE_EAST:
		case CUR_TYPE_SOUTH:
		case CUR_TYPE_WEST:
			ok = changes.ChangeCorner(p, (TileSlope)(c->type - CUR_TYPE_NORTH), direction);
			break;

		case CUR_TYPE_TILE: {
			uint8 height = (direction > 0) ? WORLD_Z_SIZE : 0;
			if (leveling) changes.UpdateLevelingHeight(p, direction, &height);
			ok = changes.ChangeVoxel(p, height, direction);
			break;
		}

		default:
			NOT_REACHED();
	}

	if (ok) {
		ok = changes.ModifyWorld(direction);
		_additions.Clear();
		if (!ok) return;

		/* Move voxel selection along with the terrain to allow another mousewheel event at the same place.
		 * Note that the mouse cursor position is not changed at all, it still points at the original position.
		 * The coupling is restored with the next mouse movement.
		 */
		c->zpos = _world.GetGroundHeight(c->xpos, c->ypos);
		GroundModificationMap::const_iterator iter;
		for (iter = changes.changes.begin(); iter != changes.changes.end(); iter++) {
			const Point32 &pt = (*iter).first;
			vp->MarkVoxelDirty(pt.x, pt.y, (*iter).second.height);
		}
	}
}

/**
 * Change the terrain while in 'area' mode (ie a rectangle of tiles that changes).
 * @param vp %Viewport displaying the world.
 * @param leveling If \c true, use leveling mode (only change the lowest/highest corners of a tile), else move every corner.
 * @param direction Direction of change.
 */
static void ChangeAreaCursorMode(Viewport *vp, bool leveling, int direction)
{
	Point32 p;

	MultiCursor *c = &vp->area_cursor;
	TerrainChanges changes(c->rect.base, c->rect.width, c->rect.height);

	uint8 height = (direction > 0) ? WORLD_Z_SIZE : 0;
	if (leveling) {
		for (uint dx = 0; dx < c->rect.width; dx++) {
			p.x = c->rect.base.x + dx;
			for (uint dy = 0; dy < c->rect.height; dy++) {
				p.y = c->rect.base.y + dy;
				changes.UpdateLevelingHeight(p, direction, &height);
			}
		}
	}

	/* Make the change in 'changes'. */
	for (uint dx = 0; dx < c->rect.width; dx++) {
		p.x = c->rect.base.x + dx;
		for (uint dy = 0; dy < c->rect.height; dy++) {
			p.y = c->rect.base.y + dy;
			bool ok = changes.ChangeVoxel(p, height, direction);
			if (!ok) return;
		}
	}

	bool ok = changes.ModifyWorld(direction);
	_additions.Clear();
	if (!ok) return;

	/* Like the dotmode, the cursor position is changed, but the mouse position is not touched to allow more
	 * mousewheel events to happen at the same place.
	 */
	GroundModificationMap::const_iterator iter;
	for (iter = changes.changes.begin(); iter != changes.changes.end(); iter++) {
		const Point32 &pt = (*iter).first;
		c->ResetZPosition(pt);
		vp->MarkVoxelDirty(pt.x, pt.y, (*iter).second.height);
	}
}

/* virtual */ void TileTerraformMouseMode::OnMouseWheelEvent(Viewport *vp, int direction)
{
	if (vp->tile_cursor.type != CUR_TYPE_INVALID) {
		ChangeTileCursorMode(vp, this->leveling, direction, this->xsize == 0 && this->ysize == 0);
	} else {
		ChangeAreaCursorMode(vp, this->leveling, direction);
	}
}


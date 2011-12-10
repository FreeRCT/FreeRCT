/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map.cpp Voxels of the world. */

#include "stdafx.h"
#include "map.h"
#include "memory.h"

VoxelWorld _world; ///< The game world.

/** Structure describing a corner at a voxel stack. */
struct VoxelCorner {
	Point16 rel_xy;   ///< Relative voxel stack position.
	TileSlope corner; ///< Corner of the voxel (#TC_NORTH, #TC_EAST, #TC_SOUTH or #TC_WEST).
};

/**
 * Description of neighbouring corners of a corner at a ground tile.
 * #left_neighbour and #right_neighbour are neighbours at the same tile, while
 * #neighbour_tiles list neighbouring corners at the three tiles around the
 * corner.
 */
struct CornerNeighbours {
	TileSlope left_neighbour;       ///< Left neighbouring corner.
	TileSlope right_neighbour;      ///< Right neighbouring corner.
	VoxelCorner neighbour_tiles[3]; ///< Neighbouring corners at other tiles.
};

/** Neighbouring corners of each corner. */
static const CornerNeighbours neighbours[4] = {
	{TC_EAST,  TC_WEST,  { {{-1, -1}, TC_SOUTH}, {{-1,  0}, TC_WEST }, {{ 0, -1}, TC_EAST }} }, // TC_NORTH
	{TC_NORTH, TC_SOUTH, { {{-1,  0}, TC_SOUTH}, {{-1,  1}, TC_WEST }, {{ 0,  1}, TC_NORTH}} }, // TC_EAST
	{TC_EAST,  TC_WEST,  { {{ 0,  1}, TC_WEST }, {{ 1,  1}, TC_NORTH}, {{ 1,  0}, TC_EAST }} }, //<TC_SOUTH
	{TC_SOUTH, TC_NORTH, { {{ 0, -1}, TC_SOUTH}, {{ 1, -1}, TC_EAST }, {{ 1,  0}, TC_NORTH}} }, // TC_WEST
};


/** Default constructor. */
VoxelStack::VoxelStack()
{
	this->voxels = NULL;
	this->base = 0;
	this->height = 0;
}

/** Destructor. */
VoxelStack::~VoxelStack()
{
	free(this->voxels);
}

/** Remove the stack. */
void VoxelStack::Clear()
{
	free(this->voxels);
	this->voxels = NULL;
	this->base = 0;
	this->height = 0;
}

/**
 * (Re)Allocate a voxel stack.
 * @param new_base New base voxel height.
 * @param new_height New number of voxels in the stack.
 * @return New stack could be created.
 * @note The old stack must fit in the new stack.
 * @todo If contents of a voxel is cleared, it might be released from the stack. Currently that is never done.
 */
bool VoxelStack::MakeVoxelStack(int16 new_base, uint16 new_height)
{
	assert(new_height > 0);
	/* Make sure the voxels live between 0 and MAX_VOXEL_STACK_SIZE. */
	if (new_base < 0 || new_base + (int)new_height > MAX_VOXEL_STACK_SIZE) return false;

	Voxel *new_voxels = Calloc<Voxel>(new_height);
	assert(this->height == 0 || (this->base >= new_base && this->base + this->height <= new_base + new_height));
	MemCpy<Voxel>(new_voxels + (this->base - new_base), this->voxels, this->height);

	free(this->voxels);
	this->voxels = new_voxels;
	this->height = new_height;
	this->base = new_base;
	return true;
}

/**
 * Get a voxel in the world by voxel coordinate.
 * @param z Z coordinate of the voxel.
 * @param create If the requested voxel does not exist, try to create it.
 * @return Address of the voxel (if it exists or could be created).
 */
Voxel *VoxelStack::Get(int16 z, bool create)
{
	if (z < 0 || z >= MAX_VOXEL_STACK_SIZE) return NULL;

	if (this->height == 0) {
		if (!create || !this->MakeVoxelStack(z, 1)) return NULL;
	} else if (z < this->base) {
		if (!create || !this->MakeVoxelStack(z, this->height + this->base - z)) return NULL;
	} else if ((uint)(z - this->base) >= this->height) {
		if (!create || !this->MakeVoxelStack(this->base, z - this->base + 1)) return NULL;
	}

	assert(z >= this->base);
	z -= this->base;
	assert((uint16)z < this->height);
	return &this->voxels[(uint16)z];
}


/** Default constructor of the voxel world. */
VoxelWorld::VoxelWorld()
{
	this->x_size = 64;
	this->y_size = 64;
}

/**
 * Create a new world. Everything gets cleared.
 * @param xs X size of the world.
 * @param ys Y size of the world.
 */
void VoxelWorld::SetWorldSize(uint16 xs, uint16 ys)
{
	assert(xs < WORLD_X_SIZE);
	assert(ys < WORLD_Y_SIZE);

	this->x_size = xs;
	this->y_size = ys;

	/* Clear the world. */
	for (uint pos = 0; pos < WORLD_X_SIZE * WORLD_Y_SIZE; pos++) {
		this->stacks[pos].Clear();
	}
}

/**
 * Creates a world of flat tiles.
 * @param z Height of the tiles.
 */
void VoxelWorld::MakeFlatWorld(int16 z)
{
	for (uint16 xpos = 0; xpos < this->x_size; xpos++) {
		for (uint16 ypos = 0; ypos < this->y_size; ypos++) {
			Voxel *v = GetVoxel(xpos, ypos, z, true);
			SurfaceVoxelData svd;
			svd.ground.type = GTP_GRASS0;
			svd.ground.slope = ImplodeTileSlope(SL_FLAT);
			svd.foundation.type = FDT_INVALID;
			v->SetSurface(svd);
		}
	}
}

/**
 * Get a voxel stack.
 * @param x X coordinate of the stack.
 * @param y Y coordinate of the stack.
 * @return The requested voxel stack.
 * @pre The coordinate must exist within the world.
 */
VoxelStack *VoxelWorld::GetStack(uint16 x, uint16 y)
{
	assert(x < WORLD_X_SIZE && x < this->x_size);
	assert(y < WORLD_Y_SIZE && y < this->y_size);

	return &this->stacks[x + y*WORLD_X_SIZE];
}

/**
 * Return height of the ground at the given voxel stack.
 * @param x Horizontal position.
 * @param y Vertical position.
 * @return Height of the ground.
 */
uint8 VoxelWorld::GetGroundHeight(uint16 x, uint16 y)
{
	VoxelStack *vs = this->GetStack(x, y);
	for (int16 i = vs->height - 1; i >= 0; i--) {
		const Voxel &v = vs->voxels[i];
		if (v.GetType() != VT_SURFACE) continue;
		const SurfaceVoxelData *svd = v.GetSurface();
		if (svd->ground.type < GTP_COUNT && svd->ground.type != GTP_INVALID) {
			assert(vs->base + i >= 0 && vs->base + i <= 255);
			return (uint8)(vs->base + i);
		}
	}
	NOT_REACHED();
}


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
		assert(v != NULL && v->GetType() == VT_SURFACE);
		const SurfaceVoxelData *svd = v->GetSurface();
		assert(svd->ground.type != GTP_INVALID);
		std::pair<Point32, GroundData> p(pos, GroundData(height, ExpandTileSlope(svd->ground.slope)));
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
	if (direction > 0 && old_height == MAX_VOXEL_STACK_SIZE) return false; // Cannot change above top.
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

		VoxelStack *vs = _world.GetStack(pos.x, pos.y);
		Voxel *v = vs->Get(gd.height, false);
		SurfaceVoxelData *pvd = v->GetSurface();
		uint8 gtype = pvd->ground.type;
		uint8 ftype = pvd->foundation.type;
		/* Clear existing ground and foundations. */
		v->SetEmpty();
		if ((gd.orig_slope & TCB_STEEP) != 0) vs->Get(gd.height + 1, false)->SetEmpty();

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
		v = vs->Get(min_h, true);
		SurfaceVoxelData svd;
		svd.ground.type = gtype;
		svd.foundation.type = ftype;
		svd.foundation.slope = 0; // XXX Needs further work.
		if (max_h - min_h <= 1) {
			/* Normal slope. */
			svd.ground.slope = ImplodeTileSlope(((new_heights[TC_NORTH] > min_h) ? TCB_NORTH : SL_FLAT)
					| ((new_heights[TC_EAST]  > min_h) ? TCB_EAST  : SL_FLAT)
					| ((new_heights[TC_SOUTH] > min_h) ? TCB_SOUTH : SL_FLAT)
					| ((new_heights[TC_WEST]  > min_h) ? TCB_WEST  : SL_FLAT));
			v->SetSurface(svd);
		} else {
			assert(max_h - min_h == 2);
			int corner;
			for (corner = 0; corner < 4; corner++) {
				if (new_heights[corner] == max_h) break;
			}
			assert(corner <= TC_WEST);
			svd.ground.slope = ImplodeTileSlope(TCB_STEEP | (TileSlope)(1 << corner));
			v->SetSurface(svd);
			ReferenceVoxelData rvd;
			rvd.xpos = pos.x;
			rvd.ypos = pos.y;
			rvd.zpos = min_h;
			vs->Get(min_h + 1, true)->SetReference(rvd);
		}
	}
}

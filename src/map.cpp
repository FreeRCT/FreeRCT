/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map.cpp Voxels of the world. */

/**
 * @defgroup map_group World map data and code
 */

#include "stdafx.h"
#include "map.h"
#include "memory.h"
#include "viewport.h"
#include "math_func.h"
#include "sprite_store.h"

/**
 * The game world.
 * @ingroup map_group
 */
VoxelWorld _world;

/**
 * Copy a voxel.
 * @param dest Destination address.
 * @param src Source address.
 * @param copyPersons Copy the person list too.
 */
static inline void CopyVoxel(Voxel *dest, Voxel *src, bool copyPersons)
{
	dest->w0 = src->w0;
	dest->w1 = src->w1;
	if (copyPersons) CopyPersonList(dest->persons, src->persons);
}

/**
 * Copy a stack of voxels.
 * @param dest Destination address.
 * @param src Source address.
 * @param count Number of voxels to copy.
 * @param copyPersons Copy the person list too.
 */
static void CopyStackData(Voxel *dest, Voxel *src, int count, bool copyPersons)
{
	while (count > 0) {
		CopyVoxel(dest, src, copyPersons);
		dest++;
		src++;
		count--;
	}
}

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
	this->owner = OWN_NONE;
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
	/* Make sure the voxels live between 0 and WORLD_Z_SIZE. */
	if (new_base < 0 || new_base + (int)new_height > WORLD_Z_SIZE) return false;

	Voxel *new_voxels = Calloc<Voxel>(new_height);
	assert(this->height == 0 || (this->base >= new_base && this->base + this->height <= new_base + new_height));
	CopyStackData(new_voxels + (this->base - new_base), this->voxels, this->height, true);

	free(this->voxels);
	this->voxels = new_voxels;
	this->height = new_height;
	this->base = new_base;
	return true;
}

/**
 * Make a copy of self.
 * @param copyPersons Copy the person list too.
 * @return The copied structure.
 */
VoxelStack *VoxelStack::Copy(bool copyPersons) const
{
	VoxelStack *vs = new VoxelStack;
	if (this->height > 0) {
		vs->MakeVoxelStack(this->base, this->height);
		CopyStackData(vs->voxels, this->voxels, this->height, copyPersons);
	}
	return vs;
}

/**
 * Get a voxel in the world by voxel coordinate.
 * @param z Z coordinate of the voxel.
 * @return Address of the voxel (if it exists or could be created).
 */
const Voxel *VoxelStack::Get(int16 z) const
{
	if (z < 0 || z >= WORLD_Z_SIZE) return NULL;

	if (this->height == 0 || z < this->base || (uint)(z - this->base) >= this->height) return NULL;

	assert(z >= this->base);
	z -= this->base;
	assert((uint16)z < this->height);
	return &this->voxels[(uint16)z];
}

/**
 * Get a voxel in the world by voxel coordinate. Create one if needed.
 * @param z Z coordinate of the voxel.
 * @param create If the requested voxel does not exist, try to create it.
 * @return Address of the voxel (if it exists or could be created).
 */
Voxel *VoxelStack::GetCreate(int16 z, bool create)
{
	if (z < 0 || z >= WORLD_Z_SIZE) return NULL;

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
 * Add foundation bits from the bottom up to the given voxel.
 * @param world %Voxel storage.
 * @param xpos X position of the voxel stack.
 * @param ypos Y position of the voxel stack.
 * @param z Height of the ground.
 * @param bits Foundation bits to add.
 */
static void AddFoundations(VoxelWorld *world, uint16 xpos, uint16 ypos, int16 z, uint8 bits)
{
	for (int16 zpos = 0; zpos < z; zpos++) {
		Voxel *v = world->GetCreateVoxel(xpos, ypos, zpos, true);
		if (v->GetType() != VT_SURFACE) {
			v->SetFoundationType(FDT_GROUND);
			v->SetGroundType(GTP_INVALID);
			v->SetPathRideNumber(PT_INVALID);
		}
		v->SetFoundationSlope(v->GetFoundationSlope() | bits);
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
			Voxel *v = this->GetCreateVoxel(xpos, ypos, z, true);
			v->SetFoundationType(FDT_INVALID);
			v->SetGroundType(GTP_GRASS0);
			v->SetGroundSlope(ImplodeTileSlope(SL_FLAT));
			v->SetPathRideNumber(PT_INVALID);
		}
	}
	for (uint16 xpos = 0; xpos < this->x_size; xpos++) {
		AddFoundations(this, xpos, 0, z, 0xC0);
		AddFoundations(this, xpos, this->y_size - 1, z, 0x0C);
	}
	for (uint16 ypos = 0; ypos < this->y_size; ypos++) {
		AddFoundations(this, 0, ypos, z, 0x03);
		AddFoundations(this, this->x_size - 1, ypos, z, 0x30);
	}
}

/**
 * Move a voxel stack to this world. May destroy the original stack in the process.
 * @param vs Source stack.
 */
void VoxelStack::MoveStack(VoxelStack *vs)
{
	/* Clean up the stack a bit before copying it, and get lowest and highest non-empty voxel. */
	int vs_first = 0;
	int vs_last = 0;
	for (int i = 0; i < (int)vs->height; i++) {
		Voxel *v = &vs->voxels[i];
		assert(v->persons.IsEmpty()); // There should be no persons in the stack being moved.
		switch (v->GetType()) {
			case VT_SURFACE: {
				if (!v->IsEmpty()) {
					vs_last = i;
					break;
				}
				v->SetEmpty();
				/* FALL-THROUGH */
			}

			case VT_EMPTY:
				if (vs_first == i) vs_first++;
				break;

			case VT_REFERENCE:
				vs_last = i;
				break;

			default: NOT_REACHED();
		}
	}

	/* There should be at least one surface voxel. */
	assert(vs_first <= vs_last);

	/* Examine current stack with respect to persons. */
	int old_first = 0;
	int old_last = 0;
	for (int i = 0; i < (int)this->height; i++) {
		const Voxel *v = &this->voxels[i];
		if (v->persons.IsEmpty()) {
			if (old_first == i) old_first++;
		} else {
			old_last = i;
		}
	}

	int new_base = min(vs->base + vs_first, this->base + old_first);
	int new_height = max(vs->base + vs_last, this->base + old_last) - new_base + 1;
	assert(new_base >= 0 && new_height > 0);

	/* Make a new stack. Copy new surface, then copy the persons. */
	Voxel *new_voxels = Calloc<Voxel>(new_height);
	CopyStackData(new_voxels + (vs->base + vs_first) - new_base, vs->voxels + vs_first, vs_last - vs_first + 1, false);
	int i = (this->base + old_first) - new_base;
	while (old_first <= old_last) {
		CopyPersonList(new_voxels[i].persons, this->voxels[old_first].persons);
		i++;
		old_first++;
	}

	this->base = new_base;
	this->height = new_height;
	free(this->voxels);
	this->voxels = new_voxels;
}

/**
 * Get a voxel stack.
 * @param x X coordinate of the stack.
 * @param y Y coordinate of the stack.
 * @return The requested voxel stack.
 * @pre The coordinate must exist within the world.
 */
VoxelStack *VoxelWorld::GetModifyStack(uint16 x, uint16 y)
{
	assert(x < WORLD_X_SIZE && x < this->x_size);
	assert(y < WORLD_Y_SIZE && y < this->y_size);

	return &this->stacks[x + y*WORLD_X_SIZE];
}

/**
 * Get a voxel stack (for read-only access).
 * @param x X coordinate of the stack.
 * @param y Y coordinate of the stack.
 * @return The requested voxel stack.
 * @pre The coordinate must exist within the world.
 */
const VoxelStack *VoxelWorld::GetStack(uint16 x, uint16 y) const
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
uint8 VoxelWorld::GetGroundHeight(uint16 x, uint16 y) const
{
	const VoxelStack *vs = this->GetStack(x, y);
	for (int16 i = vs->height - 1; i >= 0; i--) {
		const Voxel &v = vs->voxels[i];
		if (v.GetType() != VT_SURFACE) continue;
		if (v.GetGroundType() != GTP_INVALID) {
			assert(vs->base + i >= 0 && vs->base + i <= 255);
			return (uint8)(vs->base + i);
		}
	}
	NOT_REACHED();
}

/**
 * Get the ownership of a tile
 * @param x X coordinate of the tile.
 * @param y Y coordinate of the tile.
 * @return Ownership of the tile.
 */
TileOwner VoxelWorld::GetTileOwner(uint16 x,  uint16 y)
{
	return this->GetStack(x, y)->owner;
}

/**
 * Set the ownership of a tile
 * @param x X coordinate of the tile.
 * @param y Y coordinate of the tile.
 * @param owner Ownership of the tile.
 */
void VoxelWorld::SetTileOwner(uint16 x,  uint16 y, TileOwner owner)
{
	this->GetModifyStack(x, y)->owner = owner;
}

/**
 * Set tile ownership for a rectangular area.
 * @param x Base X coordinate of the rectangle.
 * @param y Base Y coordinate of the rectangle.
 * @param width Length in X direction of the rectangle.
 * @param height Length in Y direction of the rectangle.
 * @param owner New value for ownership of all tiles.
 */
void VoxelWorld::SetTileOwnerRect(uint16 x, uint16 y, uint16 width, uint16 height, TileOwner owner)
{
	for (uint16 ix = x; ix < x + width; ix++) {
		for (uint16 iy = y; iy < y + height; iy++) {
			this->SetTileOwner(ix, iy, owner);
		}
	}
}

WorldAdditions::WorldAdditions()
{
}

WorldAdditions::~WorldAdditions()
{
	this->Clear();
}

/** Remove all modifications. */
void WorldAdditions::Clear()
{
	VoxelStackMap::iterator iter;
	for (iter = this->modified_stacks.begin(); iter != this->modified_stacks.end(); iter++) {
		delete (*iter).second;
	}
	this->modified_stacks.clear();
}

/** Move modifications to the 'real' #_world. */
void WorldAdditions::Commit()
{
	VoxelStackMap::iterator iter;
	for (iter = this->modified_stacks.begin(); iter != this->modified_stacks.end(); iter++) {
		Point32 pt = (*iter).first;
		_world.MoveStack(pt.x, pt.y, (*iter).second);
	}
	this->Clear();
}

/**
 * Get a voxel stack with the purpose of modifying it. If necessary, a copy from the #_world stack is made.
 * @param x X coordinate of the stack.
 * @param y Y coordinate of the stack.
 * @return The requested voxel stack.
 */
VoxelStack *WorldAdditions::GetModifyStack(uint16 x, uint16 y)
{
	Point32 pt;
	pt.x = x;
	pt.y = y;

	VoxelStackMap::iterator iter = this->modified_stacks.find(pt);
	if (iter != this->modified_stacks.end()) return (*iter).second;
	std::pair<Point32, VoxelStack *> p(pt, _world.GetStack(x, y)->Copy(false));
	iter = this->modified_stacks.insert(p).first;
	return (*iter).second;
}

/**
 * Get a voxel stack (read-only) either from the modifications, or from the 'real' #_world.
 * @param x X coordinate of the stack.
 * @param y Y coordinate of the stack.
 * @return The requested voxel stack.
 */
const VoxelStack *WorldAdditions::GetStack(uint16 x, uint16 y) const
{
	Point32 pt;
	pt.x = x;
	pt.y = y;

	VoxelStackMap::const_iterator iter = this->modified_stacks.find(pt);
	if (iter != this->modified_stacks.end()) return (*iter).second;
	return _world.GetStack(x, y);
}

/**
 * Mark the additions in the display as outdated, so they get repainted.
 * @param vp %Viewport displaying the world.
 */
void WorldAdditions::MarkDirty(Viewport *vp)
{
	VoxelStackMap::const_iterator iter;
	for (iter = this->modified_stacks.begin(); iter != this->modified_stacks.end(); iter++) {
		const Point32 pt = (*iter).first;
		const VoxelStack *vstack = (*iter).second;
		if (vstack != NULL) vp->MarkVoxelDirty(pt.x, pt.x, vstack->base, vstack->height);
	}
}

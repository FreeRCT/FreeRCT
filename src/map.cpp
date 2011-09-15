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

static const uint MAX_VOXEL_STACK_SIZE = 64; ///< At most a stack of 64 voxels.

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
	if (new_height > MAX_VOXEL_STACK_SIZE) return false;

	Voxel *new_voxels = Calloc<Voxel>(new_height);
	assert(this->height == 0 || (this->base >= new_base && this->base + this->height <= new_height));
	MemCpy<Voxel>(new_voxels + (new_base - this->base), this->voxels, this->height);

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
	if (this->height == 0) {
		if (!create || !this->MakeVoxelStack(z, 1)) return NULL;
	} else if (z < this->base) {
		if (!create || !this->MakeVoxelStack(z, this->height + base - z)) return NULL;
	} else if ((uint)(z - this->base) >= this->height) {
		if (!create || !this->MakeVoxelStack(this->base, z + 1)) return NULL;
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
			v->SetSurface(SL_FLAT);
		}
	}
}

/**
 * Make a nice bump in the world.
 * @param x xpos
 * @param y ypos
 * @param z zpos
 * @todo Temporary, remove me after implementing world manipulation code.
 */
void VoxelWorld::MakeBump(uint16 x, uint16 y, int16 z)
{
	Voxel *v;
	v = GetVoxel(x-1, y-1, z,   true); v->SetSurface(TCB_SOUTH);
	v = GetVoxel(x-1, y,   z,   true); v->SetSurface((Slope)(TCB_SOUTH | TCB_WEST));
	v = GetVoxel(x-1, y+1, z,   true); v->SetSurface(TCB_WEST);
	v = GetVoxel(x,   y-1, z,   true); v->SetSurface((Slope)(TCB_SOUTH | TCB_EAST));
	v = GetVoxel(x,   y,   z+1, true); v->SetSurface(SL_FLAT);
	v = GetVoxel(x,   y+1, z,   true); v->SetSurface((Slope)(TCB_NORTH | TCB_WEST));
	v = GetVoxel(x+1, y-1, z,   true); v->SetSurface(TCB_EAST);
	v = GetVoxel(x+1, y,   z,   true); v->SetSurface((Slope)(TCB_NORTH | TCB_EAST));
	v = GetVoxel(x+1, y+1, z,   true); v->SetSurface(TCB_NORTH);

	v = GetVoxel(x, y, z, true); v->SetEmpty();
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


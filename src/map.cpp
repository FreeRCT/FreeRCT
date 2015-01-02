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
 * Move the list animated voxel objects to the destination.
 * @param dest Destination of the voxel object list.
 * @param src Source voxel objects.
 */
static inline void CopyVoxelObjectList(Voxel *dest, Voxel *src)
{
	dest->voxel_objects = src->voxel_objects;
	src->voxel_objects = nullptr;
}

/**
 * Copy a voxel.
 * @param dest Destination address.
 * @param src Source address.
 * @param copy_voxel_objects Copy/move the voxel objects too.
 */
static inline void CopyVoxel(Voxel *dest, Voxel *src, bool copy_voxel_objects)
{
	dest->instance = src->instance;
	dest->instance_data = src->instance_data;
	dest->ground = src->ground;
	dest->fence = src->fence;
	if (copy_voxel_objects) CopyVoxelObjectList(dest, src);
}

/**
 * Copy a stack of voxels.
 * @param dest Destination address.
 * @param src Source address.
 * @param count Number of voxels to copy.
 * @param copy_voxel_objects Copy the voxel object list too.
 */
static void CopyStackData(Voxel *dest, Voxel *src, int count, bool copy_voxel_objects)
{
	while (count > 0) {
		CopyVoxel(dest, src, copy_voxel_objects);
		dest++;
		src++;
		count--;
	}
}

/**
 * Load a voxel from the save game.
 * @param ldr Input stream to read.
 * @param version Version to load.
 */
void Voxel::Load(Loader &ldr, uint32 version)
{
	this->ClearVoxel();
	if (version == 1) {
		this->ground = ldr.GetLong(); /// \todo Check sanity of the data.
		this->instance = ldr.GetByte();
		if (this->instance == SRI_FREE) {
			this->instance_data = 0; // Full rides load after the world, overwriting map data.
		} else if (this->instance >= SRI_RIDES_START && this->instance < SRI_FULL_RIDES) {
			this->instance_data = ldr.GetWord();
		} else {
			this->instance_data = 0; // Full rides load after the world, overwriting map data.
			ldr.SetFailMessage("Unknown voxel instance data");
		}
	} else {
		ldr.SetFailMessage("Unknown voxel version");
	}
}

/**
 * Write a voxel to the save game.
 * @param svr Output stream to write.
 */
void Voxel::Save(Saver &svr) const
{
	svr.PutLong(this->ground);
	if (this->instance >= SRI_RIDES_START && this->instance < SRI_FULL_RIDES) {
		svr.PutByte(this->instance);
		svr.PutWord(this->instance_data);
	} else {
		svr.PutByte(SRI_FREE); // Full rides save their own data from the world.
	}
}

/**
 * Make a new array of voxels, and initialize it.
 * @param height Desired height of the new voxel array.
 * @return New and initialized to 'empty' voxels. Caller should delete[] the memory after use.
 */
static Voxel *MakeNewVoxels(int height)
{
	assert(height > 0);
	Voxel *voxels = new Voxel[height]();
	for (int i = 0; i < height; i++) voxels[i].ClearVoxel();
	return voxels;
}

VoxelObject::~VoxelObject()
{
	if (this->added) {
		this->MarkDirty();
		Voxel *v = _world.GetCreateVoxel(this->x_vox, this->y_vox, this->z_vox, false);
		this->RemoveSelf(v);
	}
}

/**
 * \fn const ImageData *VoxelObject::GetSprite(const SpriteStorage *sprites, ViewOrientation orient, const Recolouring **recolour) const
 * Get the sprite to draw for the voxel object.
 * @param sprites Sprites at the right size for drawing.
 * @param orient Direction of view.
 * @param recolour [out] Recolour mapping if present, else \c nullptr.
 * @return Sprite to display for the voxel object.
 */

/** Mark the voxel containing the voxel object as dirty, so it is repainted. */
void VoxelObject::MarkDirty()
{
	MarkVoxelDirty(XYZPoint16(this->x_vox, this->y_vox, this->z_vox));
}

/** Default constructor. */
VoxelStack::VoxelStack()
{
	this->voxels = nullptr;
	this->base = 0;
	this->height = 0;
}

/** Destructor. */
VoxelStack::~VoxelStack()
{
	delete[] this->voxels;
}

/** Remove the stack. */
void VoxelStack::Clear()
{
	delete[] this->voxels;
	this->voxels = nullptr;
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
	/* Make sure the voxels live between 0 and WORLD_Z_SIZE. */
	if (new_base < 0 || new_base + (int)new_height > WORLD_Z_SIZE) return false;

	Voxel *new_voxels = MakeNewVoxels(new_height);
	assert(this->height == 0 || (this->base >= new_base && this->base + this->height <= new_base + new_height));
	CopyStackData(new_voxels + (this->base - new_base), this->voxels, this->height, true);

	delete[] this->voxels;
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
	if (z < 0 || z >= WORLD_Z_SIZE) return nullptr;

	if (this->height == 0 || z < this->base || (uint)(z - this->base) >= this->height) return nullptr;

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
	if (z < 0 || z >= WORLD_Z_SIZE) return nullptr;

	if (this->height == 0) {
		if (!create || !this->MakeVoxelStack(z, 1)) return nullptr;
	} else if (z < this->base) {
		if (!create || !this->MakeVoxelStack(z, this->height + this->base - z)) return nullptr;
	} else if ((uint)(z - this->base) >= this->height) {
		if (!create || !this->MakeVoxelStack(this->base, z - this->base + 1)) return nullptr;
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
		if (v->GetFoundationType() == FDT_INVALID) {
			v->SetFoundationType(FDT_GROUND);
			v->SetFoundationSlope(bits);
			continue;
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
			v->ClearInstances();
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
		assert(!v->HasVoxelObjects()); // There should be no voxel objects in the stack being moved.

		if (!v->IsEmpty()) {
			vs_last = i;
		} else {
			if (vs_first == i) vs_first++;
		}
	}

	/* There should be at least one surface voxel. */
	assert(vs_first <= vs_last);

	/* Examine current stack with respect to persons. */
	int old_first = 0;
	int old_last = 0;
	for (int i = 0; i < (int)this->height; i++) {
		const Voxel *v = &this->voxels[i];
		if (v->HasVoxelObjects()) {
			old_last = i;
		} else {
			if (old_first == i) old_first++;
		}
	}

	int new_base = std::min(vs->base + vs_first, this->base + old_first);
	int new_height = std::max(vs->base + vs_last, this->base + old_last) - new_base + 1;
	assert(new_base >= 0);

	/* Make a new stack. Copy new surface, then copy the persons. */
	Voxel *new_voxels = MakeNewVoxels(new_height);
	CopyStackData(new_voxels + (vs->base + vs_first) - new_base, vs->voxels + vs_first, vs_last - vs_first + 1, false);
	int i = (this->base + old_first) - new_base;
	while (old_first <= old_last) {
		CopyVoxelObjectList(&new_voxels[i], &this->voxels[old_first]);
		i++;
		old_first++;
	}

	this->base = new_base;
	this->height = new_height;
	delete[] this->voxels;
	this->voxels = new_voxels;
}

/**
 * Get the offset of ground in the voxel stack.
 * @return Index in the voxel array for the voxel containing the ground.
 */
int VoxelStack::GetGroundOffset() const
{
	for (int i = this->height - 1; i >= 0; i--) {
		const Voxel &v = this->voxels[i];
		if (v.GetGroundType() != GTP_INVALID && !IsImplodedSteepSlopeTop(v.GetGroundSlope())) return i;
	}
	NOT_REACHED();
}

/**
 * Load a voxel stack from the save game file.
 * @param ldr Input stream to read.
 */
void VoxelStack::Load(Loader &ldr)
{
	this->Clear();
	uint32 version = ldr.OpenBlock("VSTK");
	if (version == 1) {
		int16 base = ldr.GetWord();
		uint16 height = ldr.GetWord();
		uint8 owner = ldr.GetByte();
		if (base < 0 || base + height > WORLD_Z_SIZE || owner >= OWN_COUNT) {
			ldr.SetFailMessage("Incorrect voxel stack size");
		} else {
			this->base = base;
			this->height = height;
			this->owner = (TileOwner)owner;
			delete[] this->voxels;
			this->voxels = (height > 0) ? MakeNewVoxels(height) : nullptr;
			for (uint i = 0; i < height; i++) this->voxels[i].Load(ldr, version);
		}
	}
	ldr.CloseBlock();
}

/**
 * Save a voxel stack to the save game file.
 * @param svr Output stream to write.
 */
void VoxelStack::Save(Saver &svr) const
{
	svr.StartBlock("VSTK", 1);
	svr.PutWord(this->base);
	svr.PutWord(this->height);
	svr.PutByte(this->owner);
	for (uint i = 0; i < this->height; i++) this->voxels[i].Save(svr);
	svr.EndBlock();
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

	return &this->stacks[x + y * WORLD_X_SIZE];
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

	return &this->stacks[x + y * WORLD_X_SIZE];
}

/**
 * Return the base height of the ground at the given voxel stack.
 * @param x Horizontal position.
 * @param y Vertical position.
 * @return Height of the ground (for steep slopes, the base voxel height).
 */
uint8 VoxelWorld::GetGroundHeight(uint16 x, uint16 y) const
{
	const VoxelStack *vs = this->GetStack(x, y);
	int offset = vs->GetGroundOffset();
	assert(vs->base + offset >= 0 && vs->base + offset <= 255);
	return vs->base + offset;
}

/**
 * Get the ownership of a tile
 * @param x X coordinate of the tile.
 * @param y Y coordinate of the tile.
 * @return Ownership of the tile.
 */
TileOwner VoxelWorld::GetTileOwner(uint16 x, uint16 y)
{
	return this->GetStack(x, y)->owner;
}

/**
 * Add/remove land border fence based on current land ownership for the given
 * tile rectangle.
 * @param x Base X coordinate of the rectangle.
 * @param y Base Y coordinate of the rectangle.
 * @param width Length in X direction of the rectangle.
 * @param height Length in Y direction of the rectangle.
 */
void UpdateLandBorderFence(uint16 x, uint16 y, uint16 width, uint16 height)
{
	/*
	 * Iterate over given rectangle plus one tile, unless the map border
	 * is reached.
	 */
	uint16 x_min = x > 0 ? x-1 : x;
	uint16 y_min = y > 0 ? y-1 : y;
	uint16 x_max = std::min(_world.GetXSize()-1, x + width + 1);
	uint16 y_max = std::min(_world.GetYSize()-1, y + height + 1);

	for (uint16 ix = x_min; ix < x_max; ix++) {
		for (uint16 iy = y_min; iy < y_max; iy++) {
			VoxelStack *vs = _world.GetModifyStack(ix, iy);
			TileOwner owner = vs->owner;

			for (int16 i = vs->height - 1; i >= 0; i--) {
				Voxel &v = vs->voxels[i];

				/* Only update border fence on the surface. */
				if (v.GetGroundType() == GTP_INVALID) continue;
				TileSlope slope =  ExpandTileSlope(v.GetGroundSlope());
				if ((slope & TSB_TOP) != 0) continue; // Handle steep slopes at the bottom voxel

				for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
					bool fence;
					if (owner == OWN_PARK) {
						fence = false;
					} else {
						if ((ix == 0 && _tile_dxy[edge].x == -1) || (iy == 0 && _tile_dxy[edge].y == -1)) continue;
						uint16 neighbour_x = ix + _tile_dxy[edge].x;
						uint16 neighbour_y = iy + _tile_dxy[edge].y;
						if (neighbour_x >= _world.GetXSize() || neighbour_y >= _world.GetYSize()) continue;
						TileOwner neighbour_owner = _world.GetTileOwner(neighbour_x, neighbour_y);
						fence = neighbour_owner == OWN_PARK;
					}

					/* Only set/unset land border fence if the edge doesn't have some other fence type. */
					Voxel *fence_voxel = StoreFenceInUpperVoxel(slope, edge) ? vs->GetCreate(i + 1, true) : &v;
					FenceType curr_type = fence_voxel != nullptr ? fence_voxel->GetFenceType(edge) : FENCE_TYPE_INVALID;
					if (curr_type != (fence ? FENCE_TYPE_LAND_BORDER : FENCE_TYPE_INVALID)) {
						fence_voxel->SetFenceType(edge, fence ? FENCE_TYPE_LAND_BORDER : FENCE_TYPE_INVALID);
					}
				}

				/* There are no fences below ground. */
				break;
			}
		}
	}
}

/**
 * Set the ownership of a tile
 * @param x X coordinate of the tile.
 * @param y Y coordinate of the tile.
 * @param owner Ownership of the tile.
 */
void VoxelWorld::SetTileOwner(uint16 x, uint16 y, TileOwner owner)
{
	this->GetModifyStack(x, y)->owner = owner;

	UpdateLandBorderFence(x, y, 1, 1);
}

/**
 * Set tile ownership for a rectangular area.
 * @param x Base X coordinate of the rectangle.
 * @param y Base Y coordinate of the rectangle.
 * @param width Length in X direction of the rectangle.
 * @param height Length in Y direction of the rectangle.
 * @param owner New owner of all tiles in the rectangle.
 */
void VoxelWorld::SetTileOwnerRect(uint16 x, uint16 y, uint16 width, uint16 height, TileOwner owner)
{
	for (uint16 ix = x; ix < x + width; ix++) {
		for (uint16 iy = y; iy < y + height; iy++) {
			this->GetModifyStack(ix, iy)->owner = owner;
		}
	}

	UpdateLandBorderFence(x, y, width, height);
}

/**
 * Set an owner for all tiles in the world.
 * @param owner New owner of all tiles.
 */
void VoxelWorld::SetTileOwnerGlobally(TileOwner owner)
{
	SetTileOwnerRect(0, 0, this->GetXSize(), this->GetYSize(), owner);
}

/**
 * Load the world from a file.
 * @param ldr Input stream to read from.
 */
void VoxelWorld::Load(Loader &ldr)
{
	uint32 version = ldr.OpenBlock("WRLD");
	uint16 xsize = 64;
	uint16 ysize = 64;
	if (version == 1) {
		xsize = ldr.GetWord();
		ysize = ldr.GetWord();
	} else if (version != 0) {
		ldr.SetFailMessage("Unknown world version.");
	}
	if (xsize >= WORLD_X_SIZE || ysize >= WORLD_Y_SIZE) {
		xsize = std::min<uint16>(xsize, WORLD_X_SIZE);
		ysize = std::min<uint16>(ysize, WORLD_Y_SIZE);
		ldr.SetFailMessage("Incorrect world size");
	}
	ldr.CloseBlock();

	this->SetWorldSize(xsize, ysize);
	if (!ldr.IsFail() && version != 0) {
		for (uint16 x = 0; x < xsize; x++) {
			for (uint16 y = 0; y < ysize; y++) {
				VoxelStack *vs = this->GetModifyStack(x, y);
				vs->Load(ldr);
			}
		}
	}
	if (version == 0 || ldr.IsFail()) this->MakeFlatWorld(8);
}

/**
 * Save the world to a file.
 * @param svr Output stream to save to.
 */
void VoxelWorld::Save(Saver &svr) const
{
	/* Save basic map information (rides are saved as part of the ride). */
	svr.StartBlock("WRLD", 1);
	svr.PutWord(this->GetXSize());
	svr.PutWord(this->GetYSize());
	svr.EndBlock();
	for (uint16 x = 0; x < this->GetXSize(); x++) {
		for (uint16 y = 0; y < this->GetYSize(); y++) {
			const VoxelStack *vs = this->GetStack(x, y);
			vs->Save(svr);
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
	for (auto &iter : this->modified_stacks) {
		delete iter.second;
	}
	this->modified_stacks.clear();
}

/** Move modifications to the 'real' #_world. */
void WorldAdditions::Commit()
{
	for (auto &iter : this->modified_stacks) {
		Point32 pt = iter.first;
		_world.MoveStack(pt.x, pt.y, iter.second);
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
	Point32 pt(x, y);

	auto iter = this->modified_stacks.find(pt);
	if (iter != this->modified_stacks.end()) return iter->second;
	std::pair<Point32, VoxelStack *> p(pt, _world.GetStack(x, y)->Copy(false));
	iter = this->modified_stacks.insert(p).first;
	return iter->second;
}

/**
 * Get a voxel stack (read-only) either from the modifications, or from the 'real' #_world.
 * @param x X coordinate of the stack.
 * @param y Y coordinate of the stack.
 * @return The requested voxel stack.
 */
const VoxelStack *WorldAdditions::GetStack(uint16 x, uint16 y) const
{
	Point32 pt(x, y);

	const auto iter = this->modified_stacks.find(pt);
	if (iter != this->modified_stacks.end()) return iter->second;
	return _world.GetStack(x, y);
}

/**
 * Mark the additions in the display as outdated, so they get repainted.
 * @param vp %Viewport displaying the world.
 */
void WorldAdditions::MarkDirty(Viewport *vp)
{
	for (const auto &iter : this->modified_stacks) {
		const Point32 pt = iter.first;
		const VoxelStack *vstack = iter.second;
		if (vstack != nullptr) vp->MarkVoxelDirty(XYZPoint16(pt.x, pt.x, vstack->base), vstack->height);
	}
}

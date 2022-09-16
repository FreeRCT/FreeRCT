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

/** Make the voxel empty. */
void Voxel::ClearVoxel()
{
	this->SetGroundType(GTP_INVALID);
	this->SetFoundationType(FDT_INVALID);
	this->SetGroundSlope(ISL_FLAT);
	this->SetGrowth(0);
	this->SetFences(ALL_INVALID_FENCES);
	this->ClearInstances();
	this->voxel_objects = nullptr;
}

static const uint32 CURRENT_VERSION_VSTK  = 3;   ///< Currently supported version of the VSTK pattern.
static const uint32 CURRENT_VERSION_Voxel = 3;   ///< Currently supported version of the voxel pattern.

/**
 * Load a voxel from the save game.
 * @param ldr Input stream to read.
 */
void Voxel::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("voxl");
	this->ClearVoxel();
	if (version >= 1 && version <= 3) {
		this->ground = ldr.GetLong(); /// \todo Check sanity of the data.
		this->instance = ldr.GetByte();
		if (this->instance == SRI_FREE) {
			this->instance_data = 0; // Full rides load after the world, overwriting map data.
		} else if (this->instance >= SRI_RIDES_START && this->instance < SRI_FULL_RIDES) {
			this->instance_data = ldr.GetWord();
		} else {
			throw LoadingError("Unknown voxel instance data");
		}

		if (version >= 2) this->fences = ldr.GetWord();
	} else {
		ldr.VersionMismatch(version, CURRENT_VERSION_Voxel);
	}
	ldr.ClosePattern();
}

/**
 * Write a voxel to the save game.
 * @param svr Output stream to write.
 */
void Voxel::Save(Saver &svr) const
{
	svr.StartPattern("voxl", CURRENT_VERSION_Voxel);
	svr.PutLong(this->ground);
	if (this->instance >= SRI_RIDES_START && this->instance < SRI_FULL_RIDES) {
		svr.PutByte(this->instance);
		svr.PutWord(this->instance_data);
	} else {
		svr.PutByte(SRI_FREE); // Full rides save their own data from the world.
	}
	svr.PutWord(this->fences);
	svr.EndPattern();
}

VoxelObject::~VoxelObject()
{
	if (this->added) {
		Voxel *v = _world.GetCreateVoxel(this->vox_pos, false);
		this->RemoveSelf(v);
	}
}

/**
 * \fn const ImageData *VoxelObject::GetSprite(ViewOrientation orient, int zoom, const Recolouring **recolour) const
 * Get the sprite to draw for the voxel object.
 * @param sprites Sprites at the right size for drawing.
 * @param orient Direction of view.
 * @param zoom Zoom scale.
 * @param recolour [out] Recolour mapping if present, else \c nullptr.
 * @return Sprite to display for the voxel object.
 */

/**
 * Get the overlay sprite(s) to draw for the voxel object.
 * @param sprites Sprites at the right size for drawing.
 * @param orient Direction of view.
 * @param zoom Zoom scale.
 * @return Sprites to overlay for the voxel object, with their respective recolourings.
 */
VoxelObject::Overlays VoxelObject::GetOverlays([[maybe_unused]] ViewOrientation orient, [[maybe_unused]] int zoom) const
{
	return {};
}

static const uint32 CURRENT_VERSION_VoxelObject = 1;   ///< Currently supported version of %VoxelObject.

/**
 * Load a voxel object from the save game.
 * @param ldr Input stream to read.
 */
void VoxelObject::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("vxoj");
	if (version != CURRENT_VERSION_VoxelObject) ldr.VersionMismatch(version, CURRENT_VERSION_VoxelObject);

	uint32 x = ldr.GetLong();
	uint32 y = ldr.GetLong();
	uint32 z = ldr.GetLong();

	XYZPoint32 xyz(x, y, z);

	this->vox_pos = this->GetVoxelCoordinate(xyz);
	this->pix_pos = this->GetInVoxelCoordinate(xyz);
	ldr.ClosePattern();
}

/**
 * Save a voxel object to the save game file.
 * @param svr Output stream to write.
 */
void VoxelObject::Save(Saver &svr)
{
	svr.StartPattern("vxoj", CURRENT_VERSION_VoxelObject);
	XYZPoint32 xyz = this->MergeCoordinates();

	svr.PutLong(xyz.x);
	svr.PutLong(xyz.y);
	svr.PutLong(xyz.z);
	svr.EndPattern();
}

/** Default constructor. */
VoxelStack::VoxelStack() : base(0), height(0)
{
}

/** Remove the stack. */
void VoxelStack::Clear()
{
	this->voxels.clear();
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
 */
bool VoxelStack::MakeVoxelStack(int16 new_base, uint16 new_height)
{
	/* Make sure the voxels live between 0 and WORLD_Z_SIZE. */
	if (new_base < 0 || new_base + (int)new_height > WORLD_Z_SIZE) return false;

	assert(this->height == 0 || (this->base >= new_base && this->base + this->height <= new_base + new_height));

	for (int i = new_base; i < this->base; i++) this->voxels.insert(this->voxels.begin(), std::unique_ptr<Voxel>(new Voxel));
	for (size_t i = new_height - this->voxels.size(); i > 0; i--) this->voxels.push_back(std::unique_ptr<Voxel>(new Voxel));
	this->height = new_height;
	this->base = new_base;
	return true;
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
	return this->voxels[z].get();
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
	return this->voxels[z].get();
}

/** Default constructor of the voxel world. */
VoxelWorld::VoxelWorld() : x_size(64), y_size(64)
{
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
		Voxel *v = world->GetCreateVoxel(XYZPoint16(xpos, ypos, zpos), true);
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
	this->edges_without_border_fence.clear();
	for (uint16 xpos = 0; xpos < this->x_size; xpos++) {
		for (uint16 ypos = 0; ypos < this->y_size; ypos++) {
			Voxel *v = this->GetCreateVoxel(XYZPoint16(xpos, ypos, z), true);
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
 * Get the offset of the base of ground in the voxel stack (for steep slopes the bottom voxel).
 * @return Index in the voxel array for the base voxel containing the ground.
 */
int VoxelStack::GetBaseGroundOffset() const
{
	for (int i = this->height - 1; i >= 0; i--) {
		const Voxel &v = *this->voxels[i];
		if (v.GetGroundType() != GTP_INVALID && !IsImplodedSteepSlopeTop(v.GetGroundSlope())) return i;
	}
	NOT_REACHED();
}

/**
 * Get the offset of the top of ground in the voxel stack (for steep slopes the top voxel).
 * @return Index in the voxel array for the top voxel containing the ground.
 */
int VoxelStack::GetTopGroundOffset() const
{
	for (int i = this->height - 1; i >= 0; i--) {
		const Voxel &v = *this->voxels[i];
		if (v.GetGroundType() != GTP_INVALID) return i;
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
	uint32 version = ldr.OpenPattern("VSTK");
	if (version >= 1 && version <= 3) {
		int16 base = ldr.GetWord();
		uint16 height = ldr.GetWord();
		uint8 owner = ldr.GetByte();
		if (base < 0 || base + height > WORLD_Z_SIZE || owner >= OWN_COUNT) {
			throw LoadingError("Invalid voxel stack size");
		} else {
			this->base = base;
			this->height = height;
			this->owner = (TileOwner)owner;
			this->voxels.resize(height);
			for (uint i = 0; i < height; i++) {
				this->voxels[i].reset(new Voxel);
				this->voxels[i]->Load(ldr);
			}

			/* In version 3 of VSTK, the fences of the lowest corner of steep slopes have moved from the top voxel to the base voxel. */
			if (version < 3) {

				static const uint16 low_fences_mask[4] = { // Mask for getting the low fences.
					(0xf << (4 * EDGE_SE)) | (0xf << (4 * EDGE_SW)), // ISL_TOP_STEEP_NORTH
					(0xf << (4 * EDGE_SW)) | (0xf << (4 * EDGE_NW)), // ISL_TOP_STEEP_EAST
					(0xf << (4 * EDGE_NW)) | (0xf << (4 * EDGE_NE)), // ISL_TOP_STEEP_SOUTH
					(0xf << (4 * EDGE_NE)) | (0xf << (4 * EDGE_SE)), // ISL_TOP_STEEP_WEST
				};

				for (uint i = 0; i < height; i++) {
					if (this->voxels[i]->GetGroundType() == GTP_INVALID) continue;
					if (!IsImplodedSteepSlopeTop(this->voxels[i]->GetGroundSlope())) continue;
					uint16 mask = low_fences_mask[this->voxels[i]->GetGroundSlope() - ISL_TOP_STEEP_NORTH];

					/* Take out the fences of the top voxel that should be in the base voxel.
					 * Make the low fences in the high voxel invalid. */
					uint16 fences = this->voxels[i]->GetFences();
					uint16 lower_fences = fences & mask;
					uint16 high_invalid = ALL_INVALID_FENCES & mask;
					mask ^= 0xffff;
					this->voxels[i]->SetFences(high_invalid | (fences & mask));

					/* Fix low fences. */
					fences = this->voxels[i + 1]->GetFences();
					this->voxels[i + 1]->SetFences(lower_fences | (fences & mask));

					break; // Only one steep ground slope in a voxel stack at most.
				}
			}
		}
	}
	ldr.ClosePattern();
}

/**
 * Save a voxel stack to the save game file.
 * @param svr Output stream to write.
 */
void VoxelStack::Save(Saver &svr) const
{
	svr.CheckNoOpenPattern();
	svr.StartPattern("VSTK", CURRENT_VERSION_VSTK);
	svr.PutWord(this->base);
	svr.PutWord(this->height);
	svr.PutByte(this->owner);
	for (uint i = 0; i < this->height; i++) this->voxels[i]->Save(svr);
	svr.EndPattern();
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
uint8 VoxelWorld::GetBaseGroundHeight(uint16 x, uint16 y) const
{
	const VoxelStack *vs = this->GetStack(x, y);
	int offset = vs->GetBaseGroundOffset();
	assert(vs->base + offset >= 0 && vs->base + offset <= 255);
	return vs->base + offset;
}

/**
 * Return the top height of the ground at the given voxel stack.
 * @param x Horizontal position.
 * @param y Vertical position.
 * @return Height of the ground (for steep slopes, the top voxel height).
 */
uint8 VoxelWorld::GetTopGroundHeight(uint16 x, uint16 y) const
{
	const VoxelStack *vs = this->GetStack(x, y);
	int offset = vs->GetTopGroundOffset();
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
 * At ground level of each voxel stack are 4 fences, one at each edge (ordered as #EDGE_NE to #EDGE_NW).
 * Due to slopes, some fences are at the bottom voxel (the base voxel) of the ground level, the other
 * fences are at the top voxel. The rule for placement is
 * - Both fences near the top edge of a steep slope are in the top voxel.
 * - Both fences near the bottom edge of a steep slope are in the bottom voxel.
 * - Fences on edges of non-steep slopes with both corners raised are in the top voxel.
 * - Other fences of non-steep slopes are in the bottom voxel.
 *
 * Each voxel has the possibility to store 4 fences (one at each edge). In the general case with a
 * base voxel and a top voxel, there are 8 positions for fences, where 4 of them are used to store
 * ground level fences. Below is the mask for getting the fences at base voxel level, for each non-top slope.
 * A \c 0xF nibble means the fence is stored in the base voxel, a \c 0x0 nibble means the fence is stored
 * in the top voxel. (Top-slopes make no sense to include here, as they only describe the top voxel.)
 *
 * The elementary function is
 * - #GetVoxelZOffsetForFence to query which voxel stores a fences at a given edge for a given ground slope.
 *
 * To work with the masks, handling all fences of a tile, the following functions exist:
 * - #MergeGroundFencesAtBase sets the given fences for the base voxel.
 * - #HasTopVoxelFences tells whether the slope has any fences in the top voxel (that is, does it have a top voxel at all?)
 * - #MergeGroundFencesAtTop sets the given fences for the top voxel.
 *
 * The following functions interact with the map.
 * - #GetGroundFencesFromMap gets the fences at ground level from a voxel stack.
 * - #AddGroundFencesToMap sets the fences to a voxel stack.
 */
static const uint16 _fences_mask_at_base[] = {
	0xFFFF, // ISL_FLAT
	0xFFFF, // ISL_NORTH
	0xFFFF, // ISL_EAST
	0xFFF0, // ISL_NORTH_EAST
	0xFFFF, // ISL_SOUTH
	0xFFFF, // ISL_NORTH_SOUTH
	0xFF0F, // ISL_EAST_SOUTH
	0xFF00, // ISL_NORTH_EAST_SOUTH
	0xFFFF, // ISL_WEST
	0x0FFF, // ISL_NORTH_WEST
	0xFFFF, // ISL_EAST_WEST
	0x0FF0, // ISL_NORTH_EAST_WEST
	0xF0FF, // ISL_SOUTH_WEST
	0x00FF, // ISL_NORTH_SOUTH_WEST
	0xF00F, // ISL_EAST_SOUTH_WEST
	0x0FF0, // ISL_BOTTOM_STEEP_NORTH
	0xFF00, // ISL_BOTTOM_STEEP_EAST
	0xF00F, // ISL_BOTTOM_STEEP_SOUTH
	0x00FF, // ISL_BOTTOM_STEEP_WEST
};

/**
 * Get relative voxel offset for fence placement at an edge for a given bottom ground slope.
 * @param edge Edge being queried.
 * @param base_tile_slope Slope of the ground.
 * @return Offset of voxel position relative to the base voxel (\c 0 for bottom voxel, \c 1 for top voxel).
 */
int GetVoxelZOffsetForFence(TileEdge edge, uint8 base_tile_slope)
{
	assert(base_tile_slope < lengthof(_fences_mask_at_base)); // Top steep slopes are not allowed.
	uint16 mask = ~_fences_mask_at_base[base_tile_slope]; // Swap bits, so 0 means bottom, 0xF means top.
	return GB(mask, 4 * edge, 1); // Take lowest bit of the edge.
}

/**
 * Set the ground fences at a base ground voxel.
 * @param vxbase_fences %Voxel fences data of the base ground voxel.
 * @param fences Ground fences of the voxel stack.
 * @param base_tile_slope Slope of the ground.
 * @return The merged ground fences of the \a vxbase_fences. Non-ground fences are preserved.
 */
uint16 MergeGroundFencesAtBase(uint16 vxbase_fences, uint16 fences, uint8 base_tile_slope)
{
	assert(base_tile_slope < lengthof(_fences_mask_at_base)); // Top steep slopes are not allowed.
	uint16 mask = _fences_mask_at_base[base_tile_slope];
	fences &= mask; // Kill any fence not in the base voxel.
	mask ^= 0xFFFF; // Swap mask to keep only non-fences of the current voxel data.
	return (vxbase_fences & mask) | fences;
}

/**
 * Whether the ground tile slope has fences in the top voxel.
 * @param base_tile_slope Slope of the ground.
 * @return Whether there are any ground fences to be set in the top voxel.
 */
bool HasTopVoxelFences(uint8 base_tile_slope)
{
	return _fences_mask_at_base[base_tile_slope] != 0xFFFF;
}

/**
 * Set the ground fences at a top ground voxel.
 * @param vxtop_fences %Voxel fences data of the top ground voxel.
 * @param fences Ground fences of the voxel stack.
 * @param base_tile_slope Slope of the ground.
 * @return The merged ground fences of the \a vxtop_fences. Non-ground fences are preserved.
 * @note If there is no top voxel, use #ALL_INVALID_FENCES as \a vxtop_fences value.
 */
uint16 MergeGroundFencesAtTop(uint16 vxtop_fences, uint16 fences, uint8 base_tile_slope)
{
	assert(base_tile_slope < lengthof(_fences_mask_at_base)); // Top steep slopes are not allowed.
	uint16 mask = _fences_mask_at_base[base_tile_slope];
	vxtop_fences &= mask; // Keep fences of top voxel that are at ground level in the base voxel.
	mask ^= 0xFFFF; // Swap mask to keep fences that belong in the top voxel.
	return (fences & mask) | vxtop_fences;
}

/**
 * Set the ground fences of a voxel stack.
 * @param fences Ground fences to set.
 * @param stack The voxel stack to assign to.
 * @param base_z Height of the base voxel.
 */
void AddGroundFencesToMap(uint16 fences, VoxelStack *stack, int base_z)
{
	Voxel *v = stack->GetCreate(base_z, false); // It should have ground at least.
	assert(v->GetGroundType() != GTP_INVALID);
	uint8 slope = v->GetGroundSlope();

	v->SetFences(MergeGroundFencesAtBase(v->GetFences(), fences, slope));
	v = stack->GetCreate(base_z + 1, HasTopVoxelFences(slope));
	if (v != nullptr) v->SetFences(MergeGroundFencesAtTop(v->GetFences(), fences, slope));
}

/**
 * Get the ground fences of the given voxel stack.
 * @param stack The voxel stack to query.
 * @param base_z Height of the ground.
 * @return Fences at the edges of the ground in the queried voxel stack.
 */
uint16 GetGroundFencesFromMap(const VoxelStack *stack, int base_z)
{
	const Voxel *v = stack->Get(base_z);
	assert(v->GetGroundType() != GTP_INVALID);
	uint8 slope = v->GetGroundSlope();

	assert(slope < lengthof(_fences_mask_at_base)); // Top steep slopes are not allowed.
	uint16 mask = _fences_mask_at_base[slope];
	uint16 fences = v->GetFences() & mask; // Get ground level fences of the base voxel.
	if (HasTopVoxelFences(slope)) {
		v = stack->Get(base_z + 1);
		uint top_fences = (v != nullptr) ? v->GetFences() : ALL_INVALID_FENCES;
		mask ^= 0xFFFF; // Swap all bits to top voxel mask.
		fences |= top_fences & mask; // Add fences of the top voxel.
	}
	return fences;
}

/**
 * Override the border fence creation at a specific point by declaring that no border fence will be drawn there.
 * @param p Voxel position.
 * @param e Edge of the voxel at which no fence shall be placed.
 */
void VoxelWorld::AddEdgesWithoutBorderFence(const Point16& p, TileEdge e)
{
	this->edges_without_border_fence.insert(std::make_pair(p, e));
	this->UpdateLandBorderFence(p.x - std::min<int>(p.x, 1), p.y - std::min<int>(p.y, 1), 3, 3);
}

/**
 * Add/remove land border fence based on current land ownership for the given tile rectangle.
 * @param x Base X coordinate of the rectangle.
 * @param y Base Y coordinate of the rectangle.
 * @param width Length in X direction of the rectangle.
 * @param height Length in Y direction of the rectangle.
 */
void VoxelWorld::UpdateLandBorderFence(uint16 x, uint16 y, uint16 width, uint16 height)
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

			int16 height = vs->GetBaseGroundOffset() + vs->base;
			uint16 fences = GetGroundFencesFromMap(vs, height);
			for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
				FenceType ftype = GetFenceType(fences, edge);
				/* Don't overwrite user-buildable fences. */
				if (ftype >= FENCE_TYPE_BUILDABLE_BEGIN && ftype < FENCE_TYPE_COUNT) continue;

				/* Decide whether a fence is needed (add a border fence just outside player-owned land). */
				ftype = FENCE_TYPE_INVALID;
				if (owner != OWN_PARK) {
					const Point16 pt = _tile_dxy[edge];
					if ((ix > 0 || pt.x >= 0) && (iy > 0 || pt.y >= 0)) {
						uint16 nx = ix + pt.x;
						uint16 ny = iy + pt.y;
						if (nx < _world.GetXSize() && ny < _world.GetYSize() && _world.GetTileOwner(nx, ny) == OWN_PARK &&
								this->edges_without_border_fence.count(std::make_pair(Point16(ix, iy), edge)) == 0) {
							ftype = FENCE_TYPE_LAND_BORDER;
						}
					}
				}

				fences = SetFenceType(fences, edge, ftype);
			}
			AddGroundFencesToMap(fences, vs, height);
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
	for (unsigned ix = x; ix < x + width; ix++) {
		for (unsigned iy = y; iy < y + height; iy++) {
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
 * Find the park entrance location.
 * If the park has multiple entrances, an arbitrary one is returned.
 * If the park has no entrance at all, the invalid point is returned.
 * @return Park entrance coordinate.
 */
XYZPoint16 VoxelWorld::GetParkEntrance() const
{
	if (this->edges_without_border_fence.empty()) return XYZPoint16::invalid();
	const Point16 &p = this->edges_without_border_fence.begin()->first;
	return XYZPoint16(p.x, p.y, this->GetBaseGroundHeight(p.x, p.y));
}

static const uint32 CURRENT_VERSION_WRLD = 2;   ///< Currently supported version of the WRLD Pattern.

/**
 * Load the world from a file.
 * @param ldr Input stream to read from.
 */
void VoxelWorld::Load(Loader &ldr)
{
	uint32 version = ldr.OpenPattern("WRLD");
	uint16 xsize = 64;
	uint16 ysize = 64;
	this->edges_without_border_fence.clear();
	if (version >= 1 && version <= 2) {
		xsize = ldr.GetWord();
		ysize = ldr.GetWord();
		if (version > 1) {
			for (int i = ldr.GetWord(); i > 0; i--) {
				Point16 p;
				p.x = ldr.GetWord();
				p.y = ldr.GetWord();
				this->edges_without_border_fence.insert(std::make_pair(p, static_cast<TileEdge>(ldr.GetByte())));
			}
		}
	} else if (version != 0) {
		ldr.VersionMismatch(version, CURRENT_VERSION_WRLD);
	}
	if (xsize >= WORLD_X_SIZE || ysize >= WORLD_Y_SIZE) {
		throw LoadingError("World size out of bounds (%u × %u)", xsize, ysize);
	}
	ldr.ClosePattern();

	this->SetWorldSize(xsize, ysize);
	if (version != 0) {
		for (uint16 x = 0; x < xsize; x++) {
			for (uint16 y = 0; y < ysize; y++) {
				VoxelStack *vs = this->GetModifyStack(x, y);
				vs->Load(ldr);
			}
		}
	}
	if (version == 0) this->MakeFlatWorld(8);
}

/**
 * Save the world to a file.
 * @param svr Output stream to save to.
 */
void VoxelWorld::Save(Saver &svr) const
{
	/* Save basic map information (rides are saved as part of the ride). */
	svr.CheckNoOpenPattern();
	svr.StartPattern("WRLD", CURRENT_VERSION_WRLD);
	svr.PutWord(this->GetXSize());
	svr.PutWord(this->GetYSize());
	svr.PutWord(this->edges_without_border_fence.size());
	for (const auto &pair : this->edges_without_border_fence) {
		svr.PutWord(pair.first.x);
		svr.PutWord(pair.first.y);
		svr.PutByte(pair.second);
	}
	svr.EndPattern();
	for (uint16 x = 0; x < this->GetXSize(); x++) {
		for (uint16 y = 0; y < this->GetYSize(); y++) {
			const VoxelStack *vs = this->GetStack(x, y);
			vs->Save(svr);
		}
	}
}

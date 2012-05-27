/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map.h Definition of voxels in the world. */

#ifndef MAP_H
#define MAP_H

#include "tile.h"
#include "path.h"
#include "geometry.h"
#include "people.h"

#include <map>

class Viewport;

static const int WORLD_X_SIZE = 128;        ///< Maximal length of the X side (North-West side) of the world.
static const int WORLD_Y_SIZE = 128;        ///< Maximal length of the Y side (North-East side) of the world.
static const int MAX_VOXEL_STACK_SIZE = 64; ///< At most a stack of 64 voxels.


/**
 * Types of voxels.
 * @ingroup map_group
 */
enum VoxelType {
	VT_EMPTY,     ///< Empty voxel.
	VT_SURFACE,   ///< %Voxel contains path and/or earthy surface.
	VT_COASTER,   ///< %Voxel contains part of a coaster.
	VT_REFERENCE, ///< %Voxel contains part of the referenced voxel.
};

/**
 * Description of a path tile.
 * @ingroup map_group
 */
struct PathVoxelData {
	uint8 type;  ///< Type of path.
	uint8 slope; ///< Slope of the path (imploded).
};

/**
 * Description of ground.
 * @ingroup map_group
 */
struct GroundVoxelData {
	uint8 type;  ///< Type of ground.
	uint8 slope; ///< Slope of the ground (imploded).
};

/**
 * Description of foundation.
 * @ingroup map_group
 */
struct FoundationVoxelData {
	uint8 type;  ///< Type of foundation.
	uint8 slope; ///< Slopes of the foundation (bits 0/1 for NE, 2/3 for ES, 4/5 for SW, and 6/7 for WN edge).
};

/**
 * Description of the earth surface (combined ground and foundations).
 * @note As steep slopes are two voxels high, they have a reference voxel above them.
 * @ingroup map_group
 */
struct SurfaceVoxelData {
	PathVoxelData       path;       ///< Path sprite at this location.
	GroundVoxelData     ground;     ///< Ground sprite at this location.
	FoundationVoxelData foundation; ///< Foundation sprite at this location.
};

/**
 * Description of a referenced voxel.
 * For structures larger than a single voxel, only the base voxel contains a description.
 * The other voxels reference to the base voxel for their sprites.
 * @ingroup map_group
 */
struct ReferenceVoxelData {
	uint16 xpos; ///< Base voxel X coordinate.
	uint16 ypos; ///< Base voxel Y coordinate.
	uint8  zpos; ///< Base voxel Z coordinate.
};

/**
 * One voxel cell in the world.
 * @todo Handle #VT_COASTER voxels.
 * @ingroup map_group
 */
struct Voxel {
public:
	/**
	 * Get the type of the voxel.
	 * @return Voxel type.
	 */
	inline VoxelType GetType() const {
		return (VoxelType)this->type;
	}

	/**
	 * Get the surface data.
	 * @return Surface data (ground tile and foundation).
	 */
	inline SurfaceVoxelData *GetSurface() {
		assert(this->type == VT_SURFACE);
		return &this->surface;
	}

	/**
	 * Get the surface data for read-only access.
	 * @return Surface slope.
	 */
	inline const SurfaceVoxelData *GetSurface() const {
		assert(this->type == VT_SURFACE);
		return &this->surface;
	}

	/**
	 * Set the surface.
	 * @param vd Surface data of the voxel.
	 */
	inline void SetSurface(const SurfaceVoxelData &vd)
	{
		this->type = VT_SURFACE;
		assert(vd.path.type == PT_INVALID || (vd.path.type < PT_COUNT && vd.path.slope < PATH_COUNT));
		assert(vd.ground.type == GTP_INVALID || (vd.ground.type < GTP_COUNT && vd.ground.slope < NUM_SLOPE_SPRITES));
		assert(vd.foundation.type == FDT_INVALID || vd.foundation.type < FDT_COUNT);
		this->surface = vd;
	}

	/**
	 * Get the referenced voxel for read-only access.
	 * @return Referenced voxel position.
	 */
	inline const ReferenceVoxelData *GetReference() const
	{
		assert(this->type == VT_REFERENCE);
		return &this->reference;
	}

	/** Set the referenced voxel. */
	inline void SetReference(const ReferenceVoxelData &rvd)
	{
		this->type = VT_REFERENCE;
		this->reference = rvd;
	}

	/** Set the voxel to empty. */
	inline void SetEmpty()
	{
		this->type = VT_EMPTY;
	}

	uint8 type; ///< Type of the voxel. @see VoxelType
	union {
		SurfaceVoxelData surface;
		ReferenceVoxelData reference;
	};

	PersonList persons; ///< Persons present in this voxel.
};


/**
 * One column of voxels.
 * @ingroup map_group
 */
class VoxelStack {
public:
	VoxelStack();
	~VoxelStack();

	void Clear();
	const Voxel *Get(int16 z) const;
	Voxel *GetCreate(int16 z, bool create);

	VoxelStack *Copy(bool copyPersons) const;
	void MoveStack(VoxelStack *old_stack);

	Voxel *voxels; ///< %Voxel array at this stack.
	int16 base;    ///< Height of the bottom voxel.
	uint16 height; ///< Number of voxels in the stack.
protected:
	bool MakeVoxelStack(int16 new_base, uint16 new_height);
};

/**
 * A world of voxels.
 * @ingroup map_group
 */
class VoxelWorld {
public:
	VoxelWorld();

	void SetWorldSize(uint16 xs, uint16 ys);
	void MakeFlatWorld(int16 z);

	VoxelStack *GetModifyStack(uint16 x, uint16 y);
	const VoxelStack *GetStack(uint16 x, uint16 y) const;
	uint8 GetGroundHeight(uint16 x, uint16 y) const;

	/**
	 * Move a voxel stack to this world. May destroy the original stack in the process.
	 * @param x X coordinate of the stack.
	 * @param y Y coordinate of the stack.
	 * @param old_stack Source stack.
	 */
	void MoveStack(uint16 x, uint16 y, VoxelStack *old_stack)
	{
		this->GetModifyStack(x, y)->MoveStack(old_stack);
	}

	/**
	 * Get a voxel in the world by voxel coordinate.
	 * @param x X coordinate of the voxel.
	 * @param y Y coordinate of the voxel.
	 * @param z Z coordinate of the voxel.
	 * @return Address of the voxel (if it exists).
	 */
	inline const Voxel *GetVoxel(uint16 x, uint16 y, int16 z) const
	{
		return this->GetStack(x, y)->Get(z);
	}

	/**
	 * Get the person list associated with a voxel.
	 * @param x X coordinate of the voxel.
	 * @param y Y coordinate of the voxel.
	 * @param z Z coordinate of the voxel.
	 * @return The person list of the voxel.
	 * @note The voxel has to exist.
	 */
	inline PersonList &GetPersonList(uint16 x, uint16 y, int16 z)
	{
		Voxel *v = this->GetCreateVoxel(x, y, z, false);
		assert(v != NULL);
		return v->persons;
	}

	/**
	 * Get a voxel in the world by voxel coordinate.
	 * @param x X coordinate of the voxel.
	 * @param y Y coordinate of the voxel.
	 * @param z Z coordinate of the voxel.
	 * @param create If the requested voxel does not exist, try to create it.
	 * @return Address of the voxel (if it exists or could be created).
	 */
	inline Voxel *GetCreateVoxel(uint16 x, uint16 y, int16 z, bool create)
	{
		return this->GetModifyStack(x, y)->GetCreate(z, create);
	}

	/**
	 * Get X size of the world.
	 * @return Length of the world in X direction.
	 */
	inline uint16 GetXSize() const
	{
		return this->x_size;
	}

	/**
	 * Get Y size of the world.
	 * @return Length of the world in Y direction.
	 */
	inline uint16 GetYSize() const
	{
		return this->y_size;
	}

private:
	uint16 x_size; ///< Current max x size.
	uint16 y_size; ///< Current max y size.

	VoxelStack stacks[WORLD_X_SIZE * WORLD_Y_SIZE]; ///< All voxel stacks in the world.
};

/**
 * Ground data + modification storage.
 * @ingroup map_group
 */
struct GroundData {
	uint8 height;     ///< Height of the voxel with ground.
	uint8 orig_slope; ///< Original slope data.
	uint8 modified;   ///< Raised or lowered corners.

	GroundData(uint8 height, uint8 orig_slope);

	uint8 GetOrigHeight(TileSlope corner) const;
	bool GetCornerModified(TileSlope corner) const;
	void SetCornerModified(TileSlope corner);
};

/**
 * Map of voxels to ground modification data.
 * @ingroup map_group
 */
typedef std::map<Point32, GroundData> GroundModificationMap;

/**
 * Store and manage terrain changes.
 * @todo Enable pulling the screen min/max coordinates from it, so we can give a good estimate of the area to redraw.
 * @ingroup map_group
 */
class TerrainChanges {
public:
	TerrainChanges(const Point32 &base, uint16 xsize, uint16 ysize);
	~TerrainChanges();

	bool ChangeCorner(const Point32 &pos, TileSlope corner, int direction);
	void ChangeWorld(int direction);

	GroundModificationMap changes; ///< Registered changes.

private:
	Point32 base; ///< Base position of the smooth changing world.
	uint16 xsize; ///< Horizontal size of the smooth changing world.
	uint16 ysize; ///< Vertical size of the smooth changing world.

	GroundData *GetGroundData(const Point32 &pos);
};

/**
 * Map of x/y positions to voxel stacks.
 * @ingroup map_group
 */
typedef std::map<Point32, VoxelStack *> VoxelStackMap;

/**
 * Proposed additions to #_world.
 * Temporary buffer to make changes in the world, and show them to the user
 * without having them really in the game until the user confirms.
 */
class WorldAdditions {
public:
	WorldAdditions();
	~WorldAdditions();

	void Clear();
	void Commit();

	VoxelStack *GetModifyStack(uint16 x, uint16 y);
	const VoxelStack *GetStack(uint16 x, uint16 y) const;

	/**
	 * Get a voxel in the world by voxel coordinate.
	 * @param x X coordinate of the voxel.
	 * @param y Y coordinate of the voxel.
	 * @param z Z coordinate of the voxel.
	 * @return Address of the voxel (if it exists).
	 */
	inline const Voxel *GetVoxel(uint16 x, uint16 y, int16 z) const
	{
		return this->GetStack(x, y)->Get(z);
	}

	/**
	 * Get a voxel in the world by voxel coordinate.
	 * @param x X coordinate of the voxel.
	 * @param y Y coordinate of the voxel.
	 * @param z Z coordinate of the voxel.
	 * @param create If the requested voxel does not exist, try to create it.
	 * @return Address of the voxel (if it exists or could be created).
	 */
	inline Voxel *GetCreateVoxel(uint16 x, uint16 y, int16 z, bool create)
	{
		return this->GetModifyStack(x, y)->GetCreate(z, create);
	}

	void MarkDirty(Viewport *vp);

protected:
	VoxelStackMap modified_stacks; ///< Modified voxel stacks.
};

extern VoxelWorld _world;
extern WorldAdditions _additions;

void EnableWorldAdditions();
void DisableWorldAdditions();

#endif

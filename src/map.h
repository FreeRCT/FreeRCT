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
#include "geometry.h"

#include <map>

static const int WORLD_X_SIZE = 128;        ///< Maximal length of the X side (North-West side) of the world.
static const int WORLD_Y_SIZE = 128;        ///< Maximal length of the Y side (North-East side) of the world.
static const int MAX_VOXEL_STACK_SIZE = 64; ///< At most a stack of 64 voxels.


/** Types of voxels. */
enum VoxelType {
	VT_EMPTY,     ///< Empty voxel.
	VT_SURFACE,   ///< %Voxel contains path and/or earthy surface.
	VT_COASTER,   ///< %Voxel contains part of a coaster.
	VT_REFERENCE, ///< %Voxel contains part of the referenced voxel.
};

/** Description of ground. */
struct GroundVoxelData {
	uint8 type;  ///< Type of ground.
	uint8 slope; ///< Slope of the ground (imploded);
};

/** Description of foundation. */
struct FoundationVoxelData {
	uint8 type;  ///< Type of foundation.
	uint8 slope; ///< Slopes of the foundation (bits 0/1 for NE, 2/3 for ES, 4/5 for SW, and 6/7 for WN edge).
};

/**
 * Description of the earth surface (combined ground and foundations).
 * @note As steep slopes are two voxels high, they have a reference voxel above them.
 */
struct SurfaceVoxelData {
	GroundVoxelData     ground;     ///< Ground sprite at this location.
	FoundationVoxelData foundation; ///< Foundation sprite at this location.
};

/**
 * Description of a referenced voxel.
 * For structures larger than a single voxel, only the base voxel contains a description.
 * The other voxels reference to the base voxel for their sprites.
 */
struct ReferenceVoxelData {
	uint16 xpos; ///< Base voxel X coordinate.
	uint16 ypos; ///< Base voxel Y coordinate.
	uint8  zpos; ///< Base voxel Z coordinate.
};

/**
 * One voxel cell in the world.
 * @todo Handle #VT_COASTER voxels.
 * @todo handle paths in a #VT_SURFACE voxel.
 */
struct Voxel {
public:
	/**
	 * Get the type of the voxel.
	 * @return Voxel type.
	 */
	FORCEINLINE VoxelType GetType() const {
		return (VoxelType)this->type;
	}

	/**
	 * Get the surface data.
	 * @return Surface data (ground tile and foundation).
	 */
	FORCEINLINE SurfaceVoxelData *GetSurface() {
		assert(this->type == VT_SURFACE);
		return &this->surface;
	}

	/**
	 * Get the surface data for read-only access.
	 * @return Surface slope.
	 */
	FORCEINLINE const SurfaceVoxelData *GetSurface() const {
		assert(this->type == VT_SURFACE);
		return &this->surface;
	}

	/**
	 * Set the surface.
	 * @param vd Surface data of the voxel.
	 */
	FORCEINLINE void SetSurface(const SurfaceVoxelData &vd)
	{
		this->type = VT_SURFACE;
		assert(vd.ground.type == GTP_INVALID || (vd.ground.type < GTP_COUNT && vd.ground.slope < NUM_SLOPE_SPRITES));
		assert(vd.foundation.type == FDT_INVALID || vd.foundation.type < FDT_COUNT);
		this->surface = vd;
	}

	/**
	 * Get the referenced voxel.
	 * @return Referenced voxel position.
	 */
	FORCEINLINE const ReferenceVoxelData *GetReference()
	{
		assert(this->type == VT_REFERENCE);
		return &this->reference;
	}

	/**
	 * Get the referenced voxel for read-only access.
	 * @return Referenced voxel position.
	 */
	FORCEINLINE const ReferenceVoxelData *GetReference() const
	{
		assert(this->type == VT_REFERENCE);
		return &this->reference;
	}

	/** Set the referenced voxel. */
	FORCEINLINE void SetReference(const ReferenceVoxelData &rvd)
	{
		this->type = VT_REFERENCE;
		this->reference = rvd;
	}

	/** Set the voxel to empty. */
	FORCEINLINE void SetEmpty()
	{
		this->type = VT_EMPTY;
	}

	uint8 type; ///< Type of the voxel. @see VoxelType
	union {
		SurfaceVoxelData surface;
		ReferenceVoxelData reference;
	};
};


/**
 * One column of voxels.
 */
class VoxelStack {
public:
	VoxelStack();
	~VoxelStack();

	void Clear();
	Voxel *Get(int16 z, bool create);

	Voxel *voxels; ///< Voxel array at this stack.
	int16 base;    ///< Height of the bottom voxel.
	uint16 height; ///< Number of voxels in the stack.
protected:
	bool MakeVoxelStack(int16 new_base, uint16 new_height);
};

/**
 * A world of voxels.
 */
class VoxelWorld {
public:
	VoxelWorld();

	void SetWorldSize(uint16 xs, uint16 ys);
	void MakeFlatWorld(int16 z);
	void MakeBump(uint16 x, uint16 y, int16 z);

	VoxelStack *GetStack(uint16 x, uint16 y);
	uint8 GetGroundHeight(uint16 x, uint16 y);

	/**
	 * Get a voxel in the world by voxel coordinate.
	 * @param x X coordinate of the voxel.
	 * @param y Y coordinate of the voxel.
	 * @param z Z coordinate of the voxel.
	 * @param create If the requested voxel does not exist, try to create it.
	 * @return Address of the voxel (if it exists or could be created).
	 */
	FORCEINLINE Voxel *GetVoxel(uint16 x, uint16 y, int16 z, bool create = false)
	{
		return this->GetStack(x, y)->Get(z, create);
	}

	/**
	 * Get X size of the world.
	 * @return Length of the world in X direction.
	 */
	FORCEINLINE uint16 GetXSize() const
	{
		return this->x_size;
	}

	/**
	 * Get Y size of the world.
	 * @return Length of the world in Y direction.
	 */
	FORCEINLINE uint16 GetYSize() const
	{
		return this->y_size;
	}

private:
	uint16 x_size; ///< Current max x size.
	uint16 y_size; ///< Current max y size.

	VoxelStack stacks[WORLD_X_SIZE * WORLD_Y_SIZE]; ///< All voxel stacks in the world.
};

/** Ground data + modification storage. */
struct GroundData {
	uint8 height;     ///< Height of the voxel with ground.
	uint8 orig_slope; ///< Original slope data.
	uint8 modified;   ///< Raised or lowered corners.

	GroundData(uint8 height, uint8 orig_slope);

	uint8 GetOrigHeight(Slope corner) const;
	bool GetCornerModified(Slope corner) const;
	void SetCornerModified(Slope corner);
};

/** Map of voxels to ground modification data. */
typedef std::map<Point, GroundData> GroundModificationMap;

/** Store and manage terrain changes. */
class TerrainChanges {
public:
	TerrainChanges(const Point &base, uint16 xsize, uint16 ysize);
	~TerrainChanges();

	bool ChangeCorner(const Point &pos, Slope corner, int direction);
	void ChangeWorld(int direction);

private:
	Point base;   ///< Base position of the smooth changing world.
	uint16 xsize; ///< Horizontal size of the smooth changing world.
	uint16 ysize; ///< Vertical size of the smooth changing world.

	GroundModificationMap changes; ///< Registered changes.

	GroundData *GetGroundData(const Point &pos);
};

extern VoxelWorld _world;

#endif

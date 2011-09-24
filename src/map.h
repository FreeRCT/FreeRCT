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

static const int WORLD_X_SIZE = 128; ///< Maximal length of the X side (North-West side) of the world.
static const int WORLD_Y_SIZE = 128; ///< Maximal length of the Y side (North-East side) of the world.

/** Types of voxels. */
enum VoxelType {
	VT_EMPTY,     ///< Empty voxel.
	VT_SURFACE,   ///< Voxel contains path and/or earthy surface.
	VT_COASTER,   ///< Voxel contains part of a coaster.
	VT_REFERENCE, ///< Voxel contains part of the referenced voxel.
};

class Viewport;
struct DrawData;

/**
 * Map of distance to image.
 * Used for temporary sorting and storage of images drawn at the viewport.
 */
typedef std::multimap<int32, DrawData> DrawImages;

/**
 * One voxel cell in the world.
 * @todo Handle #VT_COASTER voxels.
 * @todo Handle #VT_REFERENCE voxels.
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
	 * Get the slope of the surface.
	 * @return Surface slope.
	 */
	FORCEINLINE Slope GetSlope() const {
		return (Slope)this->slope;
	}

	/**
	 * Set the voxel to a surface type.
	 * @param slope %Slope of the surface.
	 */
	FORCEINLINE void SetSurface(Slope slope)
	{
		this->type = VT_SURFACE;
		this->slope = slope;
	}

	/** Set the voxel to empty. */
	FORCEINLINE void SetEmpty()
	{
		this->type = VT_EMPTY;
	}

	void AddSprites(const Viewport *vport, const Rectangle &rect, int xpos, int ypos, int zpos, int32 north_x, int32 north_y, DrawImages &draw_images);

protected:
	uint32 type: 2;  ///< Type of the voxel. @see VoxelType
	uint32 slope: 5; ///< Slope of the tile.
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

extern VoxelWorld _world;

#endif

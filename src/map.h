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
#include "sprite_store.h"
#include "bitmath.h"

#include <map>

class Viewport;

static const int WORLD_X_SIZE = 128; ///< Maximal length of the X side (North-West side) of the world.
static const int WORLD_Y_SIZE = 128; ///< Maximal length of the Y side (North-East side) of the world.
static const int WORLD_Z_SIZE =  64; ///< Maximal height of the world.

/**
 * In general, ride instances are stored in the #RidesManager, where there is room to store all the detailed information
 * carried by the ride instance. Scenery and paths however so small that the instance does not actually carry state
 * information, which means the instance can be shared, greatly reducing memory requirements.
 * These rides are given a fixed ride instance number below to make them easy to handle. The graphics engine still
 * considers them to be rides, and queries the rides manager for their definition and (graphic) representation.
 */
enum SmallRideInstance {
	SRI_FREE,          ///< Ride instance is not used.
	SRI_SAME_AS_NORTH, ///< Ride instance is the same as at the northern part.
	SRI_SAME_AS_EAST,  ///< Ride instance is the same as at the eastern part.
	SRI_SAME_AS_SOUTH, ///< Ride instance is the same as at the southern part.

	SRI_RIDES_START,            ///< First ride instance.
	SRI_PATH = SRI_RIDES_START, ///< Path.
	/// \todo Add other scenery objects, like trees and flower beds.
	SRI_FULL_RIDES, ///< First ride instance number for normal rides (created and stored in #RidesManager).

	SRI_LAST = 255, ///< Biggest possible ride number.
};

class VoxelObject;

/**
 * One voxel cell in the world.
 * A voxel consists of four parts and the ground data.
 * Each part covers one corner. They are numbered using the #TileCorner values. A part consists of
 * - The instance number of the ride that uses the part.
 * - The voxel definition of the ride. This defines what sprite to draw. A ride may attach other meaning to the number,
 *   for example use some bits to display variations.
 * The ground data contains the foundations, and the ground (grass).
 *
 * @ingroup map_group
 */
struct Voxel {
public:
	uint8 instance;       ///< Ride instances that uses this voxel.
	uint16 instance_data; ///< %Voxel data of the #instance stored here.

	/**
	 * Get the ride instance at this voxel.
	 * @return Ride instance at this voxel.
	 */
	inline SmallRideInstance GetInstance() const
	{
		return (SmallRideInstance)this->instance;
	}

	/**
	 * Set the ride instance at this voxel.
	 * @param instance Ride instance at this voxel.
	 */
	inline void SetInstance(SmallRideInstance instance)
	{
		this->instance = instance;
	}

	/**
	 * Can a ride instance be placed here?
	 * @return \c true if the instance can be placed, \c false if not.
	 */
	inline bool CanPlaceInstance() const
	{
		return this->GetInstance() == SRI_FREE;
	}

	/** Remove all instances from this voxel. */
	inline void ClearInstances()
	{
		this->SetInstance(SRI_FREE);
	}

	/**
	 * Get the data associated with the ride instance in this voxel.
	 * @return %Voxel data stored in this voxel.
	 */
	inline uint16 GetInstanceData() const
	{
		return this->instance_data;
	}

	/**
	 * Set the data associated with the ride instance in this voxel.
	 * @param instance_data The voxel data stored in this voxel.
	 */
	inline void SetInstanceData(uint16 instance_data)
	{
		this->instance_data = instance_data;
	}

	/**
	 * Ground and foundations.
	 * - bit  0.. 3 (4): Type of foundation. @see FoundationType
	 * - bit  4.. 7 (4): Ground type. @see GroundType
	 * - bit  8..15 (8): Foundation slopes:
	 *   - bit  8: Northern corner of NE edge is up.
	 *   - bit  9: Eastern  corner of NE edge is up.
	 *   - bit 10: Eastern  corner of SE edge is up.
	 *   - bit 11: Southern corner of SE edge is up.
	 *   - bit 12: Southern corner of SW edge is up.
	 *   - bit 13: Western  corner of SW edge is up.
	 *   - bit 14: Western  corner of NW edge is up.
	 *   - bit 15: Northern corner of NW edge is up.
	 * - bit 16..20 (5): Imploded ground slope. @see #ExpandTileSlope
	 * - bit 21..23 (3): Growth of the tile grass.
	 */
	uint32 ground;

	/* Foundation data access. */

	/**
	 * Get the foundation slope of a surface voxel.
	 * @return The foundation slope.
	 */
	inline uint8 GetFoundationSlope() const
	{
		return GB(this->ground, 8, 8);
	}

	/**
	 * Get the foundation type of a surface voxel.
	 * @return The foundation type.
	 */
	inline FoundationType GetFoundationType() const
	{
		uint val = GB(this->ground, 0, 4);
		return (FoundationType)val;
	}

	/**
	 * Set the foundation slope of a surface voxel.
	 * @param fnd_slope The new foundation slope.
	 */
	inline void SetFoundationSlope(uint8 fnd_slope)
	{
		SB(this->ground, 8, 8, fnd_slope);
	}

	/**
	 * Set the foundation type of a surface voxel.
	 * @param fnd_type The foundation type.
	 */
	inline void SetFoundationType(FoundationType fnd_type)
	{
		assert(fnd_type < FDT_COUNT || fnd_type == FDT_INVALID);
		SB(this->ground, 0, 4, fnd_type);
	}

	/* Ground data access. */

	/**
	 * Get the imploded ground slope of a surface voxel.
	 * Steep slopes are two voxels high (a bottom and a top part).
	 * @return The imploded ground slope.
	 */
	inline uint8 GetGroundSlope() const
	{
		return GB(this->ground, 16, 5);
	}

	/**
	 * Get the ground type of a surface voxel.
	 * @return The ground type.
	 */
	inline GroundType GetGroundType() const
	{
		uint val = GB(this->ground, 4, 4);
		return (GroundType)val;
	}

	/**
	 * Get the growth.
	 * @return The growth of the grass.
	 * @todo Increment this value and (change ground type to other grass kinds) regularly to simulate grass growth.
	 */
	inline uint8 GetGrowth() const
	{
		return GB(this->ground, 21, 3);
	}

	/**
	 * Set the imploded ground slope of a surface voxel.
	 * Steep slopes are two voxels high (a bottom and a top part).
	 * @param gnd_slope The new imploded ground slope.
	 */
	inline void SetGroundSlope(uint8 gnd_slope)
	{
		assert(gnd_slope < 15 + 4 + 4); // 15 non-steep, 4 bottom, 4 top sprites.
		SB(this->ground, 16, 5, gnd_slope);
	}

	/**
	 * Set the ground type of a surface voxel.
	 * @param gnd_type The ground type.
	 */
	inline void SetGroundType(GroundType gnd_type)
	{
		assert(gnd_type < GTP_COUNT || gnd_type == GTP_INVALID);
		SB(this->ground, 4, 4, gnd_type);
	}

	/**
	 * Set the growth.
	 * @param growth New growth value.
	 * @todo Increment this value and (change ground type to other grass kinds) regularly to simulate grass growth.
	 */
	inline void SetGrowth(uint8 growth)
	{
		assert(growth < 8);
		SB(this->ground, 21, 3, growth);
	}

	/**
	 * Is the voxel empty?
	 * @return \c true if it is empty, \c false otherwise.
	 */
	inline bool IsEmpty() const
	{
		return this->GetInstance() == SRI_FREE && this->GetGroundType() == GTP_INVALID && this->GetFoundationType() == FDT_INVALID;
	}

	/** Make the voxel empty. */
	void ClearVoxel()
	{
		this->SetGroundType(GTP_INVALID);
		this->SetFoundationType(FDT_INVALID);
		this->SetGrowth(0);
		this->ClearInstances();
	}

	VoxelObject *voxel_objects; ///< First voxel object in this voxel.

	/**
	 * Does the voxel have any voxel objects currently?
	 * @return Whether it has any voxel objects (\c false means it has no voxel objects).
	 */
	inline bool HasVoxelObjects() const
	{
		return this->voxel_objects != nullptr;
	}
};

/** Base class for (moving) objects that are stored at a voxel position for easy retrieval during drawing. */
class VoxelObject {
public:
	VoxelObject() : next_object(nullptr), prev_object(nullptr), added(false)
	{
	}

	virtual ~VoxelObject();

	virtual const ImageData *GetSprite(const SpriteStorage *sprites, ViewOrientation orient, const Recolouring **recolour) const = 0;

	/**
	 * Add itself to the voxel objects chain.
	 * @param v %Voxel containing the object.
	 */
	void AddSelf(Voxel *v)
	{
		assert(!this->added);
		this->added = true;

		this->next_object = v->voxel_objects;
		if (this->next_object != nullptr) this->next_object->prev_object = this;
		v->voxel_objects = this;
		this->prev_object = nullptr;
	}

	/**
	 * Remove itself from the voxel objects chain.
	 * @param v %Voxel containing the object.
	 */
	void RemoveSelf(Voxel *v)
	{
		assert(this->added);
		this->added = false;

		if (this->next_object != nullptr) this->next_object->prev_object = this->prev_object;
		if (this->prev_object != nullptr) {
			this->prev_object->next_object = this->next_object;
		} else {
			assert(v->voxel_objects == this);
			v->voxel_objects = this->next_object;
		}
	}

	void MarkDirty();

	VoxelObject *next_object; ///< Next voxel object in the linked list.
	VoxelObject *prev_object; ///< Previous voxel object in the linked list.
	bool added;               ///< Whether the voxel object has been added to a voxel.

	int16 x_vox; ///< %Voxel index in X direction of the voxel object.
	int16 y_vox; ///< %Voxel index in Y direction of the voxel object.
	int16 z_vox; ///< %Voxel index in Z direction of the voxel object.
	int16 x_pos; ///< X position of the person inside the voxel (0..255).
	int16 y_pos; ///< Y position of the person inside the voxel (0..255).
	int16 z_pos; ///< Z position of the person inside the voxel (0..255).
};

/**
 * Does the given voxel contain a valid path?
 * @param v %Voxel to examine.
 * @return @c true if the voxel contains a valid path, else \c false.
 * @todo Extend with acceptable types of path (plain path, queueing path, etc.)
 */
static inline bool HasValidPath(const Voxel *v)
{
	return v->instance == SRI_PATH && v->instance_data != PATH_INVALID;
}

/**
 * Get the slope of the path (imploded value).
 * @param v %Voxel to examine.
 * @return Imploded slope of the path.
 * @pre %Voxel should have a valid path.
 */
static inline PathSprites GetImplodedPathSlope(const Voxel *v)
{
	assert(HasValidPath(v));
	assert(v->instance_data < PATH_COUNT);
	return (PathSprites)v->instance_data;
}

/** Possible ownerships of a tile. */
enum TileOwner {
	OWN_NONE,     ///< Tile not owned by the park and not for sale.
	OWN_FOR_SALE, ///< Tile not owned by the park, but can be bought.
	OWN_PARK,     ///< Tile owned by the park.
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

	Voxel *voxels;   ///< %Voxel array at this stack.
	int16 base;      ///< Height of the bottom voxel.
	uint16 height;   ///< Number of voxels in the stack.
	TileOwner owner; ///< Ownership of the base tile of this voxel stack.
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

	/**
	 * Does the provided voxel exist in the world?
	 * @param x X coordinate of the voxel.
	 * @param y Y coordinate of the voxel.
	 * @param z Z coordinate of the voxel.
	 * @return Whether a voxel with content exists at the given position.
	 */
	inline bool VoxelExists(int16 x, int16 y, int16 z) const
	{
		if (x < 0 || x >= (int)this->GetXSize()) return false;
		if (y < 0 || y >= (int)this->GetYSize()) return false;
		const VoxelStack *vs = this->GetStack(x, y);
		if (z < vs->base || z >= vs->base + (int)vs->height) return false;
		return true;
	}

	TileOwner GetTileOwner(uint16 x, uint16 y);
	void SetTileOwner(uint16 x, uint16 y, TileOwner owner);
	void SetTileOwnerRect(uint16 x, uint16 y, uint16 width, uint16 height, TileOwner owner);

private:
	uint16 x_size; ///< Current max x size.
	uint16 y_size; ///< Current max y size.

	VoxelStack stacks[WORLD_X_SIZE * WORLD_Y_SIZE]; ///< All voxel stacks in the world.
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
 * @note This world does not use the #Voxel::voxel_objects list.
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

/**
 * Is the given world voxelstack coordinate within the world boundaries?
 * @param x X position of the voxelstack.
 * @param y Y position of the voxelstack.
 * @return Voxelstack coordinate is within world boundaries.
 */
static inline bool IsVoxelstackInsideWorld(int x, int y)
{
	return x >= 0 && x < _world.GetXSize() && y >= 0 && y < _world.GetYSize();
}

/**
 * Is the given world voxel coordinate within the world boundaries?
 * @param x X position of the voxel.
 * @param y Y position of the voxel.
 * @param z Z position of the voxel.
 * @return %Voxel coordinate is within world boundaries.
 */
static inline bool IsVoxelInsideWorld(int x, int y, int z)
{
	return z >= 0 && z < WORLD_Z_SIZE && IsVoxelstackInsideWorld(x, y);
}

#endif

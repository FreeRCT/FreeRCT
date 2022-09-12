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
#include <set>

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
	SRI_SCENERY,                ///< Scenery items.
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

	/** Constructor */
	explicit Voxel()
	{
		this->ClearVoxel();
	}

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
	 * Fences of the voxel. @see FenceType
	 * - bit  0.. 3: Fence type of the NE edge.
	 * - bit  4.. 7: Fence type of the SE edge.
	 * - bit  8..11: Fence type of the SW edge.
	 * - bit 12..15: Fence type of the NW edge.
	 */
	uint16 fences;

	/**
	 * Get the fences of the voxel. Use #GetFenceType and #SetFenceType for further manipulation
	 * of the fence data.
	 * @return Type of the fences at each edge.
	 */
	inline uint16 GetFences() const
	{
		return this->fences;
	}

	/**
	 * Set all the fences of the voxel.
	 * @param fences Type of the fences at each edge.
	 */
	inline void SetFences(uint16 fences)
	{
		this->fences = fences;
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

	VoxelObject *voxel_objects; ///< First voxel object in this voxel.

	/**
	 * Does the voxel have any voxel objects currently?
	 * @return Whether it has any voxel objects (\c false means it has no voxel objects).
	 */
	inline bool HasVoxelObjects() const
	{
		return this->voxel_objects != nullptr;
	}

	void ClearVoxel();
	void Save(Saver &svr) const;
	void Load(Loader &ldr);
};

/** Base class for (moving) objects that are stored at a voxel position for easy retrieval during drawing. */
class VoxelObject {
public:
	VoxelObject() : next_object(nullptr), prev_object(nullptr), added(false)
	{
	}

	virtual ~VoxelObject();

	/** Holds data about an overlay to draw on top of this object's sprite. */
	struct Overlay {
		const ImageData   *sprite;    ///< Sprite to draw.
		const Recolouring *recolour;  ///< Recolouring specification to use (can be \c nullptr).
	};
	typedef std::vector<Overlay> Overlays;

	virtual const ImageData *GetSprite(const SpriteStorage *sprites, ViewOrientation orient, const Recolouring **recolour) const = 0;
	virtual Overlays GetOverlays(const SpriteStorage *sprites, ViewOrientation orient) const;

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

	/**
	 * Merge voxel coordinate, #vox_pos, with in-voxel coordinate, #pix_pos.
	 * @param vox_pos Coordinates of the voxel in the world.
	 * @param pix_pos Pixel position inside the voxel.
	 * @return Merged coordinates as 32 bit 3D point. Lower 8 bits are the in-voxel coordinate; upper remaining bits are the voxel coordinate.
	 * @see GetVoxelCoordinate, GetInVoxelCoordinate
	 */
	static inline XYZPoint32 MergeCoordinates(const XYZPoint16& vox_pos, const XYZPoint16& pix_pos)
	{
		uint32 x = (static_cast<uint32>(vox_pos.x) << 8) | (pix_pos.x & 0xff);
		uint32 y = (static_cast<uint32>(vox_pos.y) << 8) | (pix_pos.y & 0xff);
		uint32 z = (static_cast<uint32>(vox_pos.z) << 8) | (pix_pos.z & 0xff);

		return XYZPoint32(x, y, z);
	}

	/**
	 * Merge voxel coordinate, #vox_pos, with in-voxel coordinate, #pix_pos.
	 * @return Merged coordinates as 32 bit 3D point. Lower 8 bits are the in-voxel coordinate; upper remaining bits are the voxel coordinate.
	 * @see GetVoxelCoordinate, GetInVoxelCoordinate
	 */
	inline XYZPoint32 MergeCoordinates() const
	{
		return MergeCoordinates(this->vox_pos, this->pix_pos);
	}

	/**
	 * Obtain bits 8..24 of a 32 bit 3D point.
	 * @param p The point containing merged data.
	 * @return The bits as a 16 bit 3D point.
	 * @see MergeCoordinates
	 */
	inline XYZPoint16 GetVoxelCoordinate(const XYZPoint32 &p)
	{
		return XYZPoint16(p.x >> 8, p.y >> 8, p.z >> 8);
	}

	/**
	 * Obtain the first 8 bits of the merged coordinates.
	 * @param p The point containing the merged data.
	 * @return The bits as a 16 bit 3D point.
	 * @see MergeCoordinates
	 */
	inline XYZPoint16 GetInVoxelCoordinate(const XYZPoint32 &p)
	{
		return XYZPoint16(p.x & 0xff, p.y & 0xff, p.z & 0xff);
	}

	void Load(Loader &ldr);
	void Save(Saver &svr);

	VoxelObject *next_object; ///< Next voxel object in the linked list.
	VoxelObject *prev_object; ///< Previous voxel object in the linked list.
	bool added;               ///< Whether the voxel object has been added to a voxel.

	XYZPoint16 vox_pos; ///< %Voxel position of the object.
	XYZPoint16 pix_pos; ///< Position of the object inside the voxel (0..255, but may be outside).
};

/**
 * Does the instance data indicate a valid path (that is, a voxel with an actual path tile)?
 * @param instance_data Instance data to examine.
 * @return Whether the instance data indicates a valid path.
 * @pre The instance must be #SRI_PATH
 */
static inline bool HasValidPath(uint16 instance_data)
{
	return instance_data != PATH_INVALID;
}

/**
 * Does the given voxel contain a valid path?
 * @param v %Voxel to examine.
 * @return @c true if the voxel contains a valid path, else \c false.
 * @todo Extend with acceptable types of path (plain path, queueing path, etc.)
 */
static inline bool HasValidPath(const Voxel *v)
{
	return v->instance == SRI_PATH && HasValidPath(v->instance_data);
}

/**
 * Extract the imploded path slope from the instance data.
 * @param instance_data Instance data to examine.
 * @return Imploded slope of the path.
 * @pre instance data must be a valid path.
 */
static inline PathSprites GetImplodedPathSlope(uint16 instance_data)
{
	return (PathSprites)GB(instance_data, 0, 6);
}

/**
 * Change the path slope in the path instance data.
 * @param instance_data Previous instance data.
 * @param slope New imploded path slope.
 * @return New instance data of a path.
 * @pre instance data must be a valid path.
 */
static inline uint16 SetImplodedPathSlope(uint16 instance_data, uint8 slope)
{
	SB(instance_data, 0, 6, slope);
	return instance_data;
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

	PathSprites ps = GetImplodedPathSlope(v->instance_data);
	assert(ps < PATH_COUNT);
	return ps;
}

/**
 * Get the path type from the path voxel instance data.
 * @param instance_data Instance data to examine.
 * @return Type of path.
 * @pre instance data must be a valid path.
 */
static inline PathType GetPathType(uint16 instance_data)
{
	return (PathType)GB(instance_data, 6, 2);
}

/**
 * Construct instance data for a valid path.
 * @param slope Imploded slope of the path.
 * @param path_type Type of the path.
 * @return Instance data of a valid path.
 */
static inline uint16 MakePathInstanceData(uint8 slope, PathType path_type)
{
	return slope | (path_type << 6);
}

/**
 * Get the fence type of one of the four edges.
 * @param fences Fences data of the voxel.
 * @param edge The edge to retrieve.
 * @return The fence type or #FENCE_TYPE_INVALID if the edge has no fence.
 */
inline FenceType GetFenceType(uint16 fences, TileEdge edge)
{
	return static_cast<FenceType>(GB(fences, edge * 4, 4));
}

/**
 * Set the fence type of one of the four edges.
 * @param fences Fences data of the voxel.
 * @param edge The edge to set.
 * @param ftype Type of fence to set at the given edge.
 * @return The updated fences data.
 */
inline uint16 SetFenceType(uint16 fences, TileEdge edge, FenceType ftype)
{
	return SB(fences, edge * 4, 4, ftype);
}

/** Possible ownerships of a tile. */
enum TileOwner {
	OWN_NONE,     ///< Tile not owned by the park and not for sale.
	OWN_FOR_SALE, ///< Tile not owned by the park, but can be bought.
	OWN_PARK,     ///< Tile owned by the park.

	OWN_COUNT,    ///< Number of valid tile ownership values.
};

/**
 * One column of voxels.
 * @ingroup map_group
 */
class VoxelStack {
public:
	VoxelStack();

	void Clear();
	const Voxel *Get(int16 z) const;
	Voxel *GetCreate(int16 z, bool create);

	int GetTopGroundOffset() const;
	int GetBaseGroundOffset() const;

	void Save(Saver &svr) const;
	void Load(Loader &ldr);

	std::vector<std::unique_ptr<Voxel>> voxels;  ///< %Voxel array at this stack.
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
	uint8 GetTopGroundHeight(uint16 x, uint16 y) const;
	uint8 GetBaseGroundHeight(uint16 x, uint16 y) const;

	/**
	 * Get a voxel in the world by voxel coordinate.
	 * @param vox Coordinate of the voxel.
	 * @return Address of the voxel (if it exists).
	 */
	inline const Voxel *GetVoxel(const XYZPoint16 &vox) const
	{
		return this->GetStack(vox.x, vox.y)->Get(vox.z);
	}

	/**
	 * Get a voxel in the world by voxel coordinate.
	 * @param vox Coordinate of the voxel.
	 * @param create If the requested voxel does not exist, try to create it.
	 * @return Address of the voxel (if it exists or could be created).
	 */
	inline Voxel *GetCreateVoxel(const XYZPoint16 &vox, bool create)
	{
		return this->GetModifyStack(vox.x, vox.y)->GetCreate(vox.z, create);
	}

	/**
	 * Get X voxel size of the world.
	 * @return Length of the world in X direction.
	 */
	inline uint16 GetXSize() const
	{
		return this->x_size;
	}

	/**
	 * Get Y voxel size of the world.
	 * @return Length of the world in Y direction.
	 */
	inline uint16 GetYSize() const
	{
		return this->y_size;
	}

	/**
	 * Does the provided voxel exist in the world?
	 * @param vox Coordinate of the voxel.
	 * @return Whether a voxel with content exists at the given position.
	 */
	inline bool VoxelExists(const XYZPoint16 &vox) const
	{
		if (vox.x < 0 || vox.x >= (int)this->GetXSize()) return false;
		if (vox.y < 0 || vox.y >= (int)this->GetYSize()) return false;
		const VoxelStack *vs = this->GetStack(vox.x, vox.y);
		if (vox.z < vs->base || vox.z >= vs->base + (int)vs->height) return false;
		return true;
	}

	TileOwner GetTileOwner(uint16 x, uint16 y);
	void SetTileOwner(uint16 x, uint16 y, TileOwner owner);
	void SetTileOwnerRect(uint16 x, uint16 y, uint16 width, uint16 height, TileOwner owner);
	void SetTileOwnerGlobally(TileOwner owner);
	void UpdateLandBorderFence(uint16 x, uint16 y, uint16 width, uint16 height);
	void AddEdgesWithoutBorderFence(const Point16& p, TileEdge e);

	XYZPoint16 GetParkEntrance() const;

	void Save(Saver &svr) const;
	void Load(Loader &ldr);

private:
	uint16 x_size; ///< Current max x size (in voxels).
	uint16 y_size; ///< Current max y size (in voxels).

	VoxelStack stacks[WORLD_X_SIZE * WORLD_Y_SIZE]; ///< All voxel stacks in the world.
	std::set<std::pair<Point16, TileEdge>> edges_without_border_fence;  ///< Tile edges at which no border fence is desired.
};

/**
 * Map of x/y positions to voxel stacks.
 * @ingroup map_group
 */
typedef std::map<Point32, VoxelStack *> VoxelStackMap;

int GetVoxelZOffsetForFence(TileEdge edge, uint8 base_tile_slope);
uint16 MergeGroundFencesAtBase(uint16 vxbase_fences, uint16 fences, uint8 base_tile_slope);
bool HasTopVoxelFences(uint8 base_tile_slope);
uint16 MergeGroundFencesAtTop(uint16 vxtop_fences, uint16 fences, uint8 base_tile_slope);
void AddGroundFencesToMap(uint16 fences, VoxelStack *stack, int base_z);
uint16 GetGroundFencesFromMap(const VoxelStack *stack, int base_z);

extern VoxelWorld _world;

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
 * @param vox Position of the voxel.
 * @return %Voxel coordinate is within world boundaries.
 */
static inline bool IsVoxelInsideWorld(const XYZPoint16 &vox)
{
	return vox.z >= 0 && vox.z < WORLD_Z_SIZE && IsVoxelstackInsideWorld(vox.x, vox.y);
}

#endif

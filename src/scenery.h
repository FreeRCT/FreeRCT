/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file scenery.h Available scenery item types. */

#ifndef SCENERY_H
#define SCENERY_H

#include <map>
#include <memory>
#include <vector>

#include "stdafx.h"
#include "language.h"
#include "loadsave.h"
#include "map.h"
#include "money.h"
#include "sprite_store.h"

static const uint16 INVALID_VOXEL_DATA = 0xffff;     ///< Voxel instance data value that indicates that no scenery item should be drawn.

/** Available categories of scenery types. */
enum SceneryCategory {
	/* Keep the values in sync with the constants from the SCNY block documentation! */
	SCC_SCENARIO   = 0,  ///< Can not be built or removed by the player.
	SCC_TREES      = 1,  ///< Trees.
	SCC_FLOWERBEDS = 2,  ///< Flowerbeds.
	SCC_FOUNTAINS  = 3,  ///< Fountains.
};

/** A type of scenery, e.g. trees, flower beds. */
class SceneryType {
public:
	SceneryType();

	void Load(RcdFileReader *rcf_file, const ImageMap &sprites, const TextMap &texts);

	std::string internal_name; ///< Unique internal name of the scenery item type.
	SceneryCategory category;  ///< Category of scenery.
	StringID name;             ///< Name of this item type.
	uint8 width_x;             ///< Number of voxels in x direction occupied by this item.
	uint8 width_y;             ///< Number of voxels in x direction occupied by this item.
	std::unique_ptr<int8[]> heights;

	Money buy_cost;            ///< Cost of buying this item.
	Money return_cost;         ///< Amount of money returned or consumed when removing this item.
	Money return_cost_dry;     ///< Amount of money returned or consumed when removing this item when it's dry.
	uint32 watering_interval;  ///< How often in milliseconds this item needs watering (\c 0 means never).
	uint32 min_watering_interval;  ///< This item may not be watered more often than once in this many milliseconds (only valid if #watering_interval is valid).

	bool symmetric;                        ///< Whether this item is perfectly symmetric and can therefore not be rotated.
	const TimedAnimation *main_animation;  ///< Graphics for this scenery item.
	const TimedAnimation *dry_animation;   ///< Graphics for this scenery item when it's dry.
	ImageData* previews[4];                ///< Previews for the scenery placement window.

	/**
	 * The height of this scenery item at the given position.
	 * @param x X coordinate, relative to the base position.
	 * @param y Y coordinate, relative to the base position.
	 * @return The height.
	 */
	inline int8 GetHeight(const int8 x, const int8 y) const
	{
		return heights[x * width_y + y];
	}
};

/** An actual scenery item in the world. */
class SceneryInstance {
public:
	explicit SceneryInstance(const SceneryType *t);
	~SceneryInstance();

	StringID CanPlace() const;
	void GetSprites(const XYZPoint16 &vox, uint16 voxel_number, uint8 orient, const ImageData *sprites[4], uint8 *platform) const;
	void OnAnimate(int delay);
	bool IsDry() const;
	bool ShouldBeWatered() const;
	void MarkDirty();

	void InsertIntoWorld();
	void RemoveFromWorld();

	void Load(Loader &ldr);
	void Save(Saver &svr) const;

	const SceneryType *type;   ///< Type of item.
	XYZPoint16 vox_pos;        ///< Position of the item's base voxel.
	uint8 orientation;         ///< Orientation of the item.
	uint32 animtime;           ///< Time in the animation, in milliseconds.
	uint32 time_since_watered; ///< Time since the item was last watered, in milliseconds. Only valid if the #type needs watering.
};

/** A type of path object, e.g. benches, litter. */
class PathObjectType {
public:
	Money buy_cost;                   ///< Cost of buying this item (\c 0 indicates it can't be bought).
	uint8 type_id;                    ///< Unique type ID for saveloading.
	bool ignore_edges;                ///< This item lives in the middle of a path rather than on the edges.
	bool can_exist_on_slope;          ///< This item can exist on a sloping path edge (ignored for types that #ignore_edges).

	static const PathObjectType *Get(uint8 id);

	/* User-buyable path object types. */
	static const PathObjectType BENCH;      ///< A bench on which two people can sit.
	static const PathObjectType LITTERBIN;  ///< A bin into which guests can throw litter.
	static const PathObjectType LAMP;       ///< A decorative (but functionless) street lamp.

	/* Non-user-buyable path object types. */
	static const PathObjectType LITTER;  ///< Litter thrown on the ground by guests.
	static const PathObjectType VOMIT ;  ///< What guests leave on the ground after visiting nauseating rides.

	static const uint8 INVALID_PATH_OBJECT = 0;  ///< ID that denotes an invalid path object.

	static const uint8  BIN_MAX_CAPACITY  = 8;       ///< How much litter fits into the bin.
	static const uint8  BIN_FULL_CAPACITY = 7;       ///< The bin should be emptied when it contains this much litter.
	static const uint16 NO_GUEST_ON_BENCH = 0xFFFF;  ///< Denotes absence of a guest on a bench.

private:
	PathObjectType(uint8 id, bool ign, bool slope, const Money &cost);

	static std::map<uint8, PathObjectType*> all_types;  ///< All path object types with their IDs.
};

/**
 * An actual path object in the world.
 *
 * For items that are placed near path edges (such as benches), an instance of this class corresponds
 * to the 4 items on the four edges of a voxel. In this case, the #state attribute denotes the visibility
 * and demolishing state of each of these four items:
 * The lowest four bits of the #state attribute denote whether the item is visible on the NE,SE,SW,NW edge of the tile respectively.
 * The upper four bits of the #state attribute denote whether the item on the respective edge is demolished.
 * Additional #type-specific data is stored in the #data attributes.
 * For benches, the lower 2 bytes denote the ID of the guest sitting on the left half of the bench and the other
 * 2 bytes the ID of the guest sitting on the right half of the bench. The ID #PathObjectType::NO_GUEST_ON_BENCH denotes absence of a guest.
 * For litter bins, #data denotes the filling state of the bin in the range (0 .. #PathObjectType::BIN_CAPACITY).
 *
 * For litter and vomit, the #state attribute denotes the sprite type in range (0 .. #PathDecoration::[flat|ramp]_[litter|vomit]_count).
 * \c 0xFF denotes that it has not been initialized yet.
 * The first #data attribute denotes the slope direction in (#EDGE_BEGIN, #EDGE_COUNT-1), or #INVALID_EDGE for flat tiles.
 */
class PathObjectInstance {
public:
	explicit PathObjectInstance(const PathObjectType *t, const XYZPoint16 &pos, const XYZPoint16 &offset);
	~PathObjectInstance();

	void RecomputeExistenceState();
	void Demolish(TileEdge e);

	/** Holds data about a path object to draw. */
	struct PathObjectSprite {
		PathObjectSprite(const ImageData *s, XYZPoint16 off);

		const ImageData *sprite;  ///< Sprite to draw.
		XYZPoint16 offset;        ///< Image offset inside the voxel.
		bool semi_transparent;    ///< Draw this item semi-transparent.
	};
	std::vector<PathObjectSprite> GetSprites(uint8 orientation) const;
	bool GetExistsOnTileEdge(TileEdge e) const;
	bool GetDemolishedOnTileEdge(TileEdge e) const;

	uint   GetFreeBinCapacity(TileEdge e) const;
	bool   BinNeedsEmptying  (TileEdge e) const;
	uint16 GetLeftGuest      (TileEdge e) const;
	uint16 GetRightGuest     (TileEdge e) const;
	void   AddItemToBin      (TileEdge e);
	void   EmptyBin          (TileEdge e);
	void   SetLeftGuest      (TileEdge e, uint16 id);
	void   SetRightGuest     (TileEdge e, uint16 id);

	void Load(Loader &ldr);
	void Save(Saver &svr) const;

	const PathObjectType *type;  ///< Type of item.

	XYZPoint16 vox_pos;  ///< Base position of this item.
	XYZPoint16 pix_pos;  ///< Position of the object inside the voxel (0..255). Only valid for litter and vomit.

private:
	void SetExistsOnTileEdge(TileEdge e, bool b);
	void SetDemolishedOnTileEdge(TileEdge e, bool d);
	void RemoveGuestsFromBench(TileEdge e);

	uint32 data[4];      ///< #type-specific instance data for each edge.
	uint8 state;         ///< Presence and demolishing states.
};

/** All the scenery items in the world. */
class SceneryManager {
public:
	SceneryManager();

	void AddSceneryType(std::unique_ptr<SceneryType> &type);
	uint16 GetSceneryTypeIndex(const SceneryType *type) const;
	const SceneryType *GetType(uint16 index) const;
	const SceneryType *GetType(const std::string &internal_name) const;
	std::vector<const SceneryType*> GetAllTypes(SceneryCategory cat) const;

	void OnAnimate(int delay);
	void Clear();

	void AddItem(SceneryInstance* item);
	void RemoveItem(const XYZPoint16 &pos);
	SceneryInstance *GetItem(const XYZPoint16 &pos);

	void  SetPathObjectInstance(const XYZPoint16 &pos, const PathObjectType *type);
	void  AddLitter(const XYZPoint16 &pos, const XYZPoint16 &offset);
	void  AddVomit (const XYZPoint16 &pos, const XYZPoint16 &offset);
	void  RemoveLitterAndVomit(const XYZPoint16 &pos);
	uint  CountLitterAndVomit (const XYZPoint16 &pos) const;
	uint8 CountDemolishedItems(const XYZPoint16 &pos) const;
	PathObjectInstance *GetPathObject(const XYZPoint16 &pos);

	std::vector<PathObjectInstance::PathObjectSprite> DrawPathObjects(const XYZPoint16 &pos, uint8 orientation) const;

	void Load(Loader &ldr);
	void Save(Saver &svr) const;

	SceneryInstance    *temp_item;         ///< A scenery item that is currently being placed (not owned).
	PathObjectInstance *temp_path_object;  ///< A path object type that is currently being placed (not owned).

private:
	std::vector<std::unique_ptr<SceneryType>> scenery_item_types;  ///< All available scenery types.

	std::map     <XYZPoint16, std::unique_ptr<SceneryInstance   >> all_items       ;  ///< All scenery items                 in the world, with their base voxel as key.
	std::map     <XYZPoint16, std::unique_ptr<PathObjectInstance>> all_path_objects;  ///< All     user-buyable path objects in the world, with their base voxel as key.
	std::multimap<XYZPoint16, std::unique_ptr<PathObjectInstance>> litter_and_vomit;  ///< All non-user-buyable path objects in the world, with their base voxel as key.
};

extern SceneryManager _scenery;

#endif

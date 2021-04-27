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

static const int MAX_NUMBER_OF_SCENERY_TYPES = 128;  ///< Maximal number of scenery types (limit is uint16 in the map).
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

	bool Load(RcdFileReader *rcf_file, const ImageMap &sprites, const TextMap &texts);

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
	uint32 last_watered;       ///< Time since the item was last watered, in milliseconds. Only valid if the #type needs watering.
};

/** All the scenery items in the world. */
class SceneryManager {
public:
	SceneryManager();

	bool AddSceneryType(SceneryType *type);
	uint16 GetSceneryTypeIndex(const SceneryType *type) const;
	const SceneryType *GetType(uint16 index) const;
	std::vector<const SceneryType*> GetAllTypes(SceneryCategory cat) const;

	void OnAnimate(int delay);
	void Clear();

	void AddItem(SceneryInstance* item);
	void RemoveItem(const XYZPoint16 &pos);
	SceneryInstance *GetItem(const XYZPoint16 &pos);

	void Load(Loader &ldr);
	void Save(Saver &svr) const;

	SceneryInstance *temp_item;  ///< An item that is currently being placed (not owned).

private:
	std::unique_ptr<SceneryType> scenery_item_types[MAX_NUMBER_OF_SCENERY_TYPES];  ///< All available scenery types.
	std::map<XYZPoint16, std::unique_ptr<SceneryInstance>> all_items;              ///< All scenery items in the world, with their base voxel as key.
};

extern SceneryManager _scenery;

#endif

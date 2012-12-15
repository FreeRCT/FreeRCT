/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ride_type.h Available types of rides. */

#ifndef RIDE_TYPE_H
#define RIDE_TYPE_H

#include "palette.h"

static const int NUMBER_SHOP_RECOLOUR_MAPPINGS =  3; ///< Number of (random) colour remappings of a shop type.
static const int MAX_NUMBER_OF_RIDE_TYPES      = 64; ///< Maximal number of types of rides.
static const int MAX_NUMBER_OF_RIDE_INSTANCES  = 64; ///< Maximal number of ride instances (limit is uint16 in the map).
static const uint16 INVALID_RIDE_INSTANCE      = 0xFFFF; ///< Value representing 'no ride instance found'.

/**
 * Kinds of ride types.
 * @todo Split coasters into different kinds??
 */
enum RideTypeKind {
	RTK_SHOP,            ///< Ride type allows buying useful stuff.
	RTK_GENTLE,          ///< Gentle kind of ride.
	RTK_WET,             ///< Ride type uses water.
	RTK_COASTER,         ///< Ride type is a coaster.

	RTK_RIDE_KIND_COUNT, ///< Number of kinds of ride types.
};

/** Flags describing properties the shop type. */
enum ShopFlags {
	SHF_NE_ENTRANCE = 1 << EDGE_NE, ///< Entrance in NE direction (unrotated).
	SHF_SE_ENTRANCE = 1 << EDGE_SE, ///< Entrance in SE direction (unrotated).
	SHF_SW_ENTRANCE = 1 << EDGE_SW, ///< Entrance in SW direction (unrotated).
	SHF_NW_ENTRANCE = 1 << EDGE_NW, ///< Entrance in NW direction (unrotated).

	SHF_ENTRANCE_BITS = (SHF_NE_ENTRANCE | SHF_SE_ENTRANCE | SHF_SW_ENTRANCE | SHF_NW_ENTRANCE), ///< Bit mask for the entrances.
};

/** Type of items that can be bought. */
enum ItemType {
	ITP_NOTHING = 0,      ///< Dummy item to denote nothing can be bought.
	ITP_DRINK = 8,        ///< A drink in a cup.
	ITP_ICE_CREAM = 9,    ///< Ice cream (a drink that can be eaten).
	ITP_NORMAL_FOOD = 16, ///< 'plain' food.
	ITP_SALTY_FOOD = 24,  ///< 'salty' food, makes thirsty.
	ITP_UMBRELLA = 32,    ///< Umbrella against the rain.
	ITP_PARK_MAP = 40,    ///< Map of the park, may improve finding the attractions.
};

/**
 * A 'ride' where you can buy food, drinks, and other stuff you need for a visit.
 * @todo Allow for other sized sprites + different recolours.
 * @todo Allow for other kinds of ride type.
 */
class ShopType {
public:
	ShopType();
	~ShopType();

	bool Load(RcdFile *rcf_file, uint32 length, const ImageMap &sprites, const TextMap &texts);
	StringID GetString(uint16 number) const;
	const StringID *GetInstanceNames() const;

	const RideTypeKind kind;    ///< Kind of ride type.
	int8 height;                ///< Number of voxels used by this shop.
	uint8 flags;                ///< Shop flags. @see ShopFlags
	int32 item_cost[2];         ///< Cost of the items on sale.
	uint8 item_type[2];         ///< Type of items being sold.
	int32 monthly_cost;         ///< Monthly costs for owning a shop.
	int32 monthly_open_cost;    ///< Monthly extra costs if the shop is opened.
	RandomRecolouringMapping colour_remappings[NUMBER_SHOP_RECOLOUR_MAPPINGS]; ///< %Random sprite recolour mappings.
	ImageData *views[4];        ///< 64 pixel wide shop graphics.

protected:
	TextData *text;             ///< Strings of the shop.
	uint16 string_base;         ///< Base offset of the string in this shop.
};

/** State of a ride instance. */
enum RideInstanceState {
	RIS_FREE,      ///< Ride instance is not used currently.
	RIS_ALLOCATED, ///< Ride instance is allocated but not yet in play.
	RIS_CLOSED,    ///< Ride instance is available, but closed for the public.
	RIS_OPEN,      ///< Ride instance is open for use by the public.
};

/** Flags of the ride instance. */
enum RideInstanceFlags {
	RIF_MONTHLY_PAID = 0, ///< Bit number of the flags indicating the monthly presence costs have been paid.
	RIF_OPENED_PAID  = 1, ///< Bit number of the flags indicating the open costs have been paid this month.
};

/**
 * A ride in the park.
 * @todo Add ride parts and other things that need to be stored with a ride.
 */
class RideInstance {
public:
	RideInstance();
	~RideInstance();

	void ClaimRide(const ShopType *type, uint8 *name);
	void SetRide(uint8 orientation, uint16 xpos, uint16 ypos, uint8 zpos);
	uint8 GetEntranceDirections();
	void FreeRide();

	void OnNewMonth();
	void OpenRide();
	void CloseRide();

	uint8 name[64];       ///< Name of the ride, if it is instantiated.
	const ShopType *type; ///< Ride type used.
	uint8 orientation;    ///< Orientation of the shop.
	uint8 state;          ///< State of the instance. @see RideInstanceState
	uint16 xpos;          ///< X position of the base voxel.
	uint16 ypos;          ///< Y position of the base voxel.
	uint8  zpos;          ///< Z position of the base voxel.
	uint8 flags;          ///< Flags of the instance. @see RideInstanceFlags
	EditableRecolouring recolour_map; ///< Recolour map of the instance.
};

/** Storage of available ride types. */
class RidesManager {
public:
	RidesManager();
	~RidesManager();

	RideInstance *GetRideInstance(uint16 num);
	const RideInstance *GetRideInstance(uint16 num) const;
	bool AddRideType(ShopType *shop_type);
	uint16 GetFreeInstance();
	void NewInstanceAdded(uint16 num);

	void OnNewMonth();

	/**
	 * Get a ride type from the class.
	 * @param number Index of the ride type to retrieve.
	 * @return The ride type, or \c NULL if it does not exist.
	 */
	const ShopType *GetRideType(uint16 number) const
	{
		if (number >= lengthof(this->ride_types)) return NULL;
		return this->ride_types[number];
	}

	const ShopType *ride_types[MAX_NUMBER_OF_RIDE_TYPES]; ///< Loaded types of rides.
	RideInstance instances[MAX_NUMBER_OF_RIDE_INSTANCES]; ///< Rides available in the park.
};

extern RidesManager _rides_manager;

#endif


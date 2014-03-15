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
#include "money.h"

static const int NUMBER_SHOP_RECOLOUR_MAPPINGS =  3; ///< Number of (random) colour remappings of a shop type.
static const int MAX_NUMBER_OF_RIDE_TYPES      = 64; ///< Maximal number of types of rides.
static const int MAX_NUMBER_OF_RIDE_INSTANCES  = 64; ///< Maximal number of ride instances (limit is uint16 in the map).
static const uint16 INVALID_RIDE_INSTANCE      = 0xFFFF; ///< Value representing 'no ride instance found'.

static const int NUMBER_ITEM_TYPES_SOLD = 2; ///< Number of different items that a ride can sell.

class RideInstance;

/**
 * Kinds of ride types.
 * @todo Split coasters into different kinds??
 */
enum RideTypeKind {
	RTK_SHOP,    ///< Ride type allows buying useful stuff.
	RTK_GENTLE,  ///< Gentle kind of ride.
	RTK_WET,     ///< Ride type uses water.
	RTK_COASTER, ///< Ride type is a coaster.

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

/** Base class of ride types. */
class RideType {
public:
	RideType(RideTypeKind rtk);
	virtual ~RideType();

	virtual bool CanMakeInstance() const;
	virtual RideInstance *CreateInstance() const = 0;
	void SetupStrings(TextData *text, StringID base, StringID start, StringID end, StringID name, StringID desc);

	const RideTypeKind kind; ///< Kind of ride type.
	Money monthly_cost;      ///< Monthly costs for owning a ride.
	Money monthly_open_cost; ///< Monthly extra costs if the ride is opened.
	ItemType item_type[NUMBER_ITEM_TYPES_SOLD]; ///< Type of items being sold.
	Money item_cost[NUMBER_ITEM_TYPES_SOLD];    ///< Cost of the items on sale.

	StringID GetString(uint16 number) const;
	StringID GetTypeName() const;
	StringID GetTypeDescription() const;
	virtual const ImageData *GetView(uint8 orientation) const = 0;
	virtual const StringID *GetInstanceNames() const = 0;

protected:
	TextData *text;           ///< Strings of the ride type.
	StringID str_base;        ///< Base offset of the string in the ride type.
	StringID str_start;       ///< First string in the ride type.
	StringID str_end;         ///< One beyond the last string in the ride type.
	StringID str_name;        ///< String with the name of the ride type.
	StringID str_description; ///< String with the description of the ride type.
};

/** State of a ride instance. */
enum RideInstanceState {
	RIS_ALLOCATED, ///< Ride instance is allocated but not yet in play.
	RIS_BUILDING,  ///< Ride instance is being constructed.
	RIS_TESTING,   ///< Ride instance is being tested.
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
	RideInstance(const RideType *rt);
	virtual ~RideInstance();

	virtual void GetSprites(uint16 voxel_number, uint8 orient, const ImageData *sprites[4]) const = 0;
	virtual uint8 GetEntranceDirections(uint16 xvox, uint16 yvox, uint8 zvox) const = 0;
	bool CanBeVisited(uint16 xvox, uint16 yvox, uint8 zvox, TileEdge edge) const;

	void SellItem(int item_index);
	ItemType GetSaleItemType(int item_index) const;
	Money GetSaleItemPrice(int item_index) const;

	RideTypeKind GetKind() const;
	const RideType *GetRideType() const;

	virtual void OnAnimate(int delay);
	void OnNewMonth();
	void BuildRide();
	void OpenRide();
	void CloseRide();

	uint16 GetIndex() const;

	uint8 name[64];           ///< Name of the ride, if it is instantiated.
	uint8 state;              ///< State of the instance. @see RideInstanceState
	uint8 flags;              ///< Flags of the instance. @see RideInstanceFlags
	Recolouring recolour_map; ///< Recolour map of the instance.

	Money total_profit;      ///< Total profit of the ride.
	Money total_sell_profit; ///< Profit of selling items.
	Money item_price[NUMBER_ITEM_TYPES_SOLD]; ///< Selling price of each item type.
	int64 item_count[NUMBER_ITEM_TYPES_SOLD]; ///< Number of items sold for each type.

protected:
	const RideType *type; ///< Ride type used.
};

/** Storage of available ride types. */
class RidesManager {
public:
	RidesManager();
	~RidesManager();

	RideInstance *GetRideInstance(uint16 num);
	const RideInstance *GetRideInstance(uint16 num) const;
	RideInstance *FindRideByName(const uint8 *name);

	bool AddRideType(RideType *type);

	uint16 GetFreeInstance(const RideType *type);
	RideInstance *CreateInstance(const RideType *type, uint16 num);
	void NewInstanceAdded(uint16 num);
	void DeleteInstance(uint16 num);
	void CheckNoAllocatedRides() const;

	void OnAnimate(int delay);
	void OnNewMonth();

	/**
	 * Get a ride type from the class.
	 * @param number Index of the ride type to retrieve.
	 * @return The ride type, or \c nullptr if it does not exist.
	 */
	const RideType *GetRideType(uint16 number) const
	{
		if (number >= lengthof(this->ride_types)) return nullptr;
		return this->ride_types[number];
	}

	const RideType *ride_types[MAX_NUMBER_OF_RIDE_TYPES];  ///< Loaded types of rides.
	RideInstance *instances[MAX_NUMBER_OF_RIDE_INSTANCES]; ///< Rides available in the park.
};

const RideInstance *RideExistsAtBottom(int xpos, int ypos, int zpos, TileEdge edge);

extern RidesManager _rides_manager;

#endif

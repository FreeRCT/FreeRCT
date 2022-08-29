/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ride_type.h Available types of rides. */

#ifndef RIDE_TYPE_H
#define RIDE_TYPE_H

#include <memory>
#include <vector>

#include "palette.h"
#include "money.h"
#include "random.h"

static const uint16 INVALID_RIDE_INSTANCE = 0xFFFF; ///< Value representing 'no ride instance found'.

static const int MAX_RIDE_RECOLOURS = 3;  ///< Maximum number of entries in a RideInstance's recolour map.

static const int NUMBER_ITEM_TYPES_SOLD = 2; ///< Number of different items that a ride can sell.

static const int BREAKDOWN_GRACE_PERIOD = 30; ///< Number of days to wait before random breakdowns after first time opening ride.

class RideInstance;

/**
 * Kinds of ride types.
 * @todo Split coasters into different kinds??
 */
enum RideTypeKind {
	RTK_SHOP,    ///< Ride type allows buying useful stuff.
	RTK_GENTLE,  ///< Gentle kind of ride.
	RTK_THRILL,  ///< Thrilling kind of ride.
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
	SHF_ENTRANCE_NONE = 0, ///< Shop tile without entrances (used for upper storeys of buildings).
};

/** Type of items that can be bought. */
enum ItemType {
	ITP_NOTHING = 0,      ///< Dummy item to denote nothing can be bought.
	ITP_DRINK = 8,        ///< A drink in a cup.
	ITP_ICE_CREAM = 9,    ///< Ice cream (a drink that can be eaten).
	ITP_NORMAL_FOOD = 16, ///< 'plain' food.
	ITP_SALTY_FOOD = 24,  ///< 'salty' food, makes thirsty.
	ITP_UMBRELLA = 32,    ///< Umbrella against the rain.
	ITP_BALLOON = 33,     ///< Balloon.
	ITP_PARK_MAP = 40,    ///< Map of the park, may improve finding the attractions.
	ITP_SOUVENIR = 41,    ///< Souvenir of the park.
	ITP_MONEY = 48,       ///< Money for more spending (i.e. an ATM).
	ITP_TOILET = 49,      ///< Dropping of waste.
	ITP_FIRST_AID = 50,   ///< Nausea treatment.
	ITP_RIDE = 60,        ///< Entrance ticket for a normal ride.
};

/** Class describing an entrance or exit of rides. */
class RideEntranceExitType {
public:
	RideEntranceExitType();
	bool Load(RcdFileReader *rcf_file, const ImageMap &sprites, const TextMap &texts);

	std::string internal_name;       ///< Unique internal name of the entrance/exit type.
	bool is_entrance;                ///< Whether this is an entrance type or exit type.
	StringID name;                   ///< Name of the entrance or exit type.
	StringID recolour_description_1; ///< First recolouring description.
	StringID recolour_description_2; ///< Second recolouring description.
	StringID recolour_description_3; ///< Third recolouring description.
	ImageData* images[4][2];         ///< The entrance/exit's graphics.
	Recolouring recolours;           ///< Sprite recolour map.

	/* The following two values must be the same for all entrance/exit sets. */
	static uint8 entrance_height; ///< The height of all rides' entrances in voxels.
	static uint8 exit_height;     ///< The height of all rides' exits in voxels.
};

static const int RIDE_ENTRANCE_FEE_STEP_SIZE = 10;                ///< Step size of changing a ride's entrance fee in the GUI.
static const int MAINTENANCE_INTERVAL_STEP_SIZE = 5 * 60 * 1000;  ///< Step size of changing a ride's maintenance interval in the GUI, in milliseconds.
static const int IDLE_DURATION_STEP_SIZE = 5 * 1000;              ///< Step size of changing a ride's idle duration in the GUI, in milliseconds.

static const int16 RELIABILITY_RANGE = 10000;                ///< Reliability parameters are in range 0..10000.
static const uint32 RATING_NOT_YET_CALCULATED = 0xffffffff;  ///< Excitement/intensity/nausea rating was not calculated yet.

/** Base class of ride types. */
class RideType {
public:
	RideType(RideTypeKind rtk);
	virtual ~RideType() = default;

	virtual bool CanMakeInstance() const;
	virtual RideInstance *CreateInstance() const = 0;
	void SetupStrings(TextData *text, StringID base, StringID start, StringID end, StringID name, StringID desc);

	RideTypeKind kind;       ///< Kind of ride type.
	Money monthly_cost;      ///< Monthly costs for owning a ride.
	Money monthly_open_cost; ///< Monthly extra costs if the ride is opened.
	ItemType item_type[NUMBER_ITEM_TYPES_SOLD]; ///< Type of items being sold.
	Money item_cost[NUMBER_ITEM_TYPES_SOLD];    ///< Cost of the items on sale.
	Recolouring recolours;   ///< Sprite recolour map.

	int16 reliability_max;               ///< Maximum reliability.
	int16 reliability_decrease_daily;    ///< Reliability decrease per day.
	int16 reliability_decrease_monthly;  ///< Maximum reliability decrease per month.

	StringID GetString(uint16 number) const;
	StringID GetTypeName() const;
	StringID GetTypeDescription() const;
	virtual const ImageData *GetView(uint8 orientation) const = 0;
	virtual const StringID *GetInstanceNames() const = 0;

	/**
	 * Get the ride type's unique internal name.
	 * @return The internal name.
	 */
	const std::string &InternalName() const
	{
		return this->internal_name;
	}

protected:
	TextData *text;           ///< Strings of the ride type.
	StringID str_base;        ///< Base offset of the string in the ride type.
	StringID str_start;       ///< First string in the ride type.
	StringID str_end;         ///< One beyond the last string in the ride type.
	StringID str_name;        ///< String with the name of the ride type.
	StringID str_description; ///< String with the description of the ride type.
	std::string internal_name;  ///< Unique internal name of the ride type.
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

/** Answers of RideInstance::EnterRide */
enum RideEntryResult {
	RER_REFUSED, ///< Entry is refused.
	RER_ENTERED, ///< Entry is given, the guest is staying inside the ride.
	RER_DONE,    ///< Entry is given, and visit is immediately done.
	RER_WAIT,    ///< No entry is given, but the guest is told to wait outside and try again a little while later.
};

/**
 * A ride in the park.
 * @todo Add ride parts and other things that need to be stored with a ride.
 */
class RideInstance {
public:
	RideInstance(const RideType *rt);
	virtual ~RideInstance() = default;

	virtual void GetSprites(const XYZPoint16 &vox, uint16 voxel_number, uint8 orient, const ImageData *sprites[4], uint8 *platform) const = 0;
	virtual const Recolouring *GetRecolours(const XYZPoint16 &pos) const;
	virtual uint8 GetEntranceDirections(const XYZPoint16 &vox) const = 0;
	virtual RideEntryResult EnterRide(int guest, const XYZPoint16 &vox, TileEdge entry_edge) = 0;
	virtual XYZPoint32 GetExit(int guest, TileEdge entry_edge) = 0;
	virtual void RemoveAllPeople() = 0;
	virtual void RemoveFromWorld() = 0;
	virtual void InsertIntoWorld() = 0;
	virtual bool CanBeVisited(const XYZPoint16 &vox, TileEdge edge) const;
	virtual bool PathEdgeWanted(const XYZPoint16 &vox, TileEdge edge) const;

	void SellItem(int item_index);
	ItemType GetSaleItemType(int item_index) const;
	Money GetSaleItemPrice(int item_index) const;
	virtual Money GetSaleItemCost(int item_index) const;
	virtual void InitializeItemPricesAndStatistics();
	void NotifyLongQueue();

	RideTypeKind GetKind() const;
	const RideType *GetRideType() const;

	virtual void OnAnimate(int delay);
	virtual void OnNewMonth();
	virtual void OnNewDay();
	void BuildRide();
	virtual bool CanOpenRide() const;
	virtual void OpenRide();
	virtual void CloseRide();
	void BreakDown();
	void CallMechanic();
	void MechanicArrived();
	void SetEntranceType(int type);
	void SetExitType(int type);
	virtual bool IsEntranceLocation(const XYZPoint16& pos) const;
	virtual bool IsExitLocation(const XYZPoint16& pos) const;
	virtual EdgeCoordinate GetMechanicEntrance() const = 0;

	virtual void Load(Loader &ldr);
	virtual void Save(Saver &svr);

	uint16 GetIndex() const;

	std::string name;               ///< Name of the ride, if it is instantiated.
	uint8 state;                    ///< State of the instance. @see RideInstanceState
	uint8 flags;                    ///< Flags of the instance. @see RideInstanceFlags
	Recolouring recolours;          ///< Recolour map of the instance.
	Recolouring entrance_recolours; ///< Recolour map of the ride's entrance.
	Recolouring exit_recolours;     ///< Recolour map of the ride's exit.

	Money total_profit;      ///< Total profit of the ride.
	Money total_sell_profit; ///< Profit of selling items.
	Money item_price[NUMBER_ITEM_TYPES_SOLD]; ///< Selling price of each item type.
	int64 item_count[NUMBER_ITEM_TYPES_SOLD]; ///< Number of items sold for each type.

	int16 max_reliability;               ///< Current maximum reliability in 0..10000.
	int16 reliability;                   ///< Current reliability in 0..10000.
	uint32 maintenance_interval;         ///< Desired number of milliseconds between maintenance operations (\c 0 means never).
	uint32 time_since_last_maintenance;  ///< Number of milliseconds since the last maintenance operation.
	bool broken;                         ///< The ride is currently broken down.
	uint32 time_since_last_long_queue_message;  ///< Number of milliseconds since this ride last sent a message that the queue is very long.
	uint32 excitement_rating;                   ///< Ride's excitement rating in percent.
	uint32 intensity_rating;                    ///< Ride's intensity rating in percent.
	uint32 nausea_rating;                       ///< Ride's nausea rating in percent.

	uint16 entrance_type;    ///< Index of this ride's entrance.
	uint16 exit_type;        ///< Index of this ride's exit.

protected:
	virtual void RecalculateRatings() = 0;

	const RideType *type;   ///< Ride type used.
	Random rnd;             ///< Random number generator for determining ride breakage.
	bool mechanic_pending;  ///< Whether a mechanic has been called and did not arrive yet.
};

void SetRideRatingStringParam(uint32 rating);

/** Storage of available ride types. */
class RidesManager {
public:
	RideInstance *GetRideInstance(uint16 num);
	const RideInstance *GetRideInstance(uint16 num) const;
	RideInstance *FindRideByName(const std::string &name);

	bool AddRideType(std::unique_ptr<RideType> type);
	bool AddRideEntranceExitType(std::unique_ptr<RideEntranceExitType> &type);

	uint16 GetFreeInstance(const RideType *type);
	RideInstance *CreateInstance(const RideType *type, uint16 num);
	void NewInstanceAdded(uint16 num);
	void DeleteInstance(uint16 num);
	void DeleteAllRideInstances();

	void OnAnimate(int delay);
	void OnNewMonth();
	void OnNewDay();

	void Load(Loader &ldr);
	void Save(Saver &svr);

	/**
	 * Get a ride type from the class.
	 * @param number Index of the ride type to retrieve.
	 * @return The ride type, or \c nullptr if it does not exist.
	 */
	const RideType *GetRideType(uint16 number) const
	{
		if (number >= this->ride_types.size()) return nullptr;
		return this->ride_types[number].get();
	}
	const RideType *GetRideType(const std::string &internal_name) const;
	int GetEntranceIndex(const std::string &internal_name) const;
	int GetExitIndex(const std::string &internal_name) const;

	std::vector<std::unique_ptr<const RideType>> ride_types;             ///< Loaded types of rides.
	std::map<uint16, std::unique_ptr<RideInstance>> instances;           ///< Rides available in the park.
	std::vector<std::unique_ptr<const RideEntranceExitType>> entrances;  ///< Available ride entrance types.
	std::vector<std::unique_ptr<const RideEntranceExitType>> exits;      ///< Available ride exit types.
};

RideInstance *RideExistsAtBottom(XYZPoint16 pos, TileEdge edge);

extern RidesManager _rides_manager;

#endif

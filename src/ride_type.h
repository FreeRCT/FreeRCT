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

/**
 * A 'ride' where you can buy food, drinks, and other stuff you need for a visit.
 * @todo Allow for other sized sprites + different recolours.
 * @todo Allow for other kinds of ride type.
 */
class ShopType {
public:
	ShopType();
	~ShopType();

	RideTypeKind GetRideKind() const;

	bool Load(RcdFile *rcf_file, uint32 length, const ImageMap &sprites, const TextMap &texts);
	StringID GetString(uint16 number) const;

	int height;                 ///< Number of voxels used by this shop.
	RandomRecolouringMapping colour_remappings[NUMBER_SHOP_RECOLOUR_MAPPINGS]; ///< %Random sprite recolour mappings.
	ImageData *views[4];        ///< 64 pixel wide shop graphics.

protected:
	TextData *text;             ///< Strings of the shop.
	uint16 string_base;         ///< Base offset of the string in this shop.
};

/** State of a ride instance. */
enum RideInstanceState {
	RIS_NONE,   ///< Ride instance has no state currently.
	RIS_CLOSED, ///< Ride instance is available, but closed for the public.
	RIS_OPEN,   ///< Ride instance is open for use by the public.
};

/**
 * A ride in the park.
 * @todo Add ride parts and other things that need to be stored with a ride.
 */
class RideInstance {
public:
	RideInstance();
	~RideInstance();

	void SetRide(ShopType *type, uint8 *name);
	void OpenRide();
	void CloseRide();
	void FreeRide();

	uint8 name[64]; ///< Name of the ride, if it is instantiated.
	ShopType *type; ///< Ride type used. \c NULL means the instance is not used.
	uint8 state;    ///< State of the instance. @see RideInstanceState
	EditableRecolouring recolour_map; ///< Recolour map of the instance.
};

/** Storage of available ride types. */
class RidesManager {
public:
	RidesManager();
	~RidesManager();

	bool AddRideType(ShopType *shop_type);
	uint16 GetFreeInstance();

	ShopType *ride_types[MAX_NUMBER_OF_RIDE_TYPES];       ///< Loaded types of rides.
	RideInstance instances[MAX_NUMBER_OF_RIDE_INSTANCES]; ///< Rides available in the park.
};

extern RidesManager _rides_manager;

#endif


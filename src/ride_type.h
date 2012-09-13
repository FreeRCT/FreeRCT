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

static const int NUMBER_SHOP_RECOLOUR_MAPPINGS = 3; ///< Number of (random) colour remappings of a shop type.
static const int MAX_NUMBER_OF_RIDES = 64; ///< Maximal number of (types of) rides.

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


/** Storage of available ride types. */
class RideTypeManager {
public:
	RideTypeManager();
	~RideTypeManager();

	bool Add(ShopType *shop_type);

	ShopType *shops[MAX_NUMBER_OF_RIDES];
};

extern RideTypeManager _ride_type_manager;

#endif


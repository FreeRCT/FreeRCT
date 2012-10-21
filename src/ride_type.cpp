/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ride_type.cpp Ride type storage and retrieval. */

#include "stdafx.h"
#include "string_func.h"
#include "sprite_store.h"
#include "language.h"
#include "sprite_store.h"
#include "fileio.h"
#include "ride_type.h"

RidesManager _rides_manager; ///< Storage and retrieval of ride types and rides in the park.

#include "table/shops_strings.cpp"

ShopType::ShopType() : kind(RTK_SHOP)
{
	this->height = 0;
	for (uint i = 0; i < lengthof(this->views); i++) this->views[i] = NULL;
	this->text = NULL;
}

ShopType::~ShopType()
{
	/* Images and texts are handled by the sprite collector, no need to release its memory here. */
}

/**
 * Load a type of shop from the RCD file.
 * @param rcd_file Rcd file being loaded.
 * @param length Length of the data in the current block.
 * @param sprites Already loaded sprites.
 * @param texts Already loaded texts.
 * @return Loading was successful.
 */
bool ShopType::Load(RcdFile *rcd_file, uint32 length, const ImageMap &sprites, const TextMap &texts)
{
	if (length != 2 + 1 + 1 + 4*4 + 3*4 + 4*4 + 2 + 4) return false;
	uint16 width = rcd_file->GetUInt16(); // XXX Widths other than 64
	this->height = rcd_file->GetUInt8();
	if (this->height != 1) return false; // Other heights may fail.
	this->flags = rcd_file->GetUInt8() & 0xF;

	for (int i = 0; i < 4; i++) {
		ImageData *view;
		if (!LoadSpriteFromFile(rcd_file, sprites, &view)) return false;
		if (width != 64) continue; // Silently discard other sizes.
		this->views[i] = view;
	}

	for (uint i = 0; i < 3; i++) {
		uint32 recolour = rcd_file->GetUInt32();
		if (i < lengthof(this->colour_remappings)) this->colour_remappings[i].Set(recolour);
	}
	this->item_cost[0] = rcd_file->GetInt32();
	this->item_cost[1] = rcd_file->GetInt32();
	this->monthly_cost = rcd_file->GetInt32();
	this->monthly_open_costs = rcd_file->GetInt32();

	uint8 val = rcd_file->GetUInt8();
	if (val != ITP_NOTHING && val != ITP_DRINK && val != ITP_ICE_CREAM && val != ITP_NORMAL_FOOD &&
			val != ITP_SALTY_FOOD && val != ITP_UMBRELLA && val != ITP_PARK_MAP) {
		return false;
	}
	this->item_type[0] = val;

	val = rcd_file->GetUInt8();
	if (val != ITP_NOTHING && val != ITP_DRINK && val != ITP_ICE_CREAM && val != ITP_NORMAL_FOOD &&
			val != ITP_SALTY_FOOD && val != ITP_UMBRELLA && val != ITP_PARK_MAP) {
		return false;
	}
	this->item_type[1] = val;

	if (!LoadTextFromFile(rcd_file, texts, &this->text)) return false;
	this->string_base = _language.RegisterStrings(*this->text, _shops_strings_table);
	return true;
}

/**
 * Get the string instance for the generic shops string of \a number.
 * @param number Generic shops string number to retrieve.
 * @return The instantiated string for this shop type.
 */
StringID ShopType::GetString(uint16 number) const
{
	assert(number >= STR_GENERIC_SHOP_START && number < SHOPS_STRING_TABLE_END);
	return this->string_base + (number - STR_GENERIC_SHOP_START);
}

RideInstance::RideInstance()
{
	this->name[0] = '\0';
	this->type = NULL;
	this->state = RIS_NONE;
}

RideInstance::~RideInstance()
{
	/* Nothing to do currently. In the future, free instance memory. */
}

/**
 * Construct an instance of a ride type here.
 * @param type Ride type to instantiate.
 * @param name Suggested name, may be \c NULL.
 * @pre Entry must not be in use currently.
 * @note Also initialize the other variables of the instance with #SetRide
 */
void RideInstance::ClaimRide(ShopType *type, uint8 *name)
{
	assert(this->type == NULL);
	assert(type != NULL);

	this->type = type;
	if (name == NULL) {
		this->name[0] = '\0';
	} else {
		StrECpy(this->name, this->name + lengthof(this->name), name);
	}
	this->state = RIS_CLOSED;

	// XXX Make a recolour-map.
}

/**
 * Update a ride instance with its position in the world.
 * @param orientation Orientation of the shop.
 * @param xpos X position of the shop.
 * @param ypos Y position of the shop.
 * @param zpos Z position of the shop.
 */
void RideInstance::SetRide(uint8 orientation, uint16 xpos, uint16 ypos, uint8 zpos)
{
	assert(this->type != NULL); // Updating a non-claimed ride is not so useful.

	this->orientation = orientation;
	this->xpos = xpos;
	this->ypos = ypos;
	this->zpos = zpos;
}

/**
 * Open the ride for the public.
 * @pre The ride is open.
 */
void RideInstance::OpenRide()
{
	assert(this->type != NULL && state == RIS_CLOSED);
	this->state = RIS_OPEN;
}

/**
 * Close the ride for the public.
 * @todo Currently closing is instantly, we may want to have a transition phase here.
 * @pre The ride is open.
 */
void RideInstance::CloseRide()
{
	assert(this->type != NULL && state == RIS_OPEN);
	this->state = RIS_CLOSED;
}

/**
 * Destroy the instance.
 * @todo The small matter of cleaning up in the world map.
 * @pre Instance must be closed.
 */
void RideInstance::FreeRide()
{
	assert(this->type != NULL && state == RIS_CLOSED);

	// XXX Free the instance memory here.
	this->type = NULL;
	this->name[0] = '\0';
	this->state = RIS_NONE;
}

/** Default constructor of the rides manager. */
RidesManager::RidesManager()
{
	for (uint i = 0; i < lengthof(this->ride_types); i++) this->ride_types[i] = NULL;
}

RidesManager::~RidesManager()
{
	for (uint i = 0; i < lengthof(this->ride_types); i++) {
		if (this->ride_types[i] != NULL) delete this->ride_types[i];
	}
}

/**
 * Get the requested ride instance.
 * @param num Ride instance to retrieve.
 * @return The requested ride, or \c NULL if not available.
 */
RideInstance *RidesManager::GetRideInstance(uint16 num)
{
	if (num >= lengthof(this->instances) || this->instances[num].type == NULL) return NULL;
	return &this->instances[num];
}

/**
 * Get the requested ride instance (read-only).
 * @param num Ride instance to retrieve.
 * @return The requested ride, or \c NULL if not available.
 */
const RideInstance *RidesManager::GetRideInstance(uint16 num) const
{
	if (num >= lengthof(this->instances) || this->instances[num].type == NULL) return NULL;
	return &this->instances[num];
}

/**
 * Add a new ride type to the manager.
 * @param shop_type New ride type to add.
 */
bool RidesManager::AddRideType(ShopType *shop_type)
{
	for (uint i = 0; i < lengthof(this->ride_types); i++) {
		if (this->ride_types[i] == NULL) {
			this->ride_types[i] = shop_type;
			return true;
		}
	}
	return false;
}

/**
 * Find a free entry to add a ride instance.
 * @return Index of a free instance if it exists (be sure to claim it immediately), or #INVALID_RIDE_INSTANCE if all instances are used.
 */
uint16 RidesManager::GetFreeInstance()
{
	for (uint16 i = 0; i < lengthof(this->instances); i++) {
		if (this->instances[i].type == NULL) return i;
	}
	return INVALID_RIDE_INSTANCE;
}


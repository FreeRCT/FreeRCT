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
#include "map.h"
#include "fileio.h"
#include "ride_type.h"
#include "bitmath.h"
#include "gamelevel.h"
#include "window.h"

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
	this->monthly_open_cost = rcd_file->GetInt32();

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

/**
 * Get names of shop instances.
 * @return Array of proposed names, terminated with #STR_INVALID.
 */
const StringID *ShopType::GetInstanceNames() const
{
	static const StringID names[] = {SHOPS_NAME_INSTANCE1, SHOPS_NAME_INSTANCE2, STR_INVALID};
	return names;
}

RideInstance::RideInstance()
{
	this->name[0] = '\0';
	this->type = NULL;
	this->state = RIS_FREE;
	this->flags = 0;
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
void RideInstance::ClaimRide(const ShopType *type, uint8 *name)
{
	assert(this->state != RIS_FREE);
	assert(type != NULL);

	this->type = type;
	this->flags = 0;
	if (name == NULL) {
		this->name[0] = '\0';
	} else {
		StrECpy(this->name, this->name + lengthof(this->name), name);
	}

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
	assert(this->state != RIS_FREE); // Updating a non-claimed ride is not useful.

	this->orientation = orientation;
	this->xpos = xpos;
	this->ypos = ypos;
	this->zpos = zpos;
	this->flags = 0;
}

/**
 * Get the set edges with an entrance to the ride.
 * @return Bit set of #TileEdge
 * @todo This needs to be generalized for rides larger than a single voxel.
 */
uint8 RideInstance::GetEntranceDirections() const
{
	uint8 entrances = this->type->flags & SHF_ENTRANCE_BITS;
	return ROL(entrances, this->orientation);
}

/**
 * Can the ride be visited, assuming the shop is approached from direction \a edge?
 * @param edge Direction of entrance (exit direction of the neighbouring voxel).
 * @return The shop can be visited.
 */
bool RideInstance::CanBeVisited(TileEdge edge) const
{
	if (this->state != RIS_OPEN) return false;
	return GB(this->GetEntranceDirections(), (edge + 2) % 4, 1) != 0;
}

/** Monthly update of the shop administration. */
void RideInstance::OnNewMonth()
{
	this->total_profit -= this->type->monthly_cost;
	_user.Pay(this->type->monthly_cost);
	SB(this->flags, RIF_MONTHLY_PAID, 1, 1);
	if (this->state == RIS_OPEN) {
		this->total_profit -= this->type->monthly_open_cost;
		_user.Pay(this->type->monthly_open_cost);
		SB(this->flags, RIF_OPENED_PAID, 1, 1);
	} else {
		SB(this->flags, RIF_OPENED_PAID, 1, 0);
	}

	NotifyChange(WC_SHOP_MANAGER, this->GetIndex(), CHG_DISPLAY_OLD, 0);
}

/**
 * Open the ride for the public.
 * @pre The ride is open.
 */
void RideInstance::OpenRide()
{
	assert(this->state == RIS_CLOSED);
	this->state = RIS_OPEN;

	/* Perform payments if they have not been done this month. */
	bool money_paid = false;
	if (GB(this->flags, RIF_MONTHLY_PAID, 1) == 0) {
		this->total_profit -= this->type->monthly_cost;
		_user.Pay(this->type->monthly_cost);
		SB(this->flags, RIF_MONTHLY_PAID, 1, 1);
		money_paid = true;
	}
	if (GB(this->flags, RIF_OPENED_PAID, 1) == 0) {
		this->total_profit -= this->type->monthly_open_cost;
		_user.Pay(this->type->monthly_open_cost);
		SB(this->flags, RIF_OPENED_PAID, 1, 1);
		money_paid = true;
	}
	if (money_paid) NotifyChange(WC_SHOP_MANAGER, this->GetIndex(), CHG_DISPLAY_OLD, 0);
}

/**
 * Close the ride for the public.
 * @todo Currently closing is instantly, we may want to have a transition phase here.
 * @pre The ride is open.
 */
void RideInstance::CloseRide()
{
	assert(this->state != RIS_FREE);
	this->state = RIS_CLOSED;
}

/**
 * Destroy the instance.
 * @todo The small matter of cleaning up in the world map.
 * @pre Instance must be closed.
 */
void RideInstance::FreeRide()
{
	assert(this->state != RIS_FREE);

	// XXX Free the instance memory here.
	this->type = NULL;
	this->name[0] = '\0';
	this->state = RIS_FREE;
}

/** Default constructor of the rides manager. */
RidesManager::RidesManager()
{
	for (uint i = 0; i < lengthof(this->ride_types); i++) this->ride_types[i] = NULL;
	/* Ride instances are initialized by their constructor. */
}

RidesManager::~RidesManager()
{
	for (uint i = 0; i < lengthof(this->ride_types); i++) {
		if (this->ride_types[i] != NULL) delete this->ride_types[i];
	}
}

/** A new month has started; perform monthly payments. */
void RidesManager::OnNewMonth()
{
	for (uint16 i = 0; i < lengthof(this->instances); i++) {
		if (this->instances[i].state == RIS_FREE || this->instances[i].state == RIS_ALLOCATED) continue;
		this->instances[i].OnNewMonth();
	}
}

/**
 * Get the requested ride instance.
 * @param num Ride instance to retrieve.
 * @return The requested ride, or \c NULL if not available.
 */
RideInstance *RidesManager::GetRideInstance(uint16 num)
{
	if (num >= lengthof(this->instances)) return NULL;
	return &this->instances[num];
}

/**
 * Get the ride instance index number.
 * @return Ride instance index.
 */
uint16 RideInstance::GetIndex() const
{
	assert(this >= &_rides_manager.instances[0] && this < &_rides_manager.instances[lengthof(_rides_manager.instances)]);
	return this - &_rides_manager.instances[0];
}

/**
 * Get the requested ride instance (read-only).
 * @param num Ride instance to retrieve.
 * @return The requested ride, or \c NULL if not available.
 */
const RideInstance *RidesManager::GetRideInstance(uint16 num) const
{
	if (num >= lengthof(this->instances)) return NULL;
	return &this->instances[num];
}

/**
 * Add a new ride type to the manager.
 * @param shop_type New ride type to add.
 * @return \c true if the addition was successful, else \c false.
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
		if (this->instances[i].state == RIS_FREE) return i;
	}
	return INVALID_RIDE_INSTANCE;
}

/**
 * A new ride instance was added. Initialize it further.
 * @param num Index of the new ride instance.
 */
void RidesManager::NewInstanceAdded(uint16 num)
{
	RideInstance *ri = this->GetRideInstance(num);
	assert(ri->state == RIS_ALLOCATED);
	const ShopType *st = ri->type;

	/* Find a new name for the instance. */
	const StringID *names = st->GetInstanceNames();
	int count = 0;
	while (count < 10 && names[count] != STR_INVALID) count++; // Arbitrary limit of 10.
	int shop_num = 1;
	while (shop_num >= 1) {
		for (int idx = 0; idx < count; idx++) {
			/* Construct a new name. */
			if (shop_num == 1) {
				DrawText(st->GetString(names[idx]), ri->name, lengthof(ri->name));
			} else {
				_str_params.SetStrID(1, st->GetString(names[idx]));
				_str_params.SetNumber(2, shop_num);
				DrawText(GUI_NUMBERED_INSTANCE_NAME, ri->name, lengthof(ri->name));
			}

			/* Find the same name in the existing rides. */
			bool found = false;
			for (uint16 i = 0; i < lengthof(this->instances); i++) {
				const RideInstance &rj = this->instances[i];
				if (rj.state == RIS_FREE || rj.state == RIS_ALLOCATED) continue;
				assert(i != num); // Due to its allocated state.
				if (StrEqual(ri->name, rj.name)) {
					found = true;
					break;
				}
			}
			if (!found) {
				shop_num = -10;
				break;
			}
		}
		shop_num++;
	}

	/* Initialize money and counters. */
	ri->total_profit = 0;
	ri->total_sell_profit = 0;
	ri->item_price[0] = st->item_cost[0] * 12 / 10; // Make 20% profit.
	ri->item_price[1] = st->item_cost[1] * 12 / 10;
	ri->item_count[0] = 0;
	ri->item_count[1] = 0;

	ri->CloseRide();
}

/**
 * Does a ride entrance exists at/to the bottom the given voxel in the neighbouring voxel?
 * @param xpos X coordinate of the voxel.
 * @param ypos Y coordinate of the voxel.
 * @param zpos Z coordinate of the voxel.
 * @param edge Direction to move to get the neighbouring voxel.
 * @pre voxel coordinate must be valid in the world.
 * @return The ride at the neighbouring voxel, if available (else \c NULL is returned).
 */
const RideInstance *RideExistsAtBottom(int xpos, int ypos, int zpos, TileEdge edge)
{
	xpos += _tile_dxy[edge].x;
	ypos += _tile_dxy[edge].y;
	if (xpos < 0 || xpos >= _world.GetXSize() || ypos < 0 || ypos >= _world.GetYSize()) return NULL;

	const Voxel *vx = _world.GetVoxel(xpos, ypos, zpos);
	if (vx == NULL || vx->GetType() == VT_EMPTY || vx->GetType() == VT_REFERENCE) {
		/* No ride here, check the voxel below. */
		if (zpos == 0) return NULL;
		vx = _world.GetVoxel(xpos, ypos, zpos - 1);
	}
	if (vx == NULL || vx->GetType() != VT_SURFACE) return NULL;
	uint16 pr = vx->GetPathRideNumber();
	if (pr >= PT_START) return NULL;
	return _rides_manager.GetRideInstance(pr);
}

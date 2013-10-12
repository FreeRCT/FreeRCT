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
#include "finances.h"

/**
 * \page Rides
 *
 * Rides are the central concept in what guests 'do' to have fun.
 * The main classes are #RideType and #RideInstance.
 *
 * - The #RideType represents the type of a ride, e.g. "the kiosk" or a "basic steel roller coaster".
 *   - Shop types are implement in #ShopType.
 *   - Coaster types are implemented in #CoasterType.
 *
 * - The #RideInstance represents actual rides in the park.
 *   - Shops instances are implemented in #ShopInstance.
 *   - Coasters instances are implemented in #CoasterInstance.
 *
 * The #RidesManager (#_rides_manager) manages both ride types and ride instances.
 */

RidesManager _rides_manager; ///< Storage and retrieval of ride types and rides in the park.

/**
 * Ride type base class constructor.
 * @param rtk Kind of ride.
 */
RideType::RideType(RideTypeKind rtk) : kind(rtk)
{
	this->monthly_cost = 12345; // Arbitrary non-zero cost.
	this->monthly_open_cost = 12345; // Arbitrary non-zero cost.
	for (int i = 0; i < NUMBER_ITEM_TYPES_SOLD; i++) this->item_type[i] = ITP_NOTHING;
	for (int i = 0; i < NUMBER_ITEM_TYPES_SOLD; i++) this->item_cost[i] = 12345; // Arbitrary non-zero cost.
	this->SetupStrings(NULL, 0, 0, 0, 0, 0);
}

RideType::~RideType()
{
}

/**
 * \fn RideInstance *RideType::CreateInstance()
 * Construct a ride instance of the ride type.
 * @return An ride of its own type.
 */

/**
 * \fn const ImageData *RideType::GetView(uint8 orientation) const
 * Get a display of the ride type for the purchase screen.
 * @param orientation Orientation of the ride type. Many ride types have \c 4 view orientations,
 *                    but some types may have only a view for orientation \c 0.
 * @return An image to display in the purchase window, or \c NULL if the queried orientation has no view.
 */

/**
 * \fn const StringID *RideType::GetInstanceNames() const
 * Get the instance base names of rides.
 * @return Array of proposed names, terminated with #STR_INVALID.
 */

/**
 * Setup the the strings of the ride type.
 * @param text Strings from the RCD file.
 * @param base Assigned base number for strings of this type.
 * @param start First generic string number of this type.
 * @param end One beyond the last generic string number of this type.
 * @param name String containing the name of this ride type.
 * @param desc String containing the description of this ride type.
 */
void RideType::SetupStrings(TextData *text, StringID base, StringID start, StringID end, StringID name, StringID desc)
{
	this->text = text;
	this->str_base = base;
	this->str_start = start;
	this->str_end = end;
	this->str_name = name;
	this->str_description = desc;
}

/**
 * Retrieve the string with the name of this type of ride.
 * @return The name of this ride type.
 */
StringID RideType::GetTypeName() const
{
	return this->str_name;
}

/**
 * Retrieve the string with the description of this type of ride.
 * @return The description of this ride type.
 */
StringID RideType::GetTypeDescription() const
{
	return this->str_description;
}

/**
 * Get the string instance for the generic ride string of \a number.
 * @param number Generic ride string number to retrieve.
 * @return The instantiated string for this ride type.
 */
StringID RideType::GetString(uint16 number) const
{
	assert(number >= this->str_start && number < this->str_end);
	return this->str_base + (number - this->str_start);
}

#include "table/shops_strings.cpp"

ShopType::ShopType() : RideType(RTK_SHOP)
{
	this->height = 0;
	for (uint i = 0; i < lengthof(this->views); i++) this->views[i] = NULL;
}

ShopType::~ShopType()
{
	/* Images and texts are handled by the sprite collector, no need to release its memory here. */
}

const ImageData *ShopType::GetView(uint8 orientation) const
{
	return (orientation < 4) ? this->views[orientation] : NULL;
}

RideInstance *ShopType::CreateInstance() const
{
	return new ShopInstance(this);
}

/**
 * Can the given value be safely interpreted as item type to sell?
 * @param val Value to inspect.
 * @return The provided value is a valid item type value.
 */
static bool IsValidItemType(uint8 val)
{
	return val == ITP_NOTHING || val == ITP_DRINK || val == ITP_ICE_CREAM || val == ITP_NORMAL_FOOD ||
			val == ITP_SALTY_FOOD || val == ITP_UMBRELLA || val == ITP_PARK_MAP;
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
	uint16 width = rcd_file->GetUInt16(); /// \todo Widths other than 64.
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
	if (!IsValidItemType(val)) return false;
	this->item_type[0] = (ItemType)val;

	val = rcd_file->GetUInt8();
	if (!IsValidItemType(val)) return false;
	this->item_type[1] = (ItemType)val;

	TextData *text_data;
	if (!LoadTextFromFile(rcd_file, texts, &text_data)) return false;
	StringID base = _language.RegisterStrings(*text_data, _shops_strings_table);
	this->SetupStrings(text_data, base, STR_GENERIC_SHOP_START, SHOPS_STRING_TABLE_END, SHOPS_NAME_TYPE, SHOPS_DESCRIPTION_TYPE);
	return true;
}

const StringID *ShopType::GetInstanceNames() const
{
	static const StringID names[] = {SHOPS_NAME_INSTANCE1, SHOPS_NAME_INSTANCE2, STR_INVALID};
	return names;
}

/**
 * Constructor of the base ride instance class.
 * @param rt Type of the ride instance.
 */
RideInstance::RideInstance(const RideType *rt)
{
	this->name[0] = '\0';
	this->type = rt;
	this->state = RIS_ALLOCATED;
	this->flags = 0;
	for (int i = 0; i < NUMBER_ITEM_TYPES_SOLD; i++) this->item_price[i] = 12345; // Arbitrary non-zero amount.
	for (int i = 0; i < NUMBER_ITEM_TYPES_SOLD; i++) this->item_count[i] = 0;
}

RideInstance::~RideInstance()
{
	/* Nothing to do currently. In the future, free instance memory. */
}

/**
 * Get the kind of the ride.
 * @return the kind of the ride.
 */
RideTypeKind RideInstance::GetKind() const
{
	return this->type->kind;
}

/**
 * Get the ride type of the instance.
 * @return The ride type.
 */
const RideType *RideInstance::GetRideType() const
{
	return this->type;
}

/**
 * \fn void RideInstance::GetSprites(uint16 voxel_number, uint8 orient, const ImageData *sprites[4]) const
 * Get the sprites to display for the provided voxel number.
 * @param voxel_number Number of the voxel to draw (copied from the world voxel data).
 * @param orient View orientation.
 * @param sprites [out] Sprites to draw, from back to front, #SO_PLATFORM_BACK, #SO_RIDE, #SO_RIDE_FRONT, and #SO_PLATFORM_FRONT.
 */

/**
 * \fn uint8 RideInstance::GetEntranceDirections(uint16 xvox, uint16 yvox, uint8 zvox) const
 * Get the set edges with an entrance to the ride.
 * @return Bit set of #TileEdge
 */

/**
 * Sell an item to a customer.
 * @param item_index Index of the item being sold.
 */
void RideInstance::SellItem(int item_index)
{
	assert(item_index >= 0 && item_index < NUMBER_ITEM_TYPES_SOLD);

	this->item_count[item_index]++;
	Money profit = this->item_price[item_index] - this->type->item_cost[item_index];
	this->total_sell_profit += profit;
	this->total_profit += profit;

	_finances_manager.PayShopStock(this->type->item_cost[item_index]);
	_finances_manager.EarnShopSales(this->item_price[item_index]);
	NotifyChange(WC_SHOP_MANAGER, this->GetIndex(), CHG_DISPLAY_OLD, 0);
}

/**
 * Get the type of items sold by a ride.
 * @param item_index Index in the items being sold (\c 0 to #NUMBER_ITEM_TYPES_SOLD).
 * @return Type of item sold.
 */
ItemType RideInstance::GetSaleItemType(int item_index) const
{
	assert(item_index >= 0 && item_index < NUMBER_ITEM_TYPES_SOLD);
	return this->type->item_type[item_index];
}

/**
 * Get the price of an item sold by a ride.
 * @param item_index Index in the items being sold (\c 0 to #NUMBER_ITEM_TYPES_SOLD).
 * @return Price to pay for the item.
 */
Money RideInstance::GetSaleItemPrice(int item_index) const
{
	assert(item_index >= 0 && item_index < NUMBER_ITEM_TYPES_SOLD);
	return this->item_price[item_index];
}


/** Monthly update of the shop administration. */
void RideInstance::OnNewMonth()
{
	this->total_profit -= this->type->monthly_cost;
	_finances_manager.PayStaffWages(this->type->monthly_cost);
	SB(this->flags, RIF_MONTHLY_PAID, 1, 1);
	if (this->state == RIS_OPEN) {
		this->total_profit -= this->type->monthly_open_cost;
		_finances_manager.PayStaffWages(this->type->monthly_open_cost);
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
		_finances_manager.PayStaffWages(this->type->monthly_cost);
		SB(this->flags, RIF_MONTHLY_PAID, 1, 1);
		money_paid = true;
	}
	if (GB(this->flags, RIF_OPENED_PAID, 1) == 0) {
		this->total_profit -= this->type->monthly_open_cost;
		_finances_manager.PayStaffWages(this->type->monthly_open_cost);
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
	this->state = RIS_CLOSED;
}

/**
 * Switch the ride to being constructed.
 * @pre The ride should not be open.
 */
void RideInstance::BuildRide()
{
	assert(this->state != RIS_OPEN);
	this->state = RIS_BUILDING;
}

/**
 * Constructor of a shop 'ride'.
 * @param type Kind of shop.
 */
ShopInstance::ShopInstance(const ShopType *type) : RideInstance(type)
{
	this->orientation = 0;
	this->xpos = 0;
	this->ypos = 0;
	this->zpos = 0;
}

ShopInstance::~ShopInstance()
{
}

/**
 * Get the shop type of the ride.
 * @return The shop type of the ride.
 */
const ShopType *ShopInstance::GetShopType() const
{
	assert(this->type->kind == RTK_SHOP);
	return static_cast<const ShopType *>(this->type);
}

void ShopInstance::GetSprites(uint16 voxel_number, uint8 orient, const ImageData *sprites[4]) const
{
	sprites[0] = NULL;
	sprites[1] = this->type->GetView((4 + this->orientation - orient) & 3);
	sprites[2] = NULL;
	sprites[3] = NULL;
}

/**
 * Update a ride instance with its position in the world.
 * @param orientation Orientation of the shop.
 * @param xpos X position of the shop.
 * @param ypos Y position of the shop.
 * @param zpos Z position of the shop.
 */
void ShopInstance::SetRide(uint8 orientation, uint16 xpos, uint16 ypos, uint8 zpos)
{
	assert(this->state == RIS_ALLOCATED);
	assert(_world.GetTileOwner(xpos, ypos) == OWN_PARK); // May only place it in your own park.

	this->orientation = orientation;
	this->xpos = xpos;
	this->ypos = ypos;
	this->zpos = zpos;
	this->flags = 0;
}

uint8 ShopInstance::GetEntranceDirections(uint16 xvox, uint16 yvox, uint8 zvox) const
{
	if (xvox != this->xpos || yvox != this->ypos || zvox != this->zpos) return 0;

	uint8 entrances = this->GetShopType()->flags & SHF_ENTRANCE_BITS;
	return ROL(entrances, 4, this->orientation);
}

/**
 * Can the ride be visited, assuming the shop is approached from direction \a edge?
 * @param edge Direction of entrance (exit direction of the neighbouring voxel).
 * @return The shop can be visited.
 */
bool ShopInstance::CanBeVisited(uint16 xvox, uint16 yvox, uint8 zvox, TileEdge edge) const
{
	if (this->state != RIS_OPEN) return false;
	return GB(this->GetEntranceDirections(xvox, yvox, zvox), (edge + 2) % 4, 1) != 0;
}

/** Default constructor of the rides manager. */
RidesManager::RidesManager()
{
	for (uint i = 0; i < lengthof(this->ride_types); i++) this->ride_types[i] = NULL;
	for (uint i = 0; i < lengthof(this->instances); i++) this->instances[i] = NULL;
}

RidesManager::~RidesManager()
{
	for (uint i = 0; i < lengthof(this->ride_types); i++) delete this->ride_types[i];
	for (uint i = 0; i < lengthof(this->instances); i++) delete this->instances[i];
}

/** A new month has started; perform monthly payments. */
void RidesManager::OnNewMonth()
{
	for (uint16 i = 0; i < lengthof(this->instances); i++) {
		if (this->instances[i] == NULL || this->instances[i]->state == RIS_ALLOCATED) continue;
		this->instances[i]->OnNewMonth();
	}
}

/**
 * Get the requested ride instance.
 * @param num Ride number to retrieve.
 * @return The requested ride, or \c NULL if not available.
 */
RideInstance *RidesManager::GetRideInstance(uint16 num)
{
	assert(num >= SRI_FULL_RIDES && num < SRI_LAST);
	num -= SRI_FULL_RIDES;
	if (num >= lengthof(this->instances)) return NULL;
	return this->instances[num];
}

/**
 * Get the requested ride instance (read-only).
 * @param num Ride instance to retrieve.
 * @return The requested ride, or \c NULL if not available.
 */
const RideInstance *RidesManager::GetRideInstance(uint16 num) const
{
	assert(num >= SRI_FULL_RIDES && num < SRI_LAST);
	num -= SRI_FULL_RIDES;
	if (num >= lengthof(this->instances)) return NULL;
	return this->instances[num];
}

/**
 * Get the ride instance index number.
 * @return Ride instance index.
 */
uint16 RideInstance::GetIndex() const
{
	for (uint i = 0; i < lengthof(_rides_manager.instances); i++) {
		if (_rides_manager.instances[i] == this) return i + SRI_FULL_RIDES;
	}
	NOT_REACHED();
}

/**
 * Add a new ride type to the manager.
 * @param type New ride type to add.
 * @return \c true if the addition was successful, else \c false.
 */
bool RidesManager::AddRideType(RideType *type)
{
	for (uint i = 0; i < lengthof(this->ride_types); i++) {
		if (this->ride_types[i] == NULL) {
			this->ride_types[i] = type;
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
		if (this->instances[i] == NULL) return i + SRI_FULL_RIDES;
	}
	return INVALID_RIDE_INSTANCE;
}

/**
 * Create a new ride instance.
 * @param type Type of ride to construct.
 * @param num Ride number.
 * @return The created ride.
 */
RideInstance *RidesManager::CreateInstance(const RideType *type, uint16 num)
{
	assert(num >= SRI_FULL_RIDES && num < SRI_LAST);
	num -= SRI_FULL_RIDES;
	assert(num < lengthof(this->instances));
	assert(this->instances[num] == NULL);
	this->instances[num] = type->CreateInstance();
	return this->instances[num];
}

/**
 * Check whether a ride exists with the given name.
 * @param name Name of the ride to find.
 * @return The ride with the given name, or \c NULL if no such ride exists.
 */
RideInstance *RidesManager::FindRideByName(const uint8 *name)
{
	for (uint16 i = 0; i < lengthof(this->instances); i++) {
		if (this->instances[i] == NULL || this->instances[i]->state == RIS_ALLOCATED) continue;
		if (StrEqual(name, this->instances[i]->name)) return this->instances[i];
	}
	return NULL;
}

/**
 * A new ride instance was added. Initialize it further.
 * @param num Ride number of the new ride.
 */
void RidesManager::NewInstanceAdded(uint16 num)
{
	RideInstance *ri = this->GetRideInstance(num);
	const RideType *rt = ri->GetRideType();
	assert(ri->state == RIS_ALLOCATED);

	/* Find a new name for the instance. */
	const StringID *names = rt->GetInstanceNames();
	assert(names != NULL && names[0] != STR_INVALID); // Empty array of names loops forever below.
	int idx = 0;
	int shop_num = 1;
	for (;;) {
		if (names[idx] == STR_INVALID) {
			shop_num++;
			idx = 0;
		}

		/* Construct a new name. */
		if (shop_num == 1) {
			DrawText(rt->GetString(names[idx]), ri->name, lengthof(ri->name));
		} else {
			_str_params.SetStrID(1, rt->GetString(names[idx]));
			_str_params.SetNumber(2, shop_num);
			DrawText(GUI_NUMBERED_INSTANCE_NAME, ri->name, lengthof(ri->name));
		}

		if (this->FindRideByName(ri->name) == NULL) break;

		idx++;
	}

	/* Initialize money and counters. */
	ri->total_profit = 0;
	ri->total_sell_profit = 0;
	for (int i = 0; i < NUMBER_ITEM_TYPES_SOLD; i++) {
		ri->item_price[i] = rt->item_cost[i] * 12 / 10; // Make 20% profit.
		ri->item_count[i] = 0;
	}

	switch (ri->GetKind()) {
		case RTK_SHOP:
			ri->CloseRide();
			break;

		case RTK_COASTER:
			ri->BuildRide();
			break;

		default:
			NOT_REACHED(); /// \todo Add other ride types.
	}
}

/**
 * Destroy the indicated instance.
 * @param num The ride number to destroy.
 * @todo The small matter of cleaning up in the world map.
 * @pre Instance must be closed.
 */
void RidesManager::DeleteInstance(uint16 num)
{
	assert(num >= SRI_FULL_RIDES && num < SRI_LAST);
	num -= SRI_FULL_RIDES;
	assert(num < lengthof(this->instances));
	delete this->instances[num];
	this->instances[num] = NULL;
}

/**
 * Check that no rides are under construction at the moment of calling.
 * @note This is just a checking function, perhaps eventually remove it?
 */
void RidesManager::CheckNoAllocatedRides() const
{
	for (uint i = 0; i < lengthof(this->instances); i++) {
		assert(this->instances[i] == NULL || this->instances[i]->state != RIS_ALLOCATED);
	}
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
	if (!IsVoxelstackInsideWorld(xpos, ypos)) return NULL;

	const Voxel *vx = _world.GetVoxel(xpos, ypos, zpos);
	if (vx == NULL || vx->GetInstance() < SRI_FULL_RIDES) {
		/* No ride here, check the voxel below. */
		if (zpos == 0) return NULL;
		vx = _world.GetVoxel(xpos, ypos, zpos - 1);
	}
	if (vx == NULL || vx->GetInstance() < SRI_FULL_RIDES) return NULL;
	return _rides_manager.GetRideInstance(vx->GetInstanceData());
}

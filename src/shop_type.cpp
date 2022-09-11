/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file shop_type.cpp Implementation of the shop 'ride'. */

#include "stdafx.h"
#include "map.h"
#include "shop_type.h"
#include "person.h"
#include "people.h"
#include "fileio.h"

#include "generated/shops_strings.cpp"

static const int TOILET_TIME = 5000;  ///< Duration of visiting the toilet.
static const int CAPACITY_TOILET = 2; ///< Maximum number of guests that can use the toilet at the same time.

ShopType::ShopType() : FixedRideType(RTK_SHOP)
{
	/* Nothing to do currently. */
}

ShopType::~ShopType()
{
	/* Images and texts are handled by the sprite collector, no need to release its memory here. */
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
	switch (val) {
		case ITP_NOTHING:
		case ITP_DRINK:
		case ITP_ICE_CREAM:
		case ITP_NORMAL_FOOD:
		case ITP_SALTY_FOOD:
		case ITP_UMBRELLA:
		case ITP_BALLOON:
		case ITP_PARK_MAP:
		case ITP_SOUVENIR:
		case ITP_MONEY:
		case ITP_TOILET:
		case ITP_FIRST_AID:
			return true;

		default:
			return false;
	}
}

/**
 * Load a type of shop from the RCD file.
 * @param rcd_file Rcd file being loaded.
 * @param sprites Already loaded sprites.
 * @param texts Already loaded texts.
 */
void ShopType::Load(RcdFileReader *rcd_file, [[maybe_unused]] const ImageMap &sprites, const TextMap &texts)
{
	rcd_file->CheckVersion(8);
	int length = rcd_file->size;
	length -= 40;
	rcd_file->CheckMinLength(length, 0, "header");

	this->width_x = this->width_y = 1;
	this->heights.reset(new int8[1]);
	this->heights[0] = rcd_file->GetUInt8();
	this->flags = rcd_file->GetUInt8() & 0xF;

	animation_idle = _sprite_manager.GetFrameSet(ImageSetKey(rcd_file->filename, rcd_file->GetUInt32()));
	if (animation_idle == nullptr || animation_idle->width_x != this->width_x || animation_idle->width_y != this->width_y) {
		rcd_file->Error("Idle animation does not fit");
	}
	for (int i = 0; i < 4; i++) {
		previews[i] = animation_idle->sprites[i][0];
	}
	this->working_duration = 0;       // Shops don't have working phases.
	this->default_idle_duration = 1;  // Ignored.

	for (uint i = 0; i < 3; i++) {
		uint32 recolour = rcd_file->GetUInt32();
		this->recolours.Set(i, RecolourEntry(recolour));
	}
	this->item_cost[0] = rcd_file->GetInt32();
	this->item_cost[1] = rcd_file->GetInt32();
	this->monthly_cost = rcd_file->GetInt32();
	this->monthly_open_cost = rcd_file->GetInt32();

	uint val = rcd_file->GetUInt8();
	if (!IsValidItemType(val)) rcd_file->Error("Invalid 1st item type %u", val);
	this->item_type[0] = (ItemType)val;

	val = rcd_file->GetUInt8();
	if (!IsValidItemType(val)) rcd_file->Error("Invalid 2nd item type %u", val);
	this->item_type[1] = (ItemType)val;

	TextData *text_data;
	LoadTextFromFile(rcd_file, texts, &text_data);
	StringID base = _language.RegisterStrings(*text_data, _shops_strings_table);
	this->SetupStrings(text_data, base, STR_GENERIC_SHOP_START, SHOPS_STRING_TABLE_END, SHOPS_NAME_TYPE, SHOPS_DESCRIPTION_TYPE);

	this->internal_name = rcd_file->GetText();
	this->build_cost = rcd_file->GetUInt32();
	length -= this->internal_name.size() + 1 + 4;

	rcd_file->CheckExactLength(length, 0, "end of block");
}

FixedRideType::RideCapacity ShopType::GetRideCapacity() const
{
	for (int i = 0; i < NUMBER_ITEM_TYPES_SOLD; i++) {
		if (this->item_type[i] == ITP_TOILET) return FixedRideType::RideCapacity{CAPACITY_TOILET, 1};
	}
	return FixedRideType::RideCapacity{0, 0};
}

const StringID *ShopType::GetInstanceNames() const
{
	static const StringID names[] = {SHOPS_NAME_INSTANCE1, SHOPS_NAME_INSTANCE2, STR_INVALID};
	return names;
}

/**
 * Constructor of a shop 'ride'.
 * @param type Kind of shop.
 */
ShopInstance::ShopInstance(const ShopType *type) : FixedRideInstance(type)
{
	this->maintenance_interval = 0;  // Shops don't break down.
}

ShopInstance::~ShopInstance()
{
	/* Nothing to do currently. */
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

/**
 * Update a ride instance with its position in the world.
 * @param orientation Orientation of the shop.
 * @param pos Position of the shop.
 */
void ShopInstance::SetRide(uint8 orientation, const XYZPoint16 &pos)
{
	FixedRideInstance::SetRide(orientation, pos);
	this->flags = 0;
}

uint8 ShopInstance::GetEntranceDirections(const XYZPoint16 &vox) const
{
	if (vox != this->vox_pos) return 0;

	uint8 entrances = this->GetShopType()->flags & SHF_ENTRANCE_BITS;
	return ROL(entrances, 4, this->orientation);
}

bool ShopInstance::CanBeVisited(const XYZPoint16 &vox, TileEdge edge) const
{
	if (!RideInstance::CanBeVisited(vox, edge)) return false;
	return GB(this->GetEntranceDirections(vox), (edge + 2) % 4, 1) != 0;
}

RideEntryResult ShopInstance::EnterRide(int guest, [[maybe_unused]] const XYZPoint16 &vox, TileEdge entry)
{
	assert(vox == this->vox_pos);
	if (this->onride_guests.num_batches == 0) { // No onride guests, handle it all now.
		_guests.GetExisting(guest)->ExitRide(this, entry);
		return RER_DONE;
	}

	/* Guest should wait for the ride to finish, find a spot. */
	int free_batch = this->onride_guests.GetLoadingBatch();
	if (free_batch >= 0) {
		GuestBatch &gb = this->onride_guests.GetBatch(free_batch);
		if (gb.AddGuest(guest, entry)) {
			gb.Start(TOILET_TIME);
			return RER_ENTERED;
		}
	}
	return RER_REFUSED;
}

EdgeCoordinate ShopInstance::GetMechanicEntrance() const
{
	const int dirs = this->GetEntranceDirections(this->vox_pos);
	for (int edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		if ((dirs & (1 << edge)) != 0) return EdgeCoordinate {this->vox_pos, static_cast<TileEdge>(edge)};
	}
	NOT_REACHED();
}

XYZPoint32 ShopInstance::GetExit([[maybe_unused]] int guest, TileEdge entry_edge)
{
	/* Put the guest just outside the ride. */
	Point16 dxy = _exit_dxy[(entry_edge + 2) % 4];
	return XYZPoint32(this->vox_pos.x * 256 + dxy.x, this->vox_pos.y * 256 + dxy.y, this->vox_pos.z * 256);

}

static const uint32 CURRENT_VERSION_ShopInstance = 1;   ///< Currently supported version of %ShopInstance.

void ShopInstance::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("shop");
	if (version != CURRENT_VERSION_ShopInstance) ldr.VersionMismatch(version, CURRENT_VERSION_ShopInstance);
	this->FixedRideInstance::Load(ldr);
	AddRemovePathEdges(this->vox_pos, PATH_EMPTY, this->GetEntranceDirections(this->vox_pos), PAS_QUEUE_PATH);
	ldr.ClosePattern();
}

void ShopInstance::Save(Saver &svr)
{
	svr.StartPattern("shop", CURRENT_VERSION_ShopInstance);
	this->FixedRideInstance::Save(svr);
	/* Nothing shop-specific to do here currently. */
	svr.EndPattern();
}

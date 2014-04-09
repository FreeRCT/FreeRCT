/* $Id$ */

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
#include "fileio.h"

#include "generated/shops_strings.cpp"

ShopType::ShopType() : RideType(RTK_SHOP)
{
	this->height = 0;
	std::fill_n(this->views, lengthof(this->views), nullptr);
}

ShopType::~ShopType()
{
	/* Images and texts are handled by the sprite collector, no need to release its memory here. */
}

const ImageData *ShopType::GetView(uint8 orientation) const
{
	return (orientation < 4) ? this->views[orientation] : nullptr;
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
 * @param sprites Already loaded sprites.
 * @param texts Already loaded texts.
 * @return Loading was successful.
 */
bool ShopType::Load(RcdFileReader *rcd_file, const ImageMap &sprites, const TextMap &texts)
{
	if (rcd_file->version != 4 || rcd_file->size != 2 + 1 + 1 + 4 * 4 + 3 * 4 + 4 * 4 + 2 + 4) return false;
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
	sprites[0] = nullptr;
	sprites[1] = this->type->GetView((4 + this->orientation - orient) & 3);
	sprites[2] = nullptr;
	sprites[3] = nullptr;
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

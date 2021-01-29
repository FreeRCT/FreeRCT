/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gentle_thrill_type.cpp Implementation of gentle rides and thrill rides. */

#include "stdafx.h"
#include "map.h"
#include "shop_type.h"
#include "person.h"
#include "people.h"
#include "fileio.h"
#include "gentle_thrill_ride_type.h"
#include "generated/gentle_thrill_rides_strings.cpp"

GentleThrillRideType::GentleThrillRideType() : FixedRideType(RTK_GENTLE /* Kind will be set later in Load(). */)
{
	/* Nothing to do currently. */
}

GentleThrillRideType::~GentleThrillRideType()
{
	/* Images and texts are handled by the sprite collector, no need to release its memory here. */
}

RideInstance *GentleThrillRideType::CreateInstance() const
{
	return new GentleThrillRideInstance(this);
}

/**
 * Load a type of gentle or thrill ride from the RCD file.
 * @param rcd_file Rcd file being loaded.
 * @param sprites Already loaded sprites.
 * @param texts Already loaded texts.
 * @return Loading was successful.
 * @todo #GentleThrillRideType::Load and #ShopType::Load share a lot of similar code. Pull it out into a common function in #FixedRideType.
 */
bool GentleThrillRideType::Load(RcdFileReader *rcd_file, const ImageMap &sprites, const TextMap &texts)
{
	if (rcd_file->version != 1 || rcd_file->size < 9) return false;
	this->kind = rcd_file->GetUInt8() ? RTK_THRILL : RTK_GENTLE;
	const uint16 width = rcd_file->GetUInt16(); /// \todo Widths other than 64.
	this->width_x = rcd_file->GetUInt8();
	this->width_y = rcd_file->GetUInt8();
	this->animation_phases = rcd_file->GetUInt32();
	if (this->width_x < 1 || this->width_y < 1) return false;
	if (rcd_file->size !=
			53 + (this->width_x * this->width_y) +
			16 * (this->width_x * this->width_y * (this->animation_phases + 1))) {
		return false;
	}
	
	this->heights.reset(new int8[this->width_x * this->width_y]);
	for (int8 x = 0; x < this->width_x; ++x) {
		for (int8 y = 0; y < this->width_y; ++y) {
			this->heights[x * this->width_y + y] = rcd_file->GetUInt8();
		}
	}

	for (int i = 0; i < 4; i++) {
		this->views[i].reset(new ImageData*[this->width_x * this->width_y * (this->animation_phases + 1)]);
		for (int x = 0; x < this->width_x; ++x) {
			for (int y = 0; y < this->width_y; ++y) {
				for (unsigned a = 0; a <= this->animation_phases; ++a) {
					ImageData *view;
					if (!LoadSpriteFromFile(rcd_file, sprites, &view)) return false;
					if (width != 64) continue; // Silently discard other sizes.
					this->views[i][(a * width_x * width_y) + (x * width_y) + y] = view;
				}
			}
		}
	}
	for (int i = 0; i < 4; i++) {
		ImageData *view;
		if (!LoadSpriteFromFile(rcd_file, sprites, &view)) return false;
		if (width != 64) continue; // Silently discard other sizes.
		this->previews[i] = view;
	}

	for (uint i = 0; i < 3; i++) {
		uint32 recolour = rcd_file->GetUInt32();
		this->recolours.Set(i, RecolourEntry(recolour));
	}
	this->item_cost[0] = rcd_file->GetInt32(); // Entrance fee.
	this->item_cost[1] = 0;                    // Unused.
	this->monthly_cost = rcd_file->GetInt32();
	this->monthly_open_cost = rcd_file->GetInt32();

	TextData *text_data;
	if (!LoadTextFromFile(rcd_file, texts, &text_data)) return false;
	StringID base = _language.RegisterStrings(*text_data, _gentle_thrill_rides_strings_table);
	this->SetupStrings(text_data, base,
			STR_GENERIC_GENTLE_THRILL_RIDES_START, GENTLE_THRILL_RIDES_STRING_TABLE_END,
			GENTLE_THRILL_RIDES_NAME_TYPE, GENTLE_THRILL_RIDES_DESCRIPTION_TYPE);
	return true;
}

int GentleThrillRideType::GetRideCapacity() const
{
	return 0; // \todo Guest entering rides is not implemented yet.
}

const StringID *GentleThrillRideType::GetInstanceNames() const
{
	static const StringID names[] = {GENTLE_THRILL_RIDES_NAME_INSTANCE1, GENTLE_THRILL_RIDES_NAME_INSTANCE2, STR_INVALID};
	return names;
}

/**
 * Constructor of a gentle or thrill ride.
 * @param type Kind of ride.
 */
GentleThrillRideInstance::GentleThrillRideInstance(const GentleThrillRideType *type) : FixedRideInstance(type)
{
	/* Nothing to do currently. */
}

GentleThrillRideInstance::~GentleThrillRideInstance()
{
	/* Nothing to do currently. */
}

/**
 * Get the ride type of the ride.
 * @return The gentle or thrill ride type of the ride.
 */
const GentleThrillRideType *GentleThrillRideInstance::GetGentleThrillRideType() const
{
	assert(this->type->kind == RTK_GENTLE || this->type->kind == RTK_THRILL);
	return static_cast<const GentleThrillRideType *>(this->type);
}

uint8 GentleThrillRideInstance::GetEntranceDirections(const XYZPoint16 &vox) const
{
	return SHF_ENTRANCE_BITS; // \todo Ride entrances are not implemented yet.
}

RideEntryResult GentleThrillRideInstance::EnterRide(int guest, TileEdge entry)
{
	return RER_REFUSED; // \todo Ride entrances are not implemented yet.
}

XYZPoint32 GentleThrillRideInstance::GetExit(int guest, TileEdge entry_edge)
{
	NOT_REACHED(); // \todo Ride exits are not implemented yet.
}

void GentleThrillRideInstance::Load(Loader &ldr)
{
	this->FixedRideInstance::Load(ldr);
	/* Nothing to do here currently. */
}

void GentleThrillRideInstance::Save(Saver &svr)
{
	this->FixedRideInstance::Save(svr);
	/* Nothing to do here currently. */
}

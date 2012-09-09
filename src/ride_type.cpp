/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ride_type.cpp Ride type storage and retrieval. */

#include "stdafx.h"
#include "sprite_store.h"
#include "language.h"
#include "sprite_store.h"
#include "fileio.h"
#include "ride_type.h"

RideTypeManager _ride_type_manager; ///< Storage and retrieval of ride types.

#include "table/shops_strings.cpp"

ShopType::ShopType()
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
	if (length != 2 + 2 + 4*4 + 3*4 + 4) return false;
	uint16 width = rcd_file->GetUInt16(); // XXX Widths other than 64
	uint16 height = rcd_file->GetUInt16();
	if (height > 32) return false; // Arbitrary sane limit check.

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

RideTypeManager::RideTypeManager()
{
	for (uint i = 0; i < lengthof(this->shops); i++) this->shops[i] = NULL;
}

RideTypeManager::~RideTypeManager()
{
	for (uint i = 0; i < lengthof(this->shops); i++) {
		if (this->shops[i] != NULL) delete this->shops[i];
	}
}


/** Add a new ride type to the manager. */
bool RideTypeManager::Add(ShopType *shop_type)
{
	for (uint i = 0; i < lengthof(this->shops); i++) {
		if (this->shops[i] == NULL) {
			this->shops[i] = shop_type;
			return true;
		}
	}
	return false;
}

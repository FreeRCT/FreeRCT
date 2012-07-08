/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file person.cpp Person-related functions. */

#include "stdafx.h"
#include "random.h"
#include "enum_type.h"
#include "person_type.h"
#include "fileio.h"

static PersonTypeData _person_type_datas[PERSON_TYPE_COUNT]; ///< Data about each type of person.

/** Default constructor of the random recolour mapping. */
RandomRecolouringMapping::RandomRecolouringMapping() : range_number(COL_RANGE_INVALID), dest_set(0)
{
}

/**
 * Decide a colour for the random colour range.
 * @param rnd %Random number generator.
 * @return Colour range to use for #range_number.
 */
ColourRange RandomRecolouringMapping::DrawRandomColour(Random *rnd) const
{
	assert(this->range_number != COL_RANGE_INVALID);

	int ranges[COL_RANGE_COUNT];
	int count = 0;
	uint32 bit = 1;
	for (int i = 0; i < (int)lengthof(ranges); i++) {
		if ((bit & this->dest_set) != 0) ranges[count++] = i;
		bit <<= 1;
	}
	if (count == 0) return static_cast<ColourRange>(this->range_number); // No range to replace the original colour with.
	if (count == 1) return static_cast<ColourRange>(ranges[0]); // Just one colour, easy choice.
	return static_cast<ColourRange>(ranges[rnd->Uniform(count - 1)]);
}

/**
 * Construct a recolour mapping of this person type.
 * @param rnd %Random number generator.
 * @return The constructed recolouring.
 */
Recolouring PersonTypeGraphics::MakeRecolouring(Random *rnd) const
{
	Recolouring recolour;
	for (int i = 0; i < (int)lengthof(this->random_recolours); i++) {
		const RandomRecolouringMapping &rrcm = this->random_recolours[i];
		if (rrcm.range_number == COL_RANGE_INVALID) continue;
		recolour.SetRecolouring(static_cast<ColourRange>(rrcm.range_number), rrcm.DrawRandomColour(rnd));
	}
	return recolour;
}

/**
 * Get the data about a person type with the intention to change it.
 * @param pt Type of person queried.
 * @return Reference to the data of the queried person type.
 * @note Use #GetPersonTypeData if the data is only read.
 */
PersonTypeData &ModifyPersonTypeData(PersonType pt)
{
	assert(pt < lengthof(_person_type_datas));
	return _person_type_datas[pt];
}

/**
 * Load graphics settings of person types from an RCD file.
 * @param rcd_file RCD file to read, pointing at the start of the PRSG block data (behind the header information).
 * @param length Length of the remaining block data.
 * @return Loading was a success.
 */
bool LoadPRSG(RcdFile *rcd_file, uint32 length)
{
	if (length < 1) return false;
	uint8 count = rcd_file->GetUInt8();
	length--;

	if (length != 13 * count) return false;
	while (count > 0) {
		uint8 ps = rcd_file->GetUInt8();
		uint32 rc1 = rcd_file->GetUInt32();
		uint32 rc2 = rcd_file->GetUInt32();
		uint32 rc3 = rcd_file->GetUInt32();

		PersonType pt;
		switch (ps) {
			case  8: pt = PERSON_PILLAR;  break;
			case 16: pt = PERSON_EARTH;   break;
			default: pt = PERSON_INVALID; break;
		}

		if (pt != PERSON_INVALID) {
			PersonTypeData &ptd = ModifyPersonTypeData(pt);
			assert(NUMBER_PERSON_TYPE_RECOLOURINGS >= 3);
			ptd.graphics.random_recolours[0].Set(rc1);
			ptd.graphics.random_recolours[2].Set(rc2);
			ptd.graphics.random_recolours[3].Set(rc3);
		}
		count--;
	}
	return true;
}

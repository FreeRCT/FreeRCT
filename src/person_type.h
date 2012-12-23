/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file person_type.h Declaration of person types. */

#ifndef PERSON_TYPE_H
#define PERSON_TYPE_H

#include "palette.h"

class Random;
class RcdFile;

static const int NUMBER_PERSON_TYPE_RECOLOURINGS = 3; ///< Number of recolouring mapping of a person type.

/** Types of persons. */
enum PersonType {
	PERSON_ANY    = 0, ///< No people displayed in the animation.
	PERSON_PILLAR = 1, ///< %Guests from the planet of Pillars (test graphics).
	PERSON_EARTH  = 2, ///< Earth-bound guests.
	PERSON_TYPE_COUNT, ///< Number of known types of persons.

	PERSON_INVALID = 0xFF, ///< Invalid person type.

	PERSON_MIN_GUEST = PERSON_ANY,   ///< First value of a guest.
	PERSON_MAX_GUEST = PERSON_EARTH, ///< Last value of a guest.
};
DECLARE_POSTFIX_INCREMENT(PersonType)


/** Graphics definition of a person type. */
struct PersonTypeGraphics {
	RandomRecolouringMapping random_recolours[NUMBER_PERSON_TYPE_RECOLOURINGS]; ///< Random colour remappings.

	Recolouring MakeRecolouring(Random *rnd) const;
};

/** Collection of data for each person type. */
struct PersonTypeData {
	PersonTypeGraphics graphics; ///< Graphics definitions.
};

/**
 * Get the data about a person type with the intention to change it.
 * @param pt Type of person queried.
 * @return Reference to the data of the queried person type.
 */
PersonTypeData &ModifyPersonTypeData(PersonType pt);

/**
 * Get the data about a person type for reading.
 * @param pt Type of person queried.
 * @return Reference to the data of the queried person type.
 */
inline const PersonTypeData &GetPersonTypeData(PersonType pt)
{
	return ModifyPersonTypeData(pt);
}


/**
 * Function to decide whether a person type is a guest or not.
 * @param type %Person type to test.
 * @return Whether the given person type is a guest.
 */
static inline bool PersonIsAGuest(uint8 type)
{
	return type >= PERSON_MIN_GUEST && type <= PERSON_MAX_GUEST;
}

bool LoadPRSG(RcdFile *rcd_file, uint32 length);

#endif


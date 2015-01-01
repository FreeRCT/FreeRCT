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
class RcdFileReader;

/** Types of persons. */
enum PersonType {
	PERSON_ANY,         ///< No people displayed in the animation.
	PERSON_GUEST,       ///< %Guests.
	PERSON_HANDYMAN,    ///< %Staff handymen.
	PERSON_MECHANIC,    ///< %Staff mechanics.
	PERSON_GUARD,       ///< %Staff security guards.
	PERSON_ENTERTAINER, ///< %Staff entertainers.
	PERSON_TYPE_COUNT,  ///< Number of known types of persons.

	PERSON_INVALID = 0xFF, ///< Invalid person type.
};
DECLARE_POSTFIX_INCREMENT(PersonType)

/** Graphics definition of a person type. */
struct PersonTypeGraphics {
	Recolouring recolours; ///< Random colour remapping.

	Recolouring MakeRecolouring() const;
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

bool LoadPRSG(RcdFileReader *rcd_file);

#endif

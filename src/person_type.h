/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file people_type.h Declaration of person types. */

#ifndef PERSON_TYPE_H
#define PERSON_TYPE_H

/** Types of persons. */
enum PersonType {
	PERSON_ANY    = 0, ///< No people displayed in the animation.
	PERSON_PILLAR = 1, ///< Guests from the planet of Pillars (test graphics).
	PERSON_EARTH  = 2, ///< Earth-bound guests.

	PERSON_INVALID = 0xFF, ///< Invalid person type.

	PERSON_MIN_GUEST = PERSON_ANY,   ///< First value of a guest.
	PERSON_MAX_GUEST = PERSON_EARTH, ///< Last value of a guest.
};

/**
 * Function to decide whehter a person type is a guest or not.
 * @param type Person type to test.
 * @return Whether the given person type is a guest.
 */
static inline bool PersonIsAGuest(uint8 type)
{
	return type >= PERSON_MIN_GUEST && type <= PERSON_MAX_GUEST;
}

#endif


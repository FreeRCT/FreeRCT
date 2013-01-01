/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file people.h Declarations for people in the world. */

#ifndef PEOPLE_H
#define PEOPLE_H

static const uint GUEST_BLOCK_SIZE = 512; ///< Number of guests in a block.

/** A block of guests. */
class GuestBlock {
public:
	GuestBlock(uint16 base_id);

	void AddAll(PersonList *pl);

	/**
	 * Get a guest from the array.
	 * @param i Index of the person (should be between \c 0 and #GUEST_BLOCK_SIZE).
	 * @return The requested person.
	 */
	inline Guest *Get(uint i) {
		assert(i < lengthof(this->guests));
		return &this->guests[i];
	}

protected:
	Guest guests[GUEST_BLOCK_SIZE]; ///< Persons in the block.
};

/**
 * All our guests.
 * @todo Allow to have several blocks of guests.
 */
class Guests {
public:
	Guests();
	~Guests();

	void Initialize();
	bool CanUsePersonType(PersonType ptype);
	uint CountActiveGuests();

	void OnAnimate(int delay);
	void DoTick();
	void OnNewDay();

private:
	GuestBlock block;     ///< The data of all actual guests.
	PersonList free;      ///< Non-active persons.
	Random rnd;           ///< Random number generator for creating new guests.
	Point16 start_voxel;  ///< Entry x/y coordinate of the voxel stack at the edge.
	int daily_frac;       ///< Frame counter.
	int next_daily_index; ///< Index of the next guest to give daily service.

	uint16 valid_ptypes;  ///< Person types that can be used.
};
assert_compile(PERSON_MAX_GUEST - PERSON_MIN_GUEST + 1 <= 16); ///< Verify that all person types fit in #Guests::valid_ptypes

extern Guests _guests;

#endif


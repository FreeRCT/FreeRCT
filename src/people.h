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

#include "person_type.h"
#include "random.h"

static const uint GUEST_BLOCK_SIZE = 512; ///< Number of guests in a block.

/** Data of a guest. */
struct GuestData {
	int16 happiness; ///< Happiness of the guest (values are 0-100).
};

/**
 * Common base class of a person in the world.
 *
 * Persons are stored in contiguous blocks of memory, which makes the constructor and destructor useless.
 * Instead, \c Activate and \c DeActivate methods are used for this purpose. The #type variable controls the state of the entry.
 */
class Person {
public:
	Person();
	virtual ~Person();

	bool OnAnimate(int delay);
	bool DailyUpdate();

	void Activate(const Point16 &start, uint8 person_type);
	void DeActivate();

	void SetName(const char *name);
	const char *GetName() const;

	uint16 id;  ///< Unique id of the person.
	/**
	 * Type of person.
	 * #PERSON_INVALID means the entry is not used.
	 * @see PersonType
	 */
	uint8 type;

	Person *next; ///< Next person in the linked list.
	Person *prev; ///< Previous person in the linked list.

protected:
	Random rnd; ///< Random number generator for deciding how the person reacts.
	char *name; ///< Name of the person. \c NULL means it has a default name (like "Guest XYZ").

	union {
		GuestData guest; ///< Guest data (only valid if #PersonIsAGuest holds for #type).
	} u; ///< Person-type specific data.
};

/** Manager of a doubly linked list of persons. */
class PersonList {
public:
	PersonList();
	~PersonList();

	bool IsEmpty();

	void AddFirst(Person *p);
	void Remove(Person *p);
	Person *RemoveHead();

protected:
	Person *first; ///< First person in the list.
	Person *last;  ///< Last person in the list.
};


/** A block of guests. */
class GuestBlock {
public:
	GuestBlock(uint16 base_id);

	void AddAll(PersonList *pl);

	/**
	 * Get a person from the array.
	 * @param i Index of the person (should be between \c 0 and #GUEST_BLOCK_SIZE).
	 * @return The requested person.
	 */
	inline Person *Get(uint i) {
		assert(i < lengthof(this->persons));
		return &this->persons[i];
	}

protected:
	Person persons[GUEST_BLOCK_SIZE];
};

/**
 * All our guests.
 * @todo Allow to have several blocks of guests.
 */
class Guests {
public:
	Guests();
	~Guests();

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
};

extern Guests _guests;

#endif


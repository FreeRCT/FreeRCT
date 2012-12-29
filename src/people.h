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
#include "money.h"

static const uint GUEST_BLOCK_SIZE = 512; ///< Number of guests in a block.

 /**
 * Limits that exist at the tile.
 *
 * There are four limits in X direction (NE of tile, low x limit, high x limit, and SW of tile),
 * and four limits in Y direction (NW of tile, low y limit, high y limit, and SE of tile). Low and
 * high is created by means of a random offset from the center, to prevent all guests from walking
 * at a single line.
 *
 * Since you can walk the tile in two directions (incrementing x/y or decrementing x/y), the middle
 * limits have a below/above notion as well.
 */
enum WalkLimit {
	WLM_MINIMAL = 0, ///< Continue until reached minimal value.
	WLM_LOW     = 1, ///< Continue until reached low value.
	WLM_CENTER  = 2, ///< Continue until reached center value.
	WLM_HIGH    = 3, ///< Continue until reached high value.
	WLM_MAXIMAL = 4, ///< Continue until reached maximal value.
	WLM_INVALID = 7, ///< Invalid limit.

	WLM_LIMIT_LENGTH = 3, ///< Length of the limits in bits.

	WLM_X_START   = 0, ///< Destination position of X axis.
	WLM_Y_START   = 3, ///< Destination position of Y axis.
	WLM_END_LIMIT = 6, ///< Bit deciding which axis is the end-condition (\c 0 means #WLM_X_START, \c 1 means #WLM_Y_START).

	WLM_X_COND = 0,                  ///< X limit decides the end of this walk.
	WLM_Y_COND = 1 << WLM_END_LIMIT, ///< Y limit decides the end of this walk.

	WLM_NE_EDGE = WLM_MINIMAL | (WLM_INVALID << WLM_Y_START) | WLM_X_COND, ///< Continue until the north-east edge.
	WLM_LOW_X   = WLM_LOW     | (WLM_INVALID << WLM_Y_START) | WLM_X_COND, ///< Continue until we at the low x limit of the tile.
	WLM_MID_X   = WLM_CENTER  | (WLM_INVALID << WLM_Y_START) | WLM_X_COND, ///< Continue until we at the center x limit of the tile.
	WLM_HIGH_X  = WLM_HIGH    | (WLM_INVALID << WLM_Y_START) | WLM_X_COND, ///< Continue until we at the high x limit of the tile.
	WLM_SW_EDGE = WLM_MAXIMAL | (WLM_INVALID << WLM_Y_START) | WLM_X_COND, ///< Continue until the south-west edge.

	WLM_NW_EDGE = WLM_INVALID | (WLM_MINIMAL << WLM_Y_START) | WLM_Y_COND, ///< Continue until the north-west edge.
	WLM_LOW_Y   = WLM_INVALID | (WLM_LOW     << WLM_Y_START) | WLM_Y_COND, ///< Continue until we at the low y limit of the tile.
	WLM_MID_Y   = WLM_INVALID | (WLM_CENTER  << WLM_Y_START) | WLM_Y_COND, ///< Continue until we at the center y limit of the tile.
	WLM_HIGH_Y  = WLM_INVALID | (WLM_HIGH    << WLM_Y_START) | WLM_Y_COND, ///< Continue until we at the high y limit of the tile.
	WLM_SE_EDGE = WLM_INVALID | (WLM_MAXIMAL << WLM_Y_START) | WLM_Y_COND, ///< Continue until the south-east edge.
};

/** Walk animation to use to walk a part of the tile. */
struct WalkInformation {
	AnimationType anim_type; ///< Animation to display.
	uint8 limit_type;        ///< Limit to end use of this animation. @see WalkLimit
};

/** Exit codes of the Person::OnAnimate call. */
enum AnimateResult {
	OAR_OK,         ///< All ok, keep running.
	OAR_REMOVE,     ///< Remove person from the person-list, and de-activate.
	OAR_DEACTIVATE, ///< Person is already removed from the person-list, only de-activate.
};

/**
 * Class of a person in the world.
 *
 * Persons are stored in contiguous blocks of memory, which makes the constructor and destructor useless.
 * Instead, \c Activate and \c DeActivate methods are used for this purpose. The #type variable controls the state of the entry.
 */
class Person {
public:
	Person();
	virtual ~Person();

	AnimateResult OnAnimate(int delay);
	virtual bool DailyUpdate() = 0;

	virtual void Activate(const Point16 &start, PersonType person_type);
	virtual void DeActivate(AnimateResult ar);

	void SetName(const char *name);
	const char *GetName() const;

	uint16 id;       ///< Unique id of the person.
	PersonType type; ///< Type of person.

	Person *next; ///< Next person in the linked list.
	Person *prev; ///< Previous person in the linked list.

	int16 x_vox;  ///< %Voxel index in X direction of the person (if #type is not invalid).
	int16 y_vox;  ///< %Voxel index in Y direction of the person (if #type is not invalid).
	int16 z_vox;  ///< %Voxel index in Z direction of the person (if #type is not invalid).
	int16 x_pos;  ///< X position of the person inside the voxel (0..255).
	int16 y_pos;  ///< Y position of the person inside the voxel (0..255).
	int16 z_pos;  ///< Z position of the person inside the voxel (0..255).
	int16 offset; ///< Offset with respect to center of paths walked on (0..100).

	const WalkInformation *walk;  ///< Walk animation sequence being performed.
	const AnimationFrame *frames; ///< Animation frames of the current animation.
	uint16 frame_count;           ///< Number of frames in #frames.
	uint16 frame_index;           ///< Currently displayed frame of #frames.
	int16 frame_time;             ///< Remaining display time of this frame.
	Recolouring recolour;         ///< Person recolouring.

protected:
	Random rnd; ///< Random number generator for deciding how the person reacts.
	char *name; ///< Name of the person. \c NULL means it has a default name (like "Guest XYZ").

	void DecideMoveDirection();
	void WalkTheWalk(const WalkInformation *walk);
	void MarkDirty();
};

/** Guests walking around in the world. */
class Guest : public Person {
public:
	Guest();
	~Guest();

	/* virtual */ void Activate(const Point16 &start, PersonType person_type);

	/* virtual */ bool DailyUpdate();

	int16 happiness; ///< Happiness of the guest (values are 0-100).
	Money cash;      ///< Amount of money carried by the guest (should be non-negative).
};

/** Manager of a doubly linked list of persons. */
class PersonList {
	friend void CopyPersonList(PersonList &dest, const PersonList &src);
public:
	PersonList();
	~PersonList();

	bool IsEmpty() const;

	void AddFirst(Person *p);
	void Remove(Person *p);
	Person *RemoveHead();

	Person *first; ///< First person in the list.
	Person *last;  ///< Last person in the list.
};

void CopyPersonList(PersonList &dest, const PersonList &src);


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


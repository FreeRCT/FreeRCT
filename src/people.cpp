/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file people.cpp People in the world. */

#include "stdafx.h"
#include "map.h"
#include "dates.h"
#include "people.h"
#include "math_func.h"

Guests _guests; ///< Guests in the world/park.

Person::Person() : rnd()
{
	this->type = PERSON_INVALID;
	this->name = NULL;
	this->next = NULL;
	this->prev = NULL;
}

Person::~Person()
{
	free(this->name);
}

/**
 * Set the name of a guest.
 * @param name New name of the guest.
 * @note Currently unused.
 */
void Person::SetName(const char *name)
{
	assert(PersonIsAGuest(this->type));

	int length = strlen(name);
	this->name = (char *)malloc(length + 1);
	memcpy(this->name, name, length);
	this->name[length] = '\0';
}

/**
 * Query the name of the person.
 * The name is returned in memory owned by the person. Do not free this data. It may change on each call.
 * @return Static buffer containing the name of the person.
 * @note Currently not used.
 */
const char *Person::GetName() const
{
	static char buffer[16];

	assert(PersonIsAGuest(this->type));
	if (this->name != NULL) return this->name;
	sprintf(buffer, "guest %u", this->id);
	return buffer;
}

/**
 * Mark this person as 'in use'.
 * @param start Start x/y voxel stack for entering the world.
 * @param person_type Type of person getting activated.
 */
void Person::Activate(const Point16 &start, PersonType person_type)
{
	assert(this->type == PERSON_INVALID);
	assert(person_type != PERSON_INVALID);

	this->type = person_type;
	this->name = NULL;

	if (PersonIsAGuest(this->type)) {
		this->u.guest.happiness = 50 + this->rnd.Uniform(50);
	}

}

/** Mark this person as 'not in use'. (Called by #Guests.) */
void Person::DeActivate()
{
	if (this->type == PERSON_INVALID) return;

	this->type = PERSON_INVALID;
	free(this->name);
	this->name = NULL;
}

/**
 * Update the animation of the person.
 * @param delay Amount of milli seconds since the last update.
 * @return If \c false, de-activate the person.
 * @todo [high] Function is empty, make it so something useful instead.
 */
bool Person::OnAnimate(int delay)
{
	return true;
}

/**
 * Daily ponderings of a person.
 * @return If \c false, de-activate the person.
 * @todo Make de-activation a bit more random.
 */
bool Person::DailyUpdate()
{
	if (!PersonIsAGuest(this->type)) return false;

	this->u.guest.happiness = max(0, this->u.guest.happiness - 2);
	return this->u.guest.happiness > 10; // De-activate if at or below 10.
}


/** Default constructor of the person list. */
PersonList::PersonList()
{
	this->first = NULL;
	this->last = NULL;
}

/** Destructor of the person list. */
PersonList::~PersonList()
{
	// Silently let go of the persons in the list.
}

/**
 * Are there any persons in the list?
 * @return \c true iff at least one person is in the list.
 */
bool PersonList::IsEmpty()
{
	return this->first == NULL;
}

/**
 * Add person to the head of the list.
 * @param p %Person to add.
 */
void PersonList::AddFirst(Person *p)
{
	p->prev = NULL;
	p->next = this->first;
	if (this->first == NULL) {
		this->last = p;
		this->first = p;
	} else {
		this->first->prev = p;
		this->first = p;
	}
}

/**
 * Remove a person from the list.
 * @param p %Person to remove.
 */
void PersonList::Remove(Person *p)
{
	if (p->prev == NULL) {
		assert(this->first == p);
		this->first = p->next;
	} else {
		p->prev->next = p->next;
	}

	if (p->next == NULL) {
		assert(this->last == p);
		this->last = p->prev;
	} else {
		p->next->prev = p->prev;
	}
}

/**
 * Get the person from the head of the list.
 * @return %Person at the head of the list.
 */
Person *PersonList::RemoveHead()
{
	Person *p = this->first;
	assert(p != NULL);
	this->Remove(p);
	return p;
}

GuestBlock::GuestBlock(uint16 base_id)
{
	for (uint i = 0; i < lengthof(this->persons); i++) {
		this->persons[i].id = base_id;
		base_id++;
	}
}

/**
 * Add all persons to the provided list.
 * @param pl %Person list to add the persons to.
 */
void GuestBlock::AddAll(PersonList *pl)
{
	/* Add in reverse order so the first retrieval is the first person in this block. */
	uint i = lengthof(this->persons) - 1;
	for (;;) {
		pl->AddFirst(this->Get(i));
		if (i == 0) break;
		i--;
	}
}


/**
 * Check that the voxel stack at the given coordinate is a good spot to use as entry point for new guests.
 * @param x X position at the edge.
 * @param y Y position at the edge.
 * @return The given position is a good starting point.
 */
static bool IsGoodEdgeRoad(int16 x, int16 y)
{
	if (x < 0 || y < 0) return false;
	int16 z = _world.GetGroundHeight(x, y);
	const Voxel *vs = _world.GetVoxel(x, y, z);
	const SurfaceVoxelData *svd = vs->GetSurface();
	return svd->path.type == PT_CONCRETE && svd->path.slope < PATH_FLAT_COUNT;
}

/**
 * Try to find a voxel at the edge of the world that can be used as entry point for guests.
 * @return The x/y coordinate at the edge of the world of a usable voxelstack, else an off-world coordinate.
 */
static Point16 FindEdgeRoad()
{
	Point16 pt;

	int16 highest_x = _world.GetXSize() - 1;
	int16 highest_y = _world.GetYSize() - 1;
	for (int x = 1; x < highest_x; x++) {
		if (IsGoodEdgeRoad(x, 0)) {
			pt.x = x;
			pt.y = 0;
			return pt;
		}
		if (IsGoodEdgeRoad(x, highest_y)) {
			pt.x = x;
			pt.y = highest_y;
			return pt;
		}
	}
	for (int y = 1; y < highest_y; y++) {
		if (IsGoodEdgeRoad(0, y)) {
			pt.x = 0;
			pt.y = y;
			return pt;
		}
		if (IsGoodEdgeRoad(highest_x, y)) {
			pt.x = highest_x;
			pt.y = y;
			return pt;
		}
	}

	pt.x = -1;
	pt.y = -1;
	return pt;
}


Guests::Guests() : block(0), rnd()
{
	this->block.AddAll(&this->free);

	this->start_voxel.x = -1;
	this->start_voxel.y = -1;
	this->daily_frac = 0;
	this->next_daily_index = 0;
}

Guests::~Guests()
{
}

/**
 * Some time has passed, update the animation.
 * @param delay Number of milli seconds time that have past since the last animation update.
 */
void Guests::OnAnimate(int delay)
{
	for (uint i = 0; i < GUEST_BLOCK_SIZE; i++) {
		Person *p = this->block.Get(i);
		if (p->type == PERSON_INVALID) continue;

		if (!p->OnAnimate(delay)) {
			p->DeActivate();
			this->free.AddFirst(p);
		}
	}
}

/** A new frame arrived, perform the daily call for some of the guests. */
void Guests::DoTick()
{
	this->daily_frac++;
	int end_index = min(this->daily_frac * GUEST_BLOCK_SIZE / TICK_COUNT_PER_DAY, GUEST_BLOCK_SIZE);
	while (this->next_daily_index < end_index) {
		Person *p = this->block.Get(this->next_daily_index);
		if (p->type != PERSON_INVALID && !p->DailyUpdate()) {
			printf("Guest %d is leaving\n", p->id);
			p->DeActivate();
			this->free.AddFirst(p);
		}
		this->next_daily_index++;
	}
	if (this->next_daily_index >= (int)GUEST_BLOCK_SIZE) {
		this->daily_frac = 0;
		this->next_daily_index = 0;
	}
}

/**
 * A new day arrived, handle daily chores of the park.
 * @todo Chance of spawning a new guest should depends on popularity (and be configurable for a level).
 * @todo Person type should be configurable too.
 */
void Guests::OnNewDay()
{
	if (this->rnd.Success1024(512)) {
		if (!IsGoodEdgeRoad(this->start_voxel.x, this->start_voxel.y)) {
			printf("New guest, but no road\n");
			this->start_voxel = FindEdgeRoad();
			if (!IsGoodEdgeRoad(this->start_voxel.x, this->start_voxel.y)) return;
		}

		if (this->free.IsEmpty()) return; // No more quests available.
		printf("New guest!\n");
		Person *p = this->free.RemoveHead();
		if (p != NULL) p->Activate(this->start_voxel, PERSON_PILLAR);
	}
}


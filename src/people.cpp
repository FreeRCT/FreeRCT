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
#include "math_func.h"
#include "geometry.h"
#include "person_type.h"
#include "ride_type.h"
#include "person.h"
#include "people.h"
#include "gamelevel.h"

Guests _guests; ///< Guests in the world/park.

/**
 * Guest block constructor. Fills the id of the persons with an incrementing number.
 * @param base_id Id number of the first person in this block.
 */
GuestBlock::GuestBlock(uint16 base_id)
{
	for (uint i = 0; i < lengthof(this->guests); i++) {
		this->guests[i].id = base_id;
		base_id++;
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
	return HasValidPath(vs) && GetImplodedPathSlope(vs) < PATH_FLAT_COUNT;
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
	this->free = nullptr;
	for (uint i = 0; i < GUEST_BLOCK_SIZE; i++) this->AddFree(this->block.Get(i));

	this->start_voxel.x = -1;
	this->start_voxel.y = -1;
	this->daily_frac = 0;
	this->next_daily_index = 0;
	this->valid_ptypes = 0;
}

Guests::~Guests()
{
}

/** After loading the RCD files, check what resources actually exist. */
void Guests::Initialize()
{
	this->valid_ptypes = 0;
	for (PersonType pertype = PERSON_MIN_GUEST; pertype <= PERSON_MAX_GUEST; pertype++) {
		bool usable = true;
		for (AnimationType antype = ANIM_BEGIN; antype <= ANIM_LAST; antype++) {
			if (_sprite_manager.GetAnimation(antype, pertype) == nullptr) {
				usable = false;
				break;
			}
		}
		if (usable) this->valid_ptypes |= 1u << (pertype - PERSON_MIN_GUEST);
	}
}

/**
 * Can guests of the given person type be used for the level?
 * @param ptype %Person type to examine.
 * @return Whether the given person type can be used for playing a level.
 */
bool Guests::CanUsePersonType(PersonType ptype)
{
	if (ptype < PERSON_MIN_GUEST || ptype > PERSON_MAX_GUEST) return false;
	return (this->valid_ptypes & (1 << (ptype - PERSON_MIN_GUEST))) != 0;
}

/**
 * Count the number of active guests.
 * @return The number of active guests.
 */
uint Guests::CountActiveGuests()
{
	uint count = GUEST_BLOCK_SIZE;
	const VoxelObject *pers = this->free;
	while (pers != nullptr) {
		assert(count > 0);
		count--;
		pers = pers->next_object;
	}
	return count;
}

/**
 * Some time has passed, update the animation.
 * @param delay Number of milliseconds time that have past since the last animation update.
 */
void Guests::OnAnimate(int delay)
{
	for (int i = 0; i < GUEST_BLOCK_SIZE; i++) {
		Guest *p = this->block.Get(i);
		if (p->type == PERSON_INVALID) continue;

		AnimateResult ar = p->OnAnimate(delay);
		if (ar != OAR_OK) {
			p->DeActivate(ar);
			this->AddFree(p);
		}
	}
}

/** A new frame arrived, perform the daily call for some of the guests. */
void Guests::DoTick()
{
	this->daily_frac++;
	int end_index = std::min(this->daily_frac * GUEST_BLOCK_SIZE / TICK_COUNT_PER_DAY, GUEST_BLOCK_SIZE);
	while (this->next_daily_index < end_index) {
		Guest *p = this->block.Get(this->next_daily_index);
		if (p->type != PERSON_INVALID && !p->DailyUpdate()) {
			p->DeActivate(OAR_REMOVE);
			this->AddFree(p);
		}
		this->next_daily_index++;
	}
	if (this->next_daily_index >= GUEST_BLOCK_SIZE) {
		this->daily_frac = 0;
		this->next_daily_index = 0;
	}
}

/**
 * A new day arrived, handle daily chores of the park.
 * @todo Add popularity rating concept.
 * @todo Person type should be configurable too.
 */
void Guests::OnNewDay()
{
	PersonType ptype = PERSON_PILLAR;
	if (!this->CanUsePersonType(ptype)) return;
	if (this->CountActiveGuests() >= _scenario.max_guests) return;
	if (!this->rnd.Success1024(_scenario.GetSpawnProbability(512))) return;

	if (!IsGoodEdgeRoad(this->start_voxel.x, this->start_voxel.y)) {
		/* New guest, but no road. */
		this->start_voxel = FindEdgeRoad();
		if (!IsGoodEdgeRoad(this->start_voxel.x, this->start_voxel.y)) return;
	}

	if (!this->HasFreeGuests()) return; // No more quests available.
	/* New guest! */
	Guest *g = this->GetFree();
	g->Activate(this->start_voxel, ptype);
}

/**
 * Return whether there are still non-active guests.
 * @return \c true if there are non-active guests, else \c false.
 */
bool Guests::HasFreeGuests() const
{
       return this->free != nullptr;
}

/**
 * Add a guest to the non-active list.
 * @param g %Guest to add.
 */
void Guests::AddFree(Guest *g)
{
       g->next_object = this->free;
       this->free = g;
}

/**
 * Get a non-active guest.
 * @return A non-active guest.
 * @pre #HasFreeGuests() should hold.
 */
Guest *Guests::GetFree()
{
       Guest *g = this->free;
       this->free = static_cast<Guest *>(g->next_object);
       return g;
}

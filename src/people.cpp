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
	this->name = NULL;
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

	if (this->name != NULL) return this->name;
	sprintf(buffer, "guest %u", this->id);
	return buffer;
}


Guest::Guest() : Person()
{
}

Guest::~Guest()
{
}

/**
 * Mark this guest as 'in use'.
 * @param start Start x/y voxel stack for entering the world.
 */
void Guest::Activate(const Point16 &start)
{
	this->name = NULL;
	this->happiness = 50 + this->rnd.Uniform(50);

}

/** Mark this guest as 'not in use'. (Called by #Guests.) */
void Guest::DeActivate()
{
	free(this->name);
	this->name = NULL;
}

/**
 * Update the animation of the guest.
 * @param delay Amount of milli seconds since the last update.
 * @todo [high] Function is empty, make it so something useful instead.
 */
void Guest::OnAnimate(int delay)
{
}

/**
 * Daily ponderings of a guest.
 * @return If \c false, de-activate the guest.
 * @todo Make de-activation a bit more random.
 */
bool Guest::DailyUpdate()
{
	this->happiness = max(0, this->happiness - 2);
	return this->happiness > 10; // De-activate if at or below 10.
}


GuestBlock::GuestBlock(uint16 base_id) : Block<Guest, GUEST_BLOCK_SIZE>(base_id)
{
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
	GuestBlock::iterator iter;
	for (iter = block.begin(); iter != block.end(); iter++) {
		(*iter)->OnAnimate(delay);
	}
}

/** A new frame arrived, perform the daily call for some of the guests. */
void Guests::DoTick()
{
	this->daily_frac++;
	int end_index = min(this->daily_frac * GUEST_BLOCK_SIZE / TICK_COUNT_PER_DAY, GUEST_BLOCK_SIZE);
	while (this->next_daily_index < end_index) {
		if (this->block.IsActive(this->next_daily_index)) {
			Guest &g = this->block.Get(this->next_daily_index);
			if (!g.DailyUpdate()) {
				printf("Guest %d is leaving\n", g.id);
				this->block.DeActivate(&g);
			}
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
 * @todo Chance of spawning a new guest should depends on popularity (and be configurable for a level).
 */
void Guests::OnNewDay()
{
	if (this->rnd.Success1024(512)) {
		if (!IsGoodEdgeRoad(this->start_voxel.x, this->start_voxel.y)) {
			printf("New guest, but no road\n");
			this->start_voxel = FindEdgeRoad();
			if (!IsGoodEdgeRoad(this->start_voxel.x, this->start_voxel.y)) return;
		}
		printf("New guest!\n");
		Guest *g = this->block.GetNew();
		if (g != NULL) g->Activate(this->start_voxel);
	}
}


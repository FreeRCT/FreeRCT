/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file people.cpp People in the world. */

#include "stdafx.h"
#include "dates.h"
#include "people.h"
#include "math_func.h"

Guests _guests; ///< Guests in the world/park.

Person::Person() : rnd()
{
}

Person::~Person()
{
}


Guest::Guest() : Person()
{
	this->name = NULL;
}

Guest::~Guest()
{
	free(this->name);
}

/** Mark this guest as 'in use'. */
void Guest::Activate()
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

/**
 * Set the name of a guest.
 * @param name New name of the guest.
 * @note Currently unused.
 */
void Guest::SetName(const char *name)
{
	int length = strlen(name);
	this->name = (char *)malloc(length + 1);
	memcpy(this->name, name, length);
	this->name[length] = '\0';
}

/**
 * Query the name of the guest.
 * @return Static buffer containing the name of the guest.
 * @note Currently not used.
 */
const char *Guest::GetName() const
{
	static char buffer[16];

	if (this->name != NULL) return this->name;
	sprintf(buffer, "guest %u", this->id);
	return buffer;
}


GuestBlock::GuestBlock(uint16 base_id) : Block<Guest, GUEST_BLOCK_SIZE>(base_id)
{
}

Guests::Guests() : block(0), rnd()
{
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
 */
void Guests::OnNewDay()
{
	if (this->rnd.Success1024(512)) {
		printf("New guest!\n");
		Guest *g = this->block.GetNew();
		if (g != NULL) g->Activate();
	}
}


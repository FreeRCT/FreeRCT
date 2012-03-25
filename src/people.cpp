/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file people.cpp People in the world. */

#include "stdafx.h"
#include "people.h"

Guests _guests; ///< Guests in the world/park.

Person::Person()
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

}

/** Mark this guest as 'not in use'. */
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
 * @todo [high] Function is empty, make it so something useful instead.
 */
void Guest::OnNewDay()
{
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


GuestBlock::GuestBlock(uint16 base_id) : Block<Guest, 512>(base_id)
{
}

Guests::Guests() : block(0)
{
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

/**
 * A new day arrived, perform 'all guests' type of daily updates.
 * @todo The daily update of each guest must be implemented in a different way than through here.
 */
void Guests::OnNewDay()
{
	if (this->rnd.Success1024(512)) {
		printf("New guest!\n");
		Guest *g = this->block.GetNew();
		g->Activate();
	}
}


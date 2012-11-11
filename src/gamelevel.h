/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gamelevel.h Game level. */

#ifndef GAME_LEVEL_H
#define GAME_LEVEL_H

#include "money.h"

/** Scenario settings. */
struct Scenario {
	Scenario();

	void SetDefaultScenario();

	uint GetSpawnProbability(int popularity);

	uint16 spawn_lowest;  ///< Guest spawn probability at lowest popularity (0..1024).
	uint16 spawn_highest; ///< Guest spawn probability at highest popularity (0..1024).
	uint16 max_guests;    ///< Maximal number of guests.

	Money inital_money;   ///< Initial amount of money of the player.
	Money lowest_money;   ///< Lowest amount of money of the player.
	uint8 interest;       ///< Interest rate in 0.1 percent over the negative amount.
};

extern Scenario _scenario;

#endif


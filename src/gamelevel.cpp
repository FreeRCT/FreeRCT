/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gamelevel.cpp Game level data. */

#include "stdafx.h"
#include "gamelevel.h"

Scenario _scenario; ///< The scenario being played.

/** Scenario default constructor. */
Scenario::Scenario()
{
	SetDefaultScenario();
}

/**
 * Default scenario settings.
 * Default settings are useful for debugging, and unlikely to be of use for a 'real' game.
 * @todo Remove when real scenarios are implemented.
 */
void Scenario::SetDefaultScenario()
{
	this->name = "Default";
	this->spawn_lowest  = 200;
	this->spawn_highest = 600;
	this->max_guests    = 3000;
	this->initial_money = 1000000;
	this->initial_loan  = this->initial_money;
	this->max_loan      = 3000000;
	this->interest      = 25;
}

/**
 * Get probability of spawning a new guest.
 * @param popularity Current popularity of the park (0..1024).
 * @return Spawning popularity between 0 and 1023 (inclusive).
 */
uint Scenario::GetSpawnProbability(int popularity)
{
	int increment = this->spawn_highest - this->spawn_lowest;
	return (int)this->spawn_lowest + increment * popularity / 1024;
}

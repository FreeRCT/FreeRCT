/* $Id$ */

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
User _user;         ///< Main user instance.

/** Scenario default constructor. */
Scenario::Scenario()
{
	SetDefaultScenario();
}

/**
 * Default scenario settings.
 * Default settings are useful for debugging, and unlikely to be of use for a 'real' game.
 */
void Scenario::SetDefaultScenario()
{
	this->spawn_lowest  = 500;
	this->spawn_highest = 500;
	this->max_guests    = 10;
	this->inital_money  = 1000000;
	this->lowest_money  = 0;
	this->interest      = 0;
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

/** Default constructor of the user. */
User::User()
{
	this->money = _scenario.inital_money;
}

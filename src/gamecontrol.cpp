/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gamecontrol.cpp High level game control code. */

#include "stdafx.h"
#include "gamecontrol.h"
#include "finances.h"
#include "sprite_store.h"
#include "ride_type.h"
#include "person.h"
#include "people.h"
#include "window.h"

/** Runs various procedures that have to be done yearly. */
void OnNewYear()
{
	// Nothing (yet) needed.
}

/** Runs various procedures that have to be done monthly. */
void OnNewMonth()
{
	_finances_manager.AdvanceMonth();
	_rides_manager.OnNewMonth();
}

/** Runs various procedures that have to be done daily. */
void OnNewDay()
{
	_guests.OnNewDay();
	NotifyChange(WC_BOTTOM_TOOLBAR, ALL_WINDOWS_OF_TYPE, CHG_DISPLAY_OLD, 0);
}


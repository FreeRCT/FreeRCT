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
#include "dates.h"
#include "gamemode.h"
#include "viewport.h"
#include "weather.h"

/** Initialize all game data structures for playing a new game. */
void StartNewGame()
{
	/// \todo We blindly assume game data structures are all clean.
	_world.SetWorldSize(20, 21);
	_world.MakeFlatWorld(8);
	_world.SetTileOwnerGlobally(OWN_NONE);
	_world.SetTileOwnerRect(2, 2, 16, 15, OWN_PARK);
	_world.SetTileOwnerRect(8, 0, 4, 2, OWN_PARK); // Allow building path to map edge in north west.
	_world.SetTileOwnerRect(2, 18, 16, 2, OWN_FOR_SALE);

	_finances_manager.SetScenario(_scenario);
	_date.Initialize();
	_weather.Initialize();
	_guests.Initialize();

	_game_mode_mgr.SetGameMode(GM_PLAY);

	ShowMainDisplay();
	ShowToolbar();
	ShowBottomToolbar();
}

/** Shutdown the game interaction. */
void ShutdownGame()
{
	/// \todo Clean out the game data structures.

	_game_mode_mgr.SetGameMode(GM_NONE);
	_mouse_modes.SetMouseMode(MM_INACTIVE);
	_manager.CloseAllWindows();
}

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
	_weather.OnNewDay();
	NotifyChange(WC_BOTTOM_TOOLBAR, ALL_WINDOWS_OF_TYPE, CHG_DISPLAY_OLD, 0);
}


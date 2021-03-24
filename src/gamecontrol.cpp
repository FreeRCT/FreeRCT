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
#include "messages.h"
#include "sprite_store.h"
#include "path_build.h"
#include "person.h"
#include "people.h"
#include "window.h"
#include "dates.h"
#include "scenery.h"
#include "viewport.h"
#include "weather.h"
#include "freerct.h"

GameModeManager _game_mode_mgr; ///< Game mode manager object.

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
	_rides_manager.OnNewDay();
	_guests.OnNewDay();
	_staff.OnNewDay();
	_weather.OnNewDay();
	NotifyChange(WC_BOTTOM_TOOLBAR, ALL_WINDOWS_OF_TYPE, CHG_DISPLAY_OLD, 0);
}

/**
 * Converts a speed setting to a factor.
 * @param speed Game speed setting.
 * @return The value to multiply all times with to achieve the desired speed.
*/
static int speed_factor(GameSpeed speed)
{
	switch (speed) {
		case GSP_PAUSE: return 0;
		case GSP_1:     return 1;
		case GSP_2:     return 2;
		case GSP_4:     return 4;
		case GSP_8:     return 8;
		default:       NOT_REACHED();
	}
}

/**
 * For every frame do...
 * @param frame_delay Number of milliseconds between two frames.
*/
void OnNewFrame(const uint32 frame_delay)
{
	_window_manager.Tick();
	if (_game_control.main_menu) return;

	_inbox.Tick(frame_delay);
	for (int i = speed_factor(_game_control.speed); i > 0; i--) {
		_guests.DoTick();
		_staff.DoTick();
		DateOnTick();
		_guests.OnAnimate(frame_delay);
		_staff.OnAnimate(frame_delay);
		_rides_manager.OnAnimate(frame_delay);
		_scenery.OnAnimate(frame_delay);
	}
}

GameControl::GameControl()
{
	this->speed = GSP_1;
	this->running = false;
	this->main_menu = false;
	this->next_action = GCA_NONE;
	this->fname = "";
}

GameControl::~GameControl()
{
}

/** Initialize the game controller. */
void GameControl::Initialize(const char *fname)
{
	this->speed = GSP_1;
	this->running = true;

	if (fname == nullptr) {
		this->MainMenu();
	} else {
		this->LoadGame(fname);
	}

	this->RunAction();
}

/** Uninitialize the game controller. */
void GameControl::Uninitialize()
{
	this->ShutdownLevel();
}

/**
 * Run latest game control action.
 * @pre next_action should not be equal to #GCA_NONE.
 */
void GameControl::RunAction()
{
	switch (this->next_action) {
		case GCA_NEW_GAME:
		case GCA_LOAD_GAME:
			this->ShutdownLevel();

			if (this->next_action == GCA_NEW_GAME || !LoadGameFile(this->fname.c_str())) {
				LoadGameFile(nullptr);  // Default-initialize everything.
				this->NewLevel();
			}

			this->StartLevel();
			break;

		case GCA_SAVE_GAME:
			SaveGameFile(this->fname.c_str());
			break;

		case GCA_MENU:
			this->ShutdownLevel();
			::ShowMainMenu();
			break;

		case GCA_QUIT:
			this->running = false;
			break;

		default:
			NOT_REACHED();
	}

	this->next_action = GCA_NONE;
}

/** Prepare for a #GCA_MENU action. */
void GameControl::MainMenu()
{
	this->next_action = GCA_MENU;
}

/** Prepare for a #GCA_NEW_GAME action. */
void GameControl::NewGame()
{
	this->next_action = GCA_NEW_GAME;
}

/**
 * Prepare for a #GCA_LOAD_GAME action.
 * @param fname Name of the file to load.
 */
void GameControl::LoadGame(const std::string &fname)
{
	this->fname = fname;
	this->next_action = GCA_LOAD_GAME;
}

/**
 * Prepare for a #GCA_SAVE_GAME action.
 * @param fname Name of the file to write.
 */
void GameControl::SaveGame(const std::string &fname)
{
	this->fname = fname;
	this->next_action = GCA_SAVE_GAME;
}

/** Prepare for a #GCA_QUIT action. */
void GameControl::QuitGame()
{
	this->next_action = GCA_QUIT;
}

/** Initialize all game data structures for playing a new game. */
void GameControl::NewLevel()
{
	/// \todo We blindly assume game data structures are all clean.
	_world.SetWorldSize(20, 21);
	_world.MakeFlatWorld(8);
	_world.SetTileOwnerGlobally(OWN_NONE);
	_world.SetTileOwnerRect(2, 2, 16, 15, OWN_PARK);
	_world.SetTileOwnerRect(2, 18, 16, 2, OWN_FOR_SALE);

	std::vector<const SceneryType*> park_entrance_types = _scenery.GetAllTypes(SCC_SCENARIO);
	if (park_entrance_types.size() < 3) {
		_world.SetTileOwnerRect(8, 0, 4, 2, OWN_PARK); // Allow building path to map edge in north west.
	} else {
		/* Assemble a park entrance and some paths. This assumes that the entrance parts are the first three scenario scenery items loaded. */
		_world.AddEdgesWithoutBorderFence(Point16(9, 1), EDGE_SE);
		for (int i = 0; i < 3; i++) {
			SceneryInstance *item = new SceneryInstance(park_entrance_types[i]);
			item->orientation = 0;
			item->vox_pos.x = 8 + i;
			item->vox_pos.y = 1;
			item->vox_pos.z = (i == 1 ? 12 : 8);
			_scenery.AddItem(item);
		}
		for (int i = 0; i < 6; i++) {
			BuildFlatPath(XYZPoint16(9, i, 8), PAT_CONCRETE, false);
		}
	}

	_inbox.Clear();
	_finances_manager.SetScenario(_scenario);
	_date.Initialize();
	_weather.Initialize();
}

/** Initialize common game settings and view. */
void GameControl::StartLevel()
{
	_game_mode_mgr.SetGameMode(GM_PLAY);
	this->speed = GSP_1;

	XYZPoint32 view_pos(_world.GetXSize() * 256 / 2, _world.GetYSize() * 256 / 2, 8 * 256);
	ShowMainDisplay(view_pos);
	ShowToolbar();
	ShowBottomToolbar();
}

/** Shutdown the game interaction. */
void GameControl::ShutdownLevel()
{
	/// \todo Clean out the game data structures.
	_game_mode_mgr.SetGameMode(GM_NONE);
	_window_manager.CloseAllWindows();
	_rides_manager.DeleteAllRideInstances();
	_scenery.Clear();
	_guests.Uninitialize();
	_staff.Uninitialize();
}

GameModeManager::GameModeManager()
{
	this->game_mode = GM_NONE;
}

GameModeManager::~GameModeManager()
{
}

/**
 * Change game mode of the program.
 * @param new_mode New mode to use.
 */
void GameModeManager::SetGameMode(GameMode new_mode)
{
	this->game_mode = new_mode;
	NotifyChange(WC_TOOLBAR, 0, CHG_UPDATE_BUTTONS, 0);
}

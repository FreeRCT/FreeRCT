/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gamecontrol.cpp High level game control code. */

#include "stdafx.h"
#include "gamecontrol.h"
#include "gameobserver.h"
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
#include "fileio.h"
#include "rev.h"

GameModeManager _game_mode_mgr; ///< Game mode manager object.

/** Runs various procedures that have to be done yearly. */
void OnNewYear()
{
	// Nothing (yet) needed.
}

/** Runs various procedures that have to be done monthly. */
void OnNewMonth()
{
	Autosave();
	_finances_manager.AdvanceMonth();
	_staff.OnNewMonth();
	_rides_manager.OnNewMonth();
}

/** Runs various procedures that have to be done daily. */
void OnNewDay()
{
	_rides_manager.OnNewDay();
	_guests.OnNewDay();
	_staff.OnNewDay();
	_weather.OnNewDay();
	_finances_manager.OnNewDay();
	_game_observer.OnNewDay();
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
	_image_variants.Tick();
	_window_manager.Tick();
	_inbox.Tick(frame_delay);
	for (int i = speed_factor(_game_control.speed); i > 0; i--) {
		_guests.DoTick();
		_staff.DoTick();
		DateOnTick();
		_game_observer.DoTick();
		_guests.OnAnimate(frame_delay);
		_staff.OnAnimate(frame_delay);
		_rides_manager.OnAnimate(frame_delay);
		_scenery.OnAnimate(frame_delay);
	}
}

int _max_autosaves(3);  ///< How many autosave files are retained at most. 0 disables autosave.

/**
 * Get the file path for an autosave with index #i.
 * @param i Index number for the filename.
 * @return The file path.
 */
static std::string AutosaveFilename(int i)
{
	std::string file = SavegameDirectory();
	file += "autosave_";
	file += std::to_string(i);
	file += ".fct";
	return file;
}

/** Create a new automatic savegame, and roll older autosaves. */
void Autosave()
{
	if (_max_autosaves < 1) return;

	/* Roll old autosaves. */
	for (int i = _max_autosaves - 1; i > 0; --i) {
		std::string old_file = AutosaveFilename(i);
		if (PathIsFile(old_file)) {
		    std::string new_file = AutosaveFilename(i + 1);
		    CopyBinaryFile(old_file, new_file);
		}
	}

	_game_control.SaveGame(AutosaveFilename(1));
}

GameControl::GameControl()
:
	running(false),
	main_menu(false),
	speed(GSP_1),
	action_test_mode(false),
	next_action(GCA_NONE),
	next_scenario(nullptr)
{
}

/**
 * Initialize the game controller.
 * @param fname File to load (may be empty).
 * @param game_mode Mode to load the game in.
 */
void GameControl::Initialize(const std::string &fname, GameMode game_mode)
{
	this->speed = GSP_1;
	this->running = true;

	if (fname.empty()) {
		if (game_mode == GM_EDITOR) {
			this->LaunchEditor();
		} else {
			this->MainMenu();
		}
	} else {
		this->LoadGame(fname, game_mode);
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
		case GCA_LAUNCH_EDITOR:
			this->ShutdownLevel();
			LoadGameFile(nullptr);
			this->InitializeLevel();
			this->StartLevel(GM_EDITOR);
			break;

		case GCA_LOAD_GAME:
		case GCA_LOAD_EDITOR:
			this->ShutdownLevel();
			LoadGameFile(this->fname.c_str());
			this->StartLevel(this->next_action == GCA_LOAD_EDITOR ? GM_EDITOR : GM_PLAY);
			break;

		case GCA_NEW_GAME: {
			this->ShutdownLevel();

			Loader ldr(this->next_scenario->fct_bytes.get(), this->next_scenario->fct_length);
			::LoadGame(ldr);

			this->InitializeLevel();
			this->StartLevel(GM_PLAY);

			this->next_scenario = nullptr;
			ShowParkManagementGui(PARK_MANAGEMENT_TAB_OBJECTIVE);
			break;
		}

		case GCA_SAVE_GAME:
			SaveGameFile(this->fname.c_str());
			break;

		case GCA_MENU: {
			this->main_menu = true;

			this->ShutdownLevel();
			Loader ldr(_main_menu_config.main_menu_savegame_bytes.get(), _main_menu_config.main_menu_savegame_length);
			::LoadGame(ldr);
			this->StartLevel(GM_PLAY);

			::ShowMainMenu();
			break;
		}

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

/**
 * Prepare for a #GCA_NEW_GAME action.
 * @param scenario The scenario to load.
 */
void GameControl::NewGame(MissionScenario *scenario)
{
	this->next_action = GCA_NEW_GAME;
	this->next_scenario = scenario;
}

/** Prepare for a #GCA_LAUNCH_EDITOR action. */
void GameControl::LaunchEditor()
{
	this->next_action = GCA_LAUNCH_EDITOR;
}

/**
 * Prepare for a #GCA_LOAD_GAME or #GCA_LOAD_EDITOR action.
 * @param fname Name of the file to load.
 * @param game_mode Mode to load the game in.
 */
void GameControl::LoadGame(const std::string &fname, GameMode game_mode)
{
	this->fname = fname;
	this->next_action = game_mode == GM_EDITOR ? GCA_LOAD_EDITOR : GCA_LOAD_GAME;
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
void GameControl::InitializeLevel()
{
	Random::Initialize();

	if (this->next_scenario != nullptr) {
		_scenario = this->next_scenario->scenario;
		_scenario.wrapper = this->next_scenario;
		_scenario.name  = _language.GetSgText(this->next_scenario->name);
		_scenario.descr = _language.GetSgText(this->next_scenario->descr);
	} else {
		Loader ldr(_main_menu_config.default_scenario_bytes.get(), _main_menu_config.default_scenario_length);
		::LoadGame(ldr);
	}

	_inbox.Clear();
	_date.Initialize();
	_weather.Initialize();
	_game_observer.Initialize();
}

/**
 * Initialize common game settings and view.
 * @param game_mode Mode to start the game in.
 */
void GameControl::StartLevel(GameMode game_mode)
{
	_game_mode_mgr.SetGameMode(game_mode);
	this->speed = game_mode != GM_PLAY ? GSP_PAUSE : GSP_1;

	XYZPoint32 view_pos(_world.GetXSize() * 256 / 2, _world.GetYSize() * 256 / 2, 8 * 256);
	ShowMainDisplay(view_pos);

	if (!this->main_menu) {
		const XYZPoint16 coords = _world.GetParkEntrance();
		if (coords != XYZPoint16::invalid()) _window_manager.GetViewport()->view_pos = VoxelToPixel(coords);

		ShowToolbar();
		ShowBottomToolbar();
	}
}

/** Shutdown the game interaction. */
void GameControl::ShutdownLevel()
{
	/// \todo Clean out the game data structures.
	_game_mode_mgr.SetGameMode(GM_NONE);
	_window_manager.CloseAllWindows();
	_rides_manager.DeleteAllRideInstances();
	_scenery.Clear();
	_game_observer.Uninitialize();
	_guests.Uninitialize();
	_staff.Uninitialize();
}

GameModeManager::GameModeManager() : game_mode(GM_NONE)
{
}

/**
 * Change game mode of the program.
 * @param new_mode New mode to use.
 */
void GameModeManager::SetGameMode(GameMode new_mode)
{
	this->game_mode = new_mode;
}

/**
 * Constructor.
 * @param t Type of action for which to show error messages.
 */
BestErrorMessageReason::BestErrorMessageReason(CheckActionType t)
{
	this->type = t;
	this->reason = STR_NULL;
}

/**
 * Show the current error message to the user, if any.
 * @return An error was shown.
 */
bool BestErrorMessageReason::ShowErrorMessage() const
{
	if (!this->IsSet()) return false;
	ShowActionErrorMessage(this->type, this->reason);
	return true;
}

/**
 * Display an error message to inform the user that an action is not allowed.
 * @param type Type of action that is forbidden.
 * @param error Reason (may be \c STR_NULL).
 */
void BestErrorMessageReason::ShowActionErrorMessage(const CheckActionType type, const StringID error)
{
	::ShowErrorMessage(type == ACT_BUILD ? GUI_ERROR_MESSAGE_HEADING_BUILD : GUI_ERROR_MESSAGE_HEADING_REMOVE, error, [](){});
}

/**
 * Checks whether the player is allowed to perform an action,
 * and displays an error message if this is not the case.
 * @param type Type of action to check.
 * @param cost How expensive the action will be (ignored if ``<= 0``).
 * @return The action is allowed.
 * @note Does not check whether the land is suited for building or a removable item is located here in the first place.
 */
bool BestErrorMessageReason::CheckActionAllowed(const CheckActionType type, const Money &cost)
{
	const StringID heading = (type == ACT_BUILD ? GUI_ERROR_MESSAGE_HEADING_BUILD : GUI_ERROR_MESSAGE_HEADING_REMOVE);

	if (_game_mode_mgr.InPlayMode() && _game_control.speed == GSP_PAUSE) {
		/* Game paused. */
		::ShowErrorMessage(heading, GUI_ERROR_MESSAGE_PAUSED, [](){});
		return false;
	}

	if (_game_mode_mgr.InPlayMode() && cost > 0 && cost > _finances_manager.GetCash()) {
		/* Not enough cash. */
		::ShowErrorMessage(heading, GUI_ERROR_MESSAGE_EXPENSIVE, [cost]() { _str_params.SetMoney(1, cost); });
		return false;
	}

	/** All checks clear. */
	return true;
}

/** Assigns every error message a priority, to decide which one should be shown when multiple are applicable. */
static const std::map<StringID, int> ERROR_MESSAGE_REASON_PRIORITIES = {
	{STR_NULL,                         1},
	{GUI_ERROR_MESSAGE_BAD_LOCATION,  10},
	{GUI_ERROR_MESSAGE_UNOWNED_LAND,  20},
	{GUI_ERROR_MESSAGE_UNDERGROUND,   30},
	{GUI_ERROR_MESSAGE_OCCUPIED,      40},
	{GUI_ERROR_MESSAGE_UNREMOVABLE,   50},
	{GUI_ERROR_MESSAGE_SLOPE,         70},
	{GUI_ERROR_MESSAGE_EXPENSIVE,     90},
	{GUI_ERROR_MESSAGE_PAUSED,       100},
};

/**
 * Decides whether to replace the current reason with another one.
 * @param other Another applicable reason.
 */
void BestErrorMessageReason::UpdateReason(const StringID other)
{
	if (ERROR_MESSAGE_REASON_PRIORITIES.at(this->reason) < ERROR_MESSAGE_REASON_PRIORITIES.at(other)) this->reason = other;
}

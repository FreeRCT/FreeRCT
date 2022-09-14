/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gamecontrol.h High level game control functions. */

#ifndef GAMECONTROL_H
#define GAMECONTROL_H

#include "language.h"
#include "money.h"

void OnNewDay();
void OnNewMonth();
void OnNewYear();
void OnNewFrame(uint32 frame_delay);
void Autosave();
extern int _max_autosaves;

/** Actions that can be run to control the game. */
enum GameControlAction {
	GCA_NONE,      ///< No action to run.
	GCA_MENU,      ///< Open the main menu.
	GCA_NEW_GAME,  ///< Prepare a new game.
	GCA_LOAD_GAME, ///< Load a saved game.
	GCA_SAVE_GAME, ///< Save the current game.
	GCA_QUIT,      ///< Quit the game.
};

/** How fast the game should run. */
enum GameSpeed {
	GSP_PAUSE,  ///< The game is paused.
	GSP_1,      ///< Normal speed.
	GSP_2,      ///< Double speed.
	GSP_4,      ///< 4 times speed.
	GSP_8,      ///< 8 times speed.
	GSP_COUNT   ///< Number of entries.
};

/**
 * Class controlling the current game.
 */
class GameControl {
public:
	GameControl();

	/**
	 * If applicable, run the latest action.
	 */
	inline void DoNextAction()
	{
		if (this->next_action != GCA_NONE) this->RunAction();
	}

	void Initialize(const std::string &fname);
	void Uninitialize();

	void MainMenu();
	void NewGame();
	void LoadGame(const std::string &fname);
	void SaveGame(const std::string &fname);
	void QuitGame();

	bool running;    ///< Indicates whether a game is currently running.
	bool main_menu;  ///< Indicates whether the main menu is currently open.

	GameSpeed speed;  ///< Speed of the game.

	bool action_test_mode;  ///< Don't perform any actions, only check what they would cost.

private:
	void RunAction();
	void NewLevel();
	void StartLevel();
	void ShutdownLevel();

	GameControlAction next_action; ///< Action game control wants to run, or #GCA_NONE for 'no action'.
	std::string fname;             ///< Filename of game level to load from or save to.
};

extern GameControl _game_control;

/**
 * The current game mode controls what user operations that are allowed
 * and not. In Game mode most construction operations are limited to
 * owned land.
 */
enum GameMode {
	GM_NONE,   ///< Neither a running game nor in editor. (eg. startup/quit)
	GM_PLAY,   ///< The current scenario is being played.
	GM_EDITOR, ///< The current scenario is being edited.
};

/** Class managing the game mode of the program.
 *  @todo Move functionality to GameControl class?
 */
class GameModeManager {
public:
	GameModeManager();

	void SetGameMode(GameMode new_mode);
	inline GameMode GetGameMode() const;
	inline bool InPlayMode() const;
	inline bool InEditorMode() const;

private:
	GameMode game_mode; ///< Current game mode of the program.
};

/**
 * Get current game mode.
 * @return Current game mode.
 */
inline GameMode GameModeManager::GetGameMode() const
{
	return this->game_mode;
}

/**
 * Checks if the current game mode is GM_PLAY.
 * @return true if current game mode is GM_PLAY.
 */
inline bool GameModeManager::InPlayMode() const
{
	return this->game_mode == GM_PLAY;
}

/**
 * Checks if the current game mode is GM_EDITOR.
 * @return true if current game mode is GM_EDITOR.
 */
inline bool GameModeManager::InEditorMode() const
{
	return this->game_mode == GM_EDITOR;
}

extern GameModeManager _game_mode_mgr;

/** A class that picks the error message that will be shown to the user if multiple messages are applicable. */
class BestErrorMessageReason {
public:
	/** Types of action for which it is necessary to check whether they may be performed. */
	enum CheckActionType {
		ACT_BUILD,   ///< Build rides, scenery, paths, etc.
		ACT_REMOVE,  ///< Remove a path, scenery, etc.
	};

	explicit BestErrorMessageReason(CheckActionType t);

	static void ShowActionErrorMessage(CheckActionType type, StringID error = STR_NULL);
	static bool CheckActionAllowed(CheckActionType type, const Money &cost);

	void UpdateReason(StringID other);
	bool ShowErrorMessage() const;

	/**
	 * Whether this class holds an error currently.
	 * @return An error is set.
	 */
	inline bool IsSet() const
	{
		return this->reason != STR_NULL;
	}

	/** Clear the error. */
	inline void Reset()
	{
		this->reason = STR_NULL;
	}

	CheckActionType type;   ///< Type of action.
	StringID reason;        ///< The reason which is currently deemed most important.
};

#endif

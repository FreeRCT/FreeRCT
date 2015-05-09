/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gamecontrol.h High level game control functions. */

#ifndef GAMECONTROL_H
#define GAMECONTROL_H

void OnNewDay();
void OnNewMonth();
void OnNewYear();
void OnNewFrame(uint32 frame_delay);

/** Actions that can be run to control the game. */
enum GameControlAction {
	GCA_NONE,      ///< No action to run.
	GCA_NEW_GAME,  ///< Prepare a new game.
	GCA_LOAD_GAME, ///< Load a saved game.
	GCA_SAVE_GAME, ///< Save the current game.
	GCA_QUIT,      ///< Quit the game.
};

/**
 * Class controlling the current game.
 */
class GameControl {
public:
	GameControl();
	~GameControl();

	/**
	 * If applicable, run the latest action.
	 */
	inline void DoNextAction()
	{
		if (this->next_action != GCA_NONE) this->RunAction();
	}

	void Initialize();
	void Uninitialize();

	void NewGame();
	void LoadGame(const std::string &fname);
	void SaveGame(const std::string &fname);
	void QuitGame();

	bool running; ///< Indicates whether a game is currently running.

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
	~GameModeManager();

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

#endif

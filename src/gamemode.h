/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gamemode.h Game modes. */

#ifndef GAMEMODE_TYPE_H
#define GAMEMODE_TYPE_H

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

class GameModeManager {
public:
	GameModeManager();
	~GameModeManager();

	void SetGameMode(GameMode new_mode);
	inline GameMode GetGameMode() const;
	inline bool InPlayMode() const;
	inline bool InEditorMode() const;

private:
	GameMode game_mode;
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

/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gamemode.cpp Game mode manager. */

#include "stdafx.h"
#include "gamemode.h"
#include "window.h"

GameModeManager _game_mode_mgr; ///< Game mode manager object.

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

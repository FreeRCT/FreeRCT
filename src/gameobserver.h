/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gameobserver.h Keeps track of a scenario's progress. */

#ifndef GAMEOBSERVER_H
#define GAMEOBSERVER_H

#include <deque>
#include <string>

#include "dates.h"
#include "money.h"

/** Whether the scenario has been won or lost. */
enum WonLost {
	SCENARIO_RUNNING = 0,  ///< The scenario has not been won or lost yet.
	SCENARIO_WON     = 1,  ///< The scenario has been won.
	SCENARIO_LOST    = 2,  ///< The scenario has been lost.
	SCENARIO_WON_FIRST = 3,  ///< The scenario has been won for the first time.
};

static const unsigned STATISTICS_HISTORY = DaysInParkYear();  ///< Number of days for statistics are kept around.
static const int MAX_PARK_RATING = 1000;                      ///< Best possible park rating.
static const int PARK_ENTRANCE_FEE_STEP_SIZE = 100;           ///< Step size of changing the park's entrance fee in the GUI.

/** Keeps track of a scenario's progress. */
class GameObserver {
public:
	void Initialize();
	void Uninitialize();

	void DoTick();
	void OnNewDay();

	void Load(Loader &ldr);
	void Save(Saver &svr);

	void Win();
	void Lose();

	void SetParkOpen(bool open);

	bool park_open;         ///< The park is currently open.
	std::string park_name;  ///< Title of the scenario.
	Money entrance_fee;     ///< Park entrance fee.

	uint32 current_guest_count;  ///< Number of guests in the park right now.
	uint16 current_park_rating;  ///< The park rating right now.
	uint32 max_guests;           ///< The highest number of guests who have ever been in the park.

	std::deque<int> guest_count_history;  ///< Guest count over the last year (most recent first).
	std::deque<int> park_rating_history;  ///< Park rating over the last year (most recent first).
	WonLost won_lost;                     ///< Whether the scenario has been won or lost.

private:
	int CalculateParkRating();
};

extern GameObserver _game_observer;

#endif

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gameobserver.cpp Keeps track of a scenario's progress. */

#include "stdafx.h"
#include "gameobserver.h"
#include "gamelevel.h"
#include "gamecontrol.h"
#include "finances.h"
#include "messages.h"
#include "people.h"
#include "window.h"

GameObserver _game_observer;  ///< Game observer instance.

/** Initialize all data structures at the start of a new game. */
void GameObserver::Initialize()
{
	this->Uninitialize();
	this->park_name = _scenario.name;
	this->won_lost = SCENARIO_RUNNING;
	this->park_open = true;
}

/** Clean up all data structures at the end of a game. */
void GameObserver::Uninitialize()
{
	this->guest_count_history.clear();
	this->park_rating_history.clear();
	this->current_guest_count = 0;
	this->current_park_rating = 0;
	this->max_guests = 0;
	this->entrance_fee = Money(0);
	this->park_open = false;
}

/** A new day has arrived. */
void GameObserver::OnNewDay()
{
	this->current_park_rating = CalculateParkRating();

	this->guest_count_history.push_front(this->current_guest_count);
	this->park_rating_history.push_front(this->current_park_rating);
	while (this->guest_count_history.size() > STATISTICS_HISTORY) this->guest_count_history.pop_back();
	while (this->park_rating_history.size() > STATISTICS_HISTORY) this->park_rating_history.pop_back();

	if (this->won_lost == SCENARIO_RUNNING) _scenario.objective->OnNewDay();
}

/** A new frame has arrived. */
void GameObserver::DoTick()
{
	this->current_guest_count = _guests.CountGuestsInPark();
	this->max_guests = std::max(this->max_guests, this->current_guest_count);
}

/** The game has been won. */
void GameObserver::Win()
{
	assert(this->won_lost == SCENARIO_RUNNING);
	this->won_lost = SCENARIO_WON;
	_inbox.SendMessage(new Message(GUI_MESSAGE_SCENARIO_WON));

	if (_game_mode_mgr.InPlayMode() && !_scenario.wrapper->solved.has_value()) {
		this->won_lost = SCENARIO_WON_FIRST;

		std::string username;
		for (const auto& var : {"USER", "USERNAME"}) {
			const char *environment_variable = getenv(var);
			if (environment_variable != nullptr && environment_variable[0] != '\0') {
				username = environment_variable;
				break;
			}
		}
		if (username.empty()) username = _language.GetSgText(GUI_NO_NAME);

		_scenario.wrapper->solved = {
			username,
			_finances_manager.GetCompanyValue(),
			std::time(nullptr)
		};
		_scenario.wrapper->mission->UpdateUnlockData();
	}

	ShowParkManagementGui(PARK_MANAGEMENT_TAB_OBJECTIVE);
}

/** The game has been lost. */
void GameObserver::Lose()
{
	assert(this->won_lost == SCENARIO_RUNNING);
	this->won_lost = SCENARIO_LOST;
	_inbox.SendMessage(new Message(GUI_MESSAGE_SCENARIO_LOST));
	this->SetParkOpen(false);
	ShowParkManagementGui(PARK_MANAGEMENT_TAB_OBJECTIVE);
}

/**
 * Open or close the park, if allowed.
 * @param open The park should be open.
 */
void GameObserver::SetParkOpen(bool open)
{
	this->park_open = open && this->won_lost != SCENARIO_LOST;
}

/**
 * Recalculate the park's current park rating.
 * @return The park rating, from 0 (terrible) to #MAX_PARK_RATING (perfect).
 * @todo Implement this.
 */
int GameObserver::CalculateParkRating()
{
	Random r;
	return std::max(0, std::min(MAX_PARK_RATING, this->current_park_rating + r.Uniform(60) - 20));
}

static const uint32 CURRENT_VERSION_GOBS = 1;   ///< Currently supported version of the GOBS Pattern.

/**
 * Load game observer data from the save game.
 * @param ldr Input stream to load from.
 */
void GameObserver::Load(Loader &ldr)
{
	uint32 version = ldr.OpenPattern("GOBS");
	switch (version) {
		case 0:
			this->Initialize();
			break;

		case 1:
			this->won_lost = static_cast<WonLost>(ldr.GetByte());
			this->park_open = ldr.GetByte() > 0;
			this->park_name = ldr.GetText();
			this->entrance_fee = ldr.GetLong();
			this->current_park_rating = ldr.GetWord();
			this->current_guest_count = ldr.GetLong();
			this->max_guests = ldr.GetLong();
			for (size_t i = ldr.GetLong(); i > 0; --i) this->park_rating_history.push_back(ldr.GetWord());
			for (size_t i = ldr.GetLong(); i > 0; --i) this->guest_count_history.push_back(ldr.GetLong());
			break;

		default:
			ldr.VersionMismatch(version, CURRENT_VERSION_GOBS);
	}
	ldr.ClosePattern();
}

/**
 * Save game observer data to the save game.
 * @param svr Output stream to save to.
 */
void GameObserver::Save(Saver &svr)
{
	svr.CheckNoOpenPattern();
	svr.StartPattern("GOBS", CURRENT_VERSION_GOBS);

	svr.PutByte(this->won_lost);
	svr.PutByte(this->park_open ? 1 : 0);
	svr.PutText(this->park_name);
	svr.PutLong(this->entrance_fee);
	svr.PutWord(this->current_park_rating);
	svr.PutLong(this->current_guest_count);
	svr.PutLong(this->max_guests);

	svr.PutLong(this->park_rating_history.size());
	for (int i : this->park_rating_history) svr.PutWord(i);
	svr.PutLong(this->guest_count_history.size());
	for (int i : this->guest_count_history) svr.PutLong(i);

	svr.EndPattern();
}

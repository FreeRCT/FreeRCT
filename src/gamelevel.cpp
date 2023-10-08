/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gamelevel.cpp Game level data. */

#include "stdafx.h"
#include "gamelevel.h"
#include "messages.h"
#include "people.h"
#include "finances.h"
#include "gameobserver.h"
#include "config_reader.h"
#include "rev.h"
#include "generated/mission_strings.cpp"
#include "generated/mission_strings.h"

Scenario _scenario; ///< The scenario being played.

std::vector<std::unique_ptr<Mission>> _missions;  ///< All available missions.

/** Scenario default constructor. */
Scenario::Scenario()
{
}

/**
 * Initialize default settings for a new scenario in the editor.
 */
void Scenario::SetDefaultScenario()
{
	this->wrapper = nullptr;
	this->name  = _language.GetSgText(GUI_DEFAULT_SCENARIO_NAME);
	this->descr = _language.GetSgText(GUI_DEFAULT_SCENARIO_DESCR);
	this->spawn_lowest  = 200;
	this->spawn_highest = 600;
	this->max_guests    = 3000;
	this->max_loan      = 3000000;
	this->interest      = 25;
	this->allow_entrance_fee = true;
	this->objective.reset(new ScenarioObjective(0, TIMEOUT_EXACT, Date(31, 10, 1), {
		std::shared_ptr<AbstractObjective>(new ObjectiveGuests(0, 1000)),
		std::shared_ptr<AbstractObjective>(new ObjectiveParkRating(0, 600)),
	}));
}

/**
 * Get probability of spawning a new guest.
 * @param popularity Current popularity of the park (0..1024).
 * @return Spawning popularity between 0 and 1023 (inclusive).
 */
uint Scenario::GetSpawnProbability(int popularity)
{
	int increment = this->spawn_highest - this->spawn_lowest;
	return (int)this->spawn_lowest + increment * popularity / 1024;
}

/**
 * \fn std::string AbstractObjective::ToString() const
 * Generate a localized string representation of this objective.
 * @return Localized objective text.
 */

/**
 * Perform daily tasks related to this objective.
 */
void AbstractObjective::OnNewDay()
{
	if (this->drop_policy.days_after_drop == 0) return;

	if (this->is_fulfilled) {
		this->drop_policy.drop_counter = 0;
		return;
	}

	if (this->drop_policy.days_after_drop >= this->drop_policy.drop_counter) {
		_game_observer.Lose();
	} else if ((this->drop_policy.days_after_drop - this->drop_policy.drop_counter) % 7 == 0) {
		/* Message parameter is the number of weeks until the park is closed. */
		/* \todo Generalize message for other kinds of objectives. */
		_inbox.SendMessage(new Message(GUI_MESSAGE_BAD_RATING, (this->drop_policy.days_after_drop - this->drop_policy.drop_counter) / 7));
	}

	++this->drop_policy.drop_counter;
}

/** Constructor. */
ScenarioObjective::ScenarioObjective(uint32 y, ObjectiveTimeoutPolicy p, Date d, std::vector<std::shared_ptr<AbstractObjective>> o)
: AbstractObjective(y), objectives(o), timeout_policy(p), timeout_date(d)
{
}

std::string ObjectiveNone::ToString() const
{
	return DrawText(GUI_OBJECTIVETEXT_NONE);
}

std::string ObjectiveGuests::ToString() const
{
	_str_params.SetNumberAndPlural(1, this->nr_guests);
	return DrawText(GUI_OBJECTIVETEXT_GUESTS);
}

std::string ObjectiveParkRating::ToString() const
{
	_str_params.SetNumber(1, this->rating);
	return DrawText(GUI_OBJECTIVETEXT_PARK_RATING);
}

std::string ObjectiveParkValue::ToString() const
{
	_str_params.SetNumber(1, this->park_value);
	return DrawText(GUI_OBJECTIVETEXT_PARK_VALUE);
}

std::string ScenarioObjective::ToString() const
{
	std::string str;
	for (const auto &o : this->objectives) {
		str += o->ToString();
		str += "\n";
	}

	_str_params.SetDate(1, this->timeout_date);
	switch (this->timeout_policy) {
		case TIMEOUT_BEFORE: str += DrawText(GUI_OBJECTIVE_TIMEOUT_BEFORE); break;
		case TIMEOUT_EXACT:  str += DrawText(GUI_OBJECTIVE_TIMEOUT_EXACT ); break;
		case TIMEOUT_NONE:   str += DrawText(GUI_OBJECTIVE_TIMEOUT_NONE  ); break;
		default: NOT_REACHED();
	}
	if (this->drop_policy.days_after_drop > 0) str += DrawText(GUI_OBJECTIVE_STRICT);

	return str;
}

void ObjectiveGuests::OnNewDay()
{
	this->is_fulfilled = _game_observer.current_guest_count >= this->nr_guests;
	AbstractObjective::OnNewDay();
}

void ObjectiveParkRating::OnNewDay()
{
	this->is_fulfilled = _game_observer.current_park_rating >= this->rating;
	AbstractObjective::OnNewDay();
}

void ObjectiveParkValue::OnNewDay()
{
	this->is_fulfilled = _finances_manager.GetParkValue() >= this->park_value;
	AbstractObjective::OnNewDay();
}

void ScenarioObjective::OnNewDay()
{
	this->is_fulfilled = true;
	for (auto &o : this->objectives) {
		o->OnNewDay();
		this->is_fulfilled &= o->is_fulfilled;
	}

	if (this->is_fulfilled && this->timeout_policy != TIMEOUT_EXACT) {
		_game_observer.Win();
	} else if (this->timeout_policy != TIMEOUT_NONE && timeout_date < _date) {
		if (this->is_fulfilled) {
			_game_observer.Win();
		} else {
			_game_observer.Lose();
		}
	} else {
		AbstractObjective::OnNewDay();
	}
}

static const uint32 CURRENT_VERSION_SCNO = 3;   ///< Currently supported version of the SCNO Pattern.
static const uint32 CURRENT_VERSION_OJAO = 1;   ///< Currently supported version of the OJAO Pattern.
static const uint32 CURRENT_VERSION_OJCN = 1;   ///< Currently supported version of the OJCN Pattern.
static const uint32 CURRENT_VERSION_OJ00 = 1;   ///< Currently supported version of the OJ00 Pattern.
static const uint32 CURRENT_VERSION_OJGU = 1;   ///< Currently supported version of the OJGU Pattern.
static const uint32 CURRENT_VERSION_OJRT = 1;   ///< Currently supported version of the OJRT Pattern.
static const uint32 CURRENT_VERSION_OJPV = 1;   ///< Currently supported version of the OJPV Pattern.

/**
 * Load game observer data from the save game.
 * @param ldr Input stream to load from.
 */
void Scenario::Load(Loader &ldr)
{
	uint32 version = ldr.OpenPattern("SCNO");
	switch (version) {
		case 0:
			this->SetDefaultScenario();
			break;

		case 1:
		case 2:
		case 3:
			this->name = ldr.GetText();
			this->descr = version >= 2 ? ldr.GetText() : _language.GetSgText(GUI_DEFAULT_SCENARIO_DESCR);

			this->objective.reset(new ScenarioObjective(0, TIMEOUT_NONE, Date(), {}));
			this->objective->Load(ldr);
			this->spawn_lowest = ldr.GetWord();
			this->spawn_highest = ldr.GetWord();
			this->max_guests = ldr.GetLong();
			if (version <= 2) {
				ldr.GetLong();  // Was initial money.
				ldr.GetLong();  // Was initial loan.
			}
			this->max_loan = ldr.GetLong();
			this->interest = ldr.GetWord();
			this->allow_entrance_fee = version == 1 || ldr.GetByte() != 0;

			this->wrapper = nullptr;
			if (version >= 3) {
				std::string int_name = ldr.GetText();
				if (!int_name.empty()) {
					for (auto &mission : _missions) {
						for (MissionScenario &scenario : mission->scenarios) {
							if (scenario.internal_name == int_name) {
								this->wrapper = &scenario;
								break;
							}
						}
					}
				}
			}

			break;

		default:
			ldr.VersionMismatch(version, CURRENT_VERSION_SCNO);
	}
	ldr.ClosePattern();
}

/**
 * Save game observer data to the save game.
 * @param svr Output stream to save to.
 */
void Scenario::Save(Saver &svr)
{
	svr.StartPattern("SCNO", CURRENT_VERSION_SCNO);

	svr.PutText(this->name);
	svr.PutText(this->descr);
	this->objective->Save(svr);
	svr.PutWord(this->spawn_lowest);
	svr.PutWord(this->spawn_highest);
	svr.PutLong(this->max_guests);
	svr.PutLong(this->max_loan);
	svr.PutWord(this->interest);
	svr.PutByte(this->allow_entrance_fee ? 1 : 0);
	svr.PutText(this->wrapper != nullptr ? this->wrapper->internal_name : "");

	svr.EndPattern();
}

/**
 * Load the next objective from the save game.
 * @param ldr Input stream to load from.
 */
static std::shared_ptr<AbstractObjective> LoadObjective(Loader &ldr)
{
	std::shared_ptr<AbstractObjective> result;
	const unsigned obj_type = ldr.GetByte();

	switch (obj_type) {
		case OJT_CONTAINER:
			result.reset(new ScenarioObjective(0, TIMEOUT_NONE, Date(), {}));
			break;
		case OJT_NONE:
			result.reset(new ObjectiveNone);
			break;
		case OJT_GUESTS:
			result.reset(new ObjectiveGuests(0, 0));
			break;
		case OJT_RATING:
			result.reset(new ObjectiveParkRating(0, 0));
			break;
		case OJT_PARK_VALUE:
			result.reset(new ObjectiveParkValue(0, 0));
			break;
		default:
			throw LoadingError("Unknown objective type %u", obj_type);
	}

	result->Load(ldr);
	return result;
}

/**
 * Load an objective's data from the save game.
 * @param ldr Input stream to load from.
 */
void AbstractObjective::Load(Loader &ldr)
{
	uint32 version = ldr.OpenPattern("OJAO");
	if (version > CURRENT_VERSION_OJAO) ldr.VersionMismatch(version, CURRENT_VERSION_OJAO);
	this->is_fulfilled = ldr.GetByte();
	this->drop_policy.days_after_drop = ldr.GetLong();
	this->drop_policy.drop_counter = ldr.GetLong();
	ldr.ClosePattern();
}

/**
 * Save an objective's data to the save game.
 * @param svr Output stream to save to.
 */
void AbstractObjective::Save(Saver &svr)
{
	svr.StartPattern("OJAO", CURRENT_VERSION_OJAO);
	svr.PutByte(this->is_fulfilled);
	svr.PutLong(this->drop_policy.days_after_drop);
	svr.PutLong(this->drop_policy.drop_counter);
	svr.EndPattern();
}

void ScenarioObjective::Load(Loader &ldr)
{
	uint32 version = ldr.OpenPattern("OJCN");
	if (version > CURRENT_VERSION_OJCN) ldr.VersionMismatch(version, CURRENT_VERSION_OJCN);
	AbstractObjective::Load(ldr);

	this->timeout_policy = static_cast<ObjectiveTimeoutPolicy>(ldr.GetByte());
	this->timeout_date = Date(CompressedDate(ldr.GetLong()));

	this->objectives.clear();
	for (size_t i = ldr.GetLong(); i > 0; --i) this->objectives.emplace_back(LoadObjective(ldr));

	ldr.ClosePattern();
}

void ScenarioObjective::Save(Saver &svr)
{
	svr.StartPattern("OJCN", CURRENT_VERSION_OJCN);
	AbstractObjective::Save(svr);

	svr.PutByte(this->timeout_policy);
	svr.PutLong(this->timeout_date.Compress());
	svr.PutLong(this->objectives.size());
	for (auto &obj : this->objectives) {
		svr.PutByte(obj->Type());
		obj->Save(svr);
	}

	svr.EndPattern();
}

void ObjectiveNone::Load(Loader &ldr)
{
	uint32 version = ldr.OpenPattern("OJ00");
	if (version > CURRENT_VERSION_OJ00) ldr.VersionMismatch(version, CURRENT_VERSION_OJ00);
	AbstractObjective::Load(ldr);
	ldr.ClosePattern();
}

void ObjectiveNone::Save(Saver &svr)
{
	svr.StartPattern("OJ00", CURRENT_VERSION_OJ00);
	AbstractObjective::Save(svr);
	svr.EndPattern();
}

void ObjectiveGuests::Load(Loader &ldr)
{
	uint32 version = ldr.OpenPattern("OJGU");
	if (version > CURRENT_VERSION_OJGU) ldr.VersionMismatch(version, CURRENT_VERSION_OJGU);
	AbstractObjective::Load(ldr);
	this->nr_guests = ldr.GetLong();
	ldr.ClosePattern();
}

void ObjectiveGuests::Save(Saver &svr)
{
	svr.StartPattern("OJGU", CURRENT_VERSION_OJGU);
	AbstractObjective::Save(svr);
	svr.PutLong(this->nr_guests);
	svr.EndPattern();
}

void ObjectiveParkRating::Load(Loader &ldr)
{
	uint32 version = ldr.OpenPattern("OJRT");
	if (version > CURRENT_VERSION_OJRT) ldr.VersionMismatch(version, CURRENT_VERSION_OJRT);
	AbstractObjective::Load(ldr);
	this->rating = ldr.GetWord();
	ldr.ClosePattern();
}

void ObjectiveParkRating::Save(Saver &svr)
{
	svr.StartPattern("OJRT", CURRENT_VERSION_OJRT);
	AbstractObjective::Save(svr);
	svr.PutWord(this->rating);
	svr.EndPattern();
}

void ObjectiveParkValue::Load(Loader &ldr)
{
	uint32 version = ldr.OpenPattern("OJPV");
	if (version > CURRENT_VERSION_OJPV) ldr.VersionMismatch(version, CURRENT_VERSION_OJPV);
	AbstractObjective::Load(ldr);
	this->park_value = ldr.GetLongLong();
	ldr.ClosePattern();
}

void ObjectiveParkValue::Save(Saver &svr)
{
	svr.StartPattern("OJPV", CURRENT_VERSION_OJPV);
	AbstractObjective::Save(svr);
	svr.PutLongLong(this->park_value);
	svr.EndPattern();
}

static const std::string MISSION_SECTION_USER ("user");           ///< Section name in the missions config file for the name of the user who first solved a scenario.
static const std::string MISSION_SECTION_TIME ("timestamp");      ///< Section name in the missions config file for the timestamp when the scenario was solved.
static const std::string MISSION_SECTION_VALUE("company_value");  ///< Section name in the missions config file for the company value at the end of the scenario.

/**
 * Get the config file that holds information about solved missions.
 * @return The config file.
 */
static ConfigFile &GetMissionConfigFile()
{
	static ConfigFile cfg_file(freerct_userdata_prefix() + DIR_SEP + "missions.cfg");
	return cfg_file;
}

/**
 * Read a mission from the RCD file block and add it to the global list of missions.
 * @param rcd_file File to read from.
 * @param texts Already loaded texts.
 */
void LoadMission(RcdFileReader *rcd_file, const TextMap &texts)
{
	std::unique_ptr<Mission> mission(new Mission);

	rcd_file->CheckVersion(1);
	int length = rcd_file->size;
	rcd_file->CheckMinLength(length, 13, "header");

	mission->internal_name = rcd_file->GetText();
	length -= 13 + mission->internal_name.size();

	for (const auto &m : _missions) {
		if (m->internal_name == mission->internal_name) {
			rcd_file->Error("Mission %s already defined", mission->internal_name.c_str());
		}
	}

	TextData *text_data;
	LoadTextFromFile(rcd_file, texts, &text_data);
	StringID base = _language.RegisterStrings(*text_data, _mission_strings_table) - STR_GENERIC_MISSION_START;
	mission->name  = base + MISSION_NAME;
	mission->descr = base + MISSION_DESCR;

	mission->max_unlock = rcd_file->GetUInt32();
	const uint32 nr_scenarios = rcd_file->GetUInt32();

	if (nr_scenarios == 0) rcd_file->Error("Mission without scenarios");

	ConfigFile &cfg_file = GetMissionConfigFile();

	for (uint32 i = 0; i < nr_scenarios; ++i) {
		rcd_file->CheckMinLength(length, 9, "scenario header");
		mission->scenarios.emplace_back();
		MissionScenario &scenario = mission->scenarios.back();
		scenario.mission = mission.get();
		scenario.internal_name = rcd_file->GetText();
		length -= 9 + scenario.internal_name.size();

		scenario.internal_name.insert(0, "/");  // Prepend mission name to ensure the identifier is globally unique.
		scenario.internal_name.insert(0, mission->internal_name);
		for (uint32 j = 0; j < i; ++j) {
			if (mission->scenarios.at(j).internal_name == scenario.internal_name) {
				rcd_file->Error("Scenario %s already defined", scenario.internal_name.c_str());
			}
		}

		LoadTextFromFile(rcd_file, texts, &text_data);
		StringID base = _language.RegisterStrings(*text_data, _mission_strings_table) - STR_GENERIC_MISSION_START;
		scenario.name  = base + MISSION_NAME;
		scenario.descr = base + MISSION_DESCR;

		scenario.fct_length = rcd_file->GetUInt32();
		rcd_file->CheckMinLength(length, scenario.fct_length, "scenario blob");
		length -= scenario.fct_length;

		scenario.fct_bytes.reset(new uint8[scenario.fct_length]);
		if (!rcd_file->GetBlob(scenario.fct_bytes.get(), scenario.fct_length)) rcd_file->Error("Reading scenario bytes %u failed", i);

		Loader ldr(scenario.fct_bytes.get(), scenario.fct_length);
		PreloadData preload = Preload(ldr);
		if (!preload.load_success) rcd_file->Error("Preloading scenario %u failed", i);
		scenario.scenario = *preload.scenario;

		int64 solved_timestamp = cfg_file.GetNum(MISSION_SECTION_TIME, scenario.internal_name);
		if (solved_timestamp > 0) {
			scenario.solved = {
				cfg_file.GetValue(MISSION_SECTION_USER, scenario.internal_name),
				cfg_file.GetNum(MISSION_SECTION_VALUE, scenario.internal_name),
				solved_timestamp
			};
		}
	}

	rcd_file->CheckExactLength(length, 0, "end of block");

	mission->UpdateUnlockData();

	_missions.emplace_back(std::move(mission));
}

/** Update the information which scenarios are unlocked and save the data for solved missions to the config file. */
void Mission::UpdateUnlockData()
{
	/* Unlock as many scenarios as permitted. */
	if (this->max_unlock == 0) {
		for (MissionScenario &scenario : this->scenarios) scenario.required_to_unlock = 0;
	} else {
		int32 balance = 0;
		for (MissionScenario &scenario : this->scenarios) {
			if (scenario.solved.has_value()) {
				scenario.required_to_unlock = 0;
				balance--;
			} else {
				scenario.required_to_unlock = std::max(0, balance);
				balance++;
			}
		}
	}

	/* Save solved data to file. */
	ConfigFile &cfg_file = GetMissionConfigFile();
	ConfigSection &section_user  = cfg_file.GetCreateSection(MISSION_SECTION_USER);
	ConfigSection &section_time  = cfg_file.GetCreateSection(MISSION_SECTION_TIME);
	ConfigSection &section_value = cfg_file.GetCreateSection(MISSION_SECTION_VALUE);

	for (MissionScenario &scenario : this->scenarios) {
		if (scenario.solved.has_value()) {
			section_user .SetItem(scenario.internal_name, scenario.solved->user);
			section_time .SetItem(scenario.internal_name, std::to_string(scenario.solved->timestamp));
			section_value.SetItem(scenario.internal_name, std::to_string(scenario.solved->company_value));
		} else {
			section_user .RemoveItem(scenario.internal_name);
			section_time .RemoveItem(scenario.internal_name);
			section_value.RemoveItem(scenario.internal_name);
		}
	}

	cfg_file.Write(true);
}

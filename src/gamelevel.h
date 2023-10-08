/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gamelevel.h Game level. */

#ifndef GAME_LEVEL_H
#define GAME_LEVEL_H

#include <memory>
#include <string>
#include <vector>

#include "dates.h"
#include "money.h"
#include "loadsave.h"
#include "sprite_store.h"

struct Mission;
struct MissionScenario;

/** The policy how to interpret an objective's due date. */
enum ObjectiveTimeoutPolicy {
	TIMEOUT_NONE   = 0,  ///< The objective may be fulfilled at any time.
	TIMEOUT_EXACT  = 1,  ///< The objective has to be met at one specific point in time.
	TIMEOUT_BEFORE = 2,  ///< The objective has to be met at any time before the deadline.
};

/** Objective type constants for use in savegames. */
enum ObjectiveType {
	OJT_CONTAINER = 0,
	OJT_NONE,
	OJT_GUESTS,
	OJT_RATING,
};

/** Abstract representation of a scenario objective. */
class AbstractObjective {
public:
	/** How to behave when this objective is not met. */
	struct DropPolicy {
		explicit DropPolicy(uint32 d) : days_after_drop(d), drop_counter(0) {}
		uint32 days_after_drop;  ///< How many days the player has left to re-achieve this objective when it is not met. \c 0 means unlimited.
		uint32 drop_counter;     ///< How many days the objective has already been not met.
	};

	explicit AbstractObjective(uint32 d) : is_fulfilled(false), drop_policy(d) {}
	virtual ~AbstractObjective() = default;

	virtual std::string ToString() const = 0;
	virtual void OnNewDay();

	/**
	 * The type of this objective instance.
	 * @return The type.
	 * @note Every derived class must override this.
	 */
	virtual ObjectiveType Type() const = 0;
	virtual void Load(Loader &ldr);
	virtual void Save(Saver &svr);

	bool is_fulfilled;       ///< Whether this objective is currently met.
	DropPolicy drop_policy;  ///< How to behave when this objective is not met.
};

/** Objective to just have fun. */
class ObjectiveNone : public AbstractObjective {
public:
	ObjectiveNone() : AbstractObjective(0) {}

	std::string ToString() const override;

	inline ObjectiveType Type() const override
	{
		return OJT_NONE;
	}
	void Load(Loader &ldr) override;
	void Save(Saver &svr) override;
};

/** Objective to achieve a minimum number of guests in the park. */
class ObjectiveGuests : public AbstractObjective {
public:
	explicit ObjectiveGuests(uint32 d, uint32 g) : AbstractObjective(d), nr_guests(g) {}

	std::string ToString() const override;
	void OnNewDay() override;

	inline ObjectiveType Type() const override
	{
		return OJT_GUESTS;
	}
	void Load(Loader &ldr) override;
	void Save(Saver &svr) override;

	uint32 nr_guests;  ///< Number of guests to achieve.
};

/** Objective to achieve a minimum park rating. */
class ObjectiveParkRating : public AbstractObjective {
public:
	explicit ObjectiveParkRating(uint32 d, uint32 r) : AbstractObjective(d), rating(r) {}

	std::string ToString() const override;
	void OnNewDay() override;

	inline ObjectiveType Type() const override
	{
		return OJT_RATING;
	}
	void Load(Loader &ldr) override;
	void Save(Saver &svr) override;

	uint16 rating;  ///< Park rating to achieve.
};

/** Objective to achieve all of one or more objectives. */
class ScenarioObjective : public AbstractObjective {
public:
	ScenarioObjective(uint32, ObjectiveTimeoutPolicy, Date, std::vector<std::shared_ptr<AbstractObjective>>);

	std::string ToString() const override;
	void OnNewDay() override;

	inline ObjectiveType Type() const override
	{
		return OJT_CONTAINER;
	}
	void Load(Loader &ldr) override;
	void Save(Saver &svr) override;

	std::vector<std::shared_ptr<AbstractObjective>> objectives;  ///< The objectives to achieve.
	ObjectiveTimeoutPolicy timeout_policy;  ///< The timeout policy of this objective.
	Date timeout_date;                      ///< When this objective must be fulfilled.
};

/** %Scenario settings. */
struct Scenario {
	Scenario();

	void SetDefaultScenario();

	void Load(Loader &ldr);
	void Save(Saver &svr);

	uint GetSpawnProbability(int popularity);

	uint16 spawn_lowest;  ///< Guest spawn probability at lowest popularity (0..1024).
	uint16 spawn_highest; ///< Guest spawn probability at highest popularity (0..1024).
	uint32 max_guests;    ///< Maximal number of guests.

	std::string name;                              ///< Title of the scenario.
	std::string descr;                             ///< Description of the scenario.
	std::shared_ptr<ScenarioObjective> objective;  ///< The scenario's objective.

	Money max_loan;           ///< Maximum loan the player can take.
	uint16 interest;          ///< Annual interest rate in 0.1 percent over the current loan.
	bool allow_entrance_fee;  ///< Whether the player may set a park entrance fee.

	MissionScenario *wrapper;  ///< The metadata wrapper around this scenario (not owned).
};

/** Wrapper around a scenario in a mission plus metadata. */
struct MissionScenario {
	size_t fct_length;                   ///< Number of bytes in the scenario file.
	std::unique_ptr<uint8[]> fct_bytes;  ///< The bytes of the scenario file.

	Scenario scenario;                   ///< The wrapped scenario.
	StringID name;                       ///< String ID of the title of the scenario.
	StringID descr;                      ///< String ID of the description of the scenario.

	/** Data about the first time a scenario was solved. */
	struct Solved {
		std::string user;         ///< Name of the person who solved the scenario.
		Money company_value = 0;  ///< Company value at the end of the scenario.
		time_t timestamp = 0;     ///< Timestamp when the scenario was solved.
	};

	std::string internal_name;     ///< The scenario's internal name.
	std::optional<Solved> solved;  ///< Who solved this scenario and with what company value, if applicable.
	uint32 required_to_unlock;     ///< How many other scenarios must be solved to unlock this one (0 means it is unlocked).
	Mission *mission;              ///< The mission this scenario belongs to.
};

/** A %mission of several scenarios. */
struct Mission {
	std::vector<MissionScenario> scenarios;  ///< All scenarios in this mission.
	std::string internal_name;               ///< The mission's internal name.
	uint32 max_unlock;                       ///< Maximum number of unlocked unsolved scenarios.
	StringID name;                           ///< String ID of the title of the scenario.
	StringID descr;                          ///< String ID of the description of the scenario.

	void UpdateUnlockData();
};

void LoadMission(RcdFileReader *rcd_file, const TextMap &texts);

extern Scenario _scenario;
extern std::vector<std::unique_ptr<Mission>> _missions;

#endif

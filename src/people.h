/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file people.h Declarations for people in the world. */

#ifndef PEOPLE_H
#define PEOPLE_H

#include <list>
#include <map>

#include "person.h"

/**
 * All our guests.
 * @todo Allow to have several blocks of guests.
 */
class Guests {
public:
	Guests();

	void Uninitialize();

	void Load(Loader &ldr);
	void Save(Saver &svr);

	uint32 CountActiveGuests() const;
	uint32 CountGuestsInPark() const;

	Guest *GetExisting(int idx);
	const Guest *GetExisting(int idx) const;

	Guest *GetCreate(int idx);
	void NotifyGuestDeactivation(int idx);

	void OnAnimate(int delay);
	void DoTick();
	void OnNewDay();

	void NotifyRideDeletion(const RideInstance *);

	/** All the things guests like to complain about. */
	enum ComplaintType {
		COMPLAINT_HUNGER,     ///< A guest is hungry and doesn't know where to buy food.
		COMPLAINT_THIRST,     ///< A guest is thirsty and doesn't know where to buy a drink.
		COMPLAINT_WASTE,      ///< A guest needs a toilet and doesn't know where to find one.
		COMPLAINT_LITTER,     ///< The paths are very dirty.
		COMPLAINT_VANDALISM,  ///< Many park objects are demolished.

		COMPLAINT_COUNT       ///< Number of complaint types.
	};

	void Complain(ComplaintType t);

	Point16 start_voxel;  ///< Entry x/y coordinate of the voxel stack at the edge (negative X/Y coordinate means invalid).

private:
	Random rnd;           ///< Random number generator for creating new guests.
	int daily_frac;       ///< Frame counter.

	/** Holds statistics about guest complaints of a specific type. */
	struct Complaint {
		Complaint();

		uint16 counter;             ///< Counter for the number of complaints.
		uint32 time_since_message;  ///< Time in milliseconds since a message was last sent to the player.
	};
	Complaint complaints[COMPLAINT_COUNT];  ///< Statistics about all complaint types.

	std::vector<std::unique_ptr<Guest[]>> guests;  ///< All guest slots.
	std::vector<int> free_guest_indices;           ///< Unused indices in %guests.
};

/** All the staff (handymen, mechanics, entertainers, guards) in the park. */
class Staff {
public:
	Staff();
	~Staff();
	void Uninitialize();

	void RequestMechanic(RideInstance *ride);
	void NotifyRideDeletion(const RideInstance *);

	Mechanic    *HireMechanic();
	Handyman    *HireHandyman();
	Guard       *HireGuard();
	Entertainer *HireEntertainer();
	uint16 CountMechanics()    const;
	uint16 CountHandymen()     const;
	uint16 CountGuards()       const;
	uint16 CountEntertainers() const;
	uint16 Count(PersonType t) const;

	StaffMember *Get(PersonType t, uint list_index) const;
	void Dismiss(const StaffMember* m);

	void OnAnimate(int delay);
	void DoTick();
	void OnNewDay();
	void OnNewMonth();

	void Load(Loader &ldr);
	void Save(Saver &svr);

private:
	uint16 GenerateID();

	uint16 last_person_id;                                 ///< ID of the last staff member hired.
	std::list<RideInstance*> mechanic_requests;            ///< Rides in need of a mechanic.
	std::list<std::unique_ptr<Mechanic>>    mechanics;     ///< All mechanics    in the park.
	std::list<std::unique_ptr<Handyman>>    handymen;      ///< All handymen     in the park.
	std::list<std::unique_ptr<Guard>>       guards;        ///< All guards       in the park.
	std::list<std::unique_ptr<Entertainer>> entertainers;  ///< All entertainers in the park.
};

extern Guests _guests;
extern Staff _staff;

#endif

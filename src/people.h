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

static const int GUEST_BLOCK_SIZE = 512; ///< Number of guests in a block.

/** A block of guests. */
class GuestBlock {
public:
	GuestBlock(uint16 base_id);

	/**
	 * Get a guest from the array.
	 * @param i Index of the person (should be between \c 0 and #GUEST_BLOCK_SIZE).
	 * @return The requested person.
	 */
	inline Guest *Get(uint i)
	{
		assert(i < lengthof(this->guests));
		return &this->guests[i];
	}

	/**
	 * Get a guest from the array.
	 * @param i Index of the person (should be between \c 0 and #GUEST_BLOCK_SIZE).
	 * @return The requested person.
	 */
	inline const Guest *Get(uint i) const
	{
		assert(i < lengthof(this->guests));
		return &this->guests[i];
	}

	/**
	 * Get the index associated with a guest.
	 * @param g %Guest object to query.
	 * @return Index of the guest in the block.
	 */
	inline uint Index(Guest *g)
	{
		uint idx = g - this->guests;
		assert(idx < lengthof(this->guests));
		return idx;
	}

protected:
	Guest guests[GUEST_BLOCK_SIZE]; ///< Persons in the block.
};

/**
 * All our guests.
 * @todo Allow to have several blocks of guests.
 */
class Guests {
public:
	Guests();
	~Guests();

	void Uninitialize();

	void Load(Loader &ldr);
	void Save(Saver &svr);

	uint CountActiveGuests();
	uint CountGuestsInPark();

	/**
	 * Get a guest from the array.
	 * @param idx Index of the person (should be between \c 0 and #GUEST_BLOCK_SIZE).
	 * @return The requested person.
	 */
	inline Guest *Get(int idx)
	{
		return this->block.Get(idx);
	}

	/**
	 * Get a guest from the array.
	 * @param idx Index of the person (should be between \c 0 and #GUEST_BLOCK_SIZE).
	 * @return The requested person.
	 */
	inline const Guest *Get(int idx) const
	{
		return this->block.Get(idx);
	}

	void OnAnimate(int delay);
	void DoTick();
	void OnNewDay();

	void NotifyRideDeletion(const RideInstance *);

	Point16 start_voxel;  ///< Entry x/y coordinate of the voxel stack at the edge (negative X/Y coordinate means invalid).

private:
	GuestBlock block;     ///< The data of all actual guests.
	uint free_idx;        ///< All guests less than this index are active.
	Random rnd;           ///< Random number generator for creating new guests.
	int daily_frac;       ///< Frame counter.
	int next_daily_index; ///< Index of the next guest to give daily service.

	bool FindNextFreeGuest();
	bool FindNextFreeGuest() const;
	bool HasFreeGuests() const;
	void AddFree(Guest *g);
	Guest *GetFree();
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

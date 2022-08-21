/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file people.cpp People in the world. */

#include "stdafx.h"
#include "map.h"
#include "dates.h"
#include "math_func.h"
#include "geometry.h"
#include "messages.h"
#include "person_type.h"
#include "ride_type.h"
#include "path_finding.h"
#include "person.h"
#include "people.h"
#include "gamelevel.h"
#include "finances.h"
#include <limits>

Guests _guests; ///< %Guests in the world/park.
Staff _staff;   ///< %Staff in the world/park.

static const uint32 COMPLAINT_TIMEOUT = 8 * 60 * 1000;  ///< Time in milliseconds between two complaint notifications of the same type.
static const uint16 COMPLAINT_THRESHOLD[] = {  // Indexed by %Guests::ComplaintType.
		80,  ///< After how many hunger    complaints a notification is sent.
		80,  ///< After how many thirst    complaints a notification is sent.
		30,  ///< After how many waste     complaints a notification is sent.
		25,  ///< After how many litter    complaints a notification is sent.
		15,  ///< After how many vandalism complaints a notification is sent.
};
static const StringID COMPLAINT_MESSAGES[] = {  // Indexed by %Guests::ComplaintType.
		GUI_MESSAGE_COMPLAIN_HUNGRY,     ///< Message for complaints about lack of food.
		GUI_MESSAGE_COMPLAIN_THIRSTY,    ///< Message for complaints about lack of drink.
		GUI_MESSAGE_COMPLAIN_TOILET,     ///< Message for complaints about lack of toilets.
		GUI_MESSAGE_COMPLAIN_LITTER,     ///< Message for complaints about dirty paths.
		GUI_MESSAGE_COMPLAIN_VANDALISM,  ///< Message for complaints about demolished objects.
};
assert_compile(lengthof(COMPLAINT_THRESHOLD) == Guests::COMPLAINT_COUNT);
assert_compile(lengthof(COMPLAINT_MESSAGES ) == Guests::COMPLAINT_COUNT);

/** Constructor. */
Guests::Complaint::Complaint() : counter(0), time_since_message(COMPLAINT_TIMEOUT)
{
}

/**
 * Guest block constructor. Fills the id of the persons with an incrementing number.
 * @param base_id Id number of the first person in this block.
 */
GuestBlock::GuestBlock(uint16 base_id)
{
	for (uint i = 0; i < lengthof(this->guests); i++) {
		this->guests[i].id = base_id;
		base_id++;
	}
}

/**
 * Check that the voxel stack at the given coordinate is a good spot to use as entry point for new guests.
 * @param x X position at the edge.
 * @param y Y position at the edge.
 * @return The given position is a good starting point.
 */
static bool IsGoodEdgeRoad(int16 x, int16 y)
{
	if (x < 0 || y < 0) return false;
	int16 z = _world.GetBaseGroundHeight(x, y);
	const Voxel *vs = _world.GetVoxel(XYZPoint16(x, y, z));
	return vs && HasValidPath(vs) && GetImplodedPathSlope(vs) < PATH_FLAT_COUNT;
}

/**
 * Try to find a voxel at the edge of the world that can be used as entry point for guests.
 * @return The x/y coordinate at the edge of the world of a usable voxelstack, else an off-world coordinate.
 */
static Point16 FindEdgeRoad()
{
	int16 highest_x = _world.GetXSize() - 1;
	int16 highest_y = _world.GetYSize() - 1;
	for (int16 x = 1; x < highest_x; x++) {
		if (IsGoodEdgeRoad(x, 0))         return {x, 0};
		if (IsGoodEdgeRoad(x, highest_y)) return {x, highest_y};
	}
	for (int16 y = 1; y < highest_y; y++) {
		if (IsGoodEdgeRoad(0, y))         return {0, y};
		if (IsGoodEdgeRoad(highest_x, y)) return {highest_x, y};
	}

	return {-1, -1};
}

Guests::Guests()
: start_voxel(-1, -1), rnd(), block(0), free_idx(0), daily_frac(0), next_daily_index(0)
{
}

Guests::~Guests()
{
}

/** Deactivate all guests and reset variables. */
void Guests::Uninitialize()
{
	for (int i = 0; i < GUEST_BLOCK_SIZE; i++) {
		Guest *g = this->block.Get(i);
		if (g->IsActive()) {
			g->DeActivate(OAR_REMOVE);
			this->AddFree(g);
		}
	}
	this->start_voxel.x = -1;
	this->start_voxel.y = -1;
	this->daily_frac = 0;
	this->next_daily_index = 0;

	for (Complaint &c : this->complaints) c = Complaint();
}

static const uint32 CURRENT_VERSION_GSTS = 2;   ///< Currently supported version of the GSTS Pattern.

/**
 * Load guests from the save game.
 * @param ldr Input stream to read.
 */
void Guests::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("GSTS");
	switch (version) {
		case 0:
			break;
		case 1:
		case 2:
			this->start_voxel.x = ldr.GetWord();
			this->start_voxel.y = ldr.GetWord();
			this->daily_frac = ldr.GetWord();
			this->next_daily_index = ldr.GetWord();
			this->free_idx = ldr.GetLong();

			if (version > 1) {
				for (Complaint &c : this->complaints) c.counter = ldr.GetWord();
				for (Complaint &c : this->complaints) c.time_since_message = ldr.GetLong();
			}

			for (long i = ldr.GetLong(); i > 0; i--) {
				Guest *g = this->block.Get(ldr.GetWord());
				g->Load(ldr);
			}
			break;

		default:
			ldr.version_mismatch(version, CURRENT_VERSION_GSTS);
	}
	ldr.ClosePattern();
}

/**
 * Save guests to the save game.
 * @param svr Output stream to save to.
 */
void Guests::Save(Saver &svr)
{
	svr.CheckNoOpenPattern();
	svr.StartPattern("GSTS", CURRENT_VERSION_GSTS);
	svr.PutWord(this->start_voxel.x);
	svr.PutWord(this->start_voxel.y);
	svr.PutWord(this->daily_frac);
	svr.PutWord(this->next_daily_index);
	svr.PutLong(this->free_idx);

	for (const Complaint &c : this->complaints) svr.PutWord(c.counter);
	for (const Complaint &c : this->complaints) svr.PutLong(c.time_since_message);

	svr.PutLong(this->CountActiveGuests());
	for (uint i = 0; i < GUEST_BLOCK_SIZE; i++) {
		Guest *g = this->block.Get(i);
		if (g->IsActive()) {
			svr.PutWord(g->id);
			g->Save(svr);
		}
	}
	svr.EndPattern();
}

/**
 * Update #free_idx to the next free guest (if available).
 * @return Whether a free guest was found.
 */
bool Guests::FindNextFreeGuest()
{
	while (this->free_idx < GUEST_BLOCK_SIZE) {
		Guest *g = this->block.Get(this->free_idx);
		if (!g->IsActive()) return true;
		this->free_idx++;
	}
	return false;
}

/**
 * Update #free_idx to the next free guest (if available).
 * @return Whether a free guest was found.
 */
bool Guests::FindNextFreeGuest() const
{
	uint idx = this->free_idx;
	while (idx < GUEST_BLOCK_SIZE) {
		const Guest *g = this->block.Get(idx);
		if (!g->IsActive()) return true;
		idx++;
	}
	return false;
}

/**
 * Count the number of active guests.
 * @return The number of active guests.
 */
uint Guests::CountActiveGuests()
{
	uint count = this->free_idx;
	for (uint i = this->free_idx; i < GUEST_BLOCK_SIZE; i++) {
		Guest *g = this->block.Get(i);
		if (g->IsActive()) count++;
	}
	return count;
}

/**
 * Count the number of guests in the park.
 * @return The number of guests in the park.
 */
uint Guests::CountGuestsInPark()
{
	uint count = 0;
	for (uint i = 0; i < GUEST_BLOCK_SIZE; i++) {
		Guest *g = this->block.Get(i);
		if (g->IsActive() && g->IsInPark()) count++;
	}
	return count;
}

/**
 * Some time has passed, update the animation.
 * @param delay Number of milliseconds time that have past since the last animation update.
 */
void Guests::OnAnimate(int delay)
{
	for (Complaint &c : this->complaints) c.time_since_message += delay;

	for (int i = 0; i < GUEST_BLOCK_SIZE; i++) {
		Guest *p = this->block.Get(i);
		if (!p->IsActive()) continue;

		AnimateResult ar = p->OnAnimate(delay);
		if (ar != OAR_OK) {
			p->DeActivate(ar);
			this->AddFree(p);
		}
	}
}

/** A new frame arrived, perform the daily call for some of the guests. */
void Guests::DoTick()
{
	this->daily_frac++;
	int end_index = std::min(this->daily_frac * GUEST_BLOCK_SIZE / TICK_COUNT_PER_DAY, GUEST_BLOCK_SIZE);
	while (this->next_daily_index < end_index) {
		Guest *p = this->block.Get(this->next_daily_index);
		if (p->IsActive() && !p->DailyUpdate()) {
			p->DeActivate(OAR_REMOVE);
			this->AddFree(p);
		}
		this->next_daily_index++;
	}
	if (this->next_daily_index >= GUEST_BLOCK_SIZE) {
		this->daily_frac = 0;
		this->next_daily_index = 0;
	}
}

/**
 * A new day arrived, handle daily chores of the park.
 * @todo Add popularity rating concept.
 */
void Guests::OnNewDay()
{
	/* Gradually decrease complaint levels to prevent accumulation over very long times. */
	for (Complaint &c : this->complaints) {
		if (c.counter > 0) c.counter--;
	}

	/* Try adding a new guest to the park. */
	if (this->CountActiveGuests() >= _scenario.max_guests) return;
	if (!this->rnd.Success1024(_scenario.GetSpawnProbability(512))) return;

	if (!IsGoodEdgeRoad(this->start_voxel.x, this->start_voxel.y)) {
		/* New guest, but no road. */
		this->start_voxel = FindEdgeRoad();
		if (!IsGoodEdgeRoad(this->start_voxel.x, this->start_voxel.y)) return;
	}

	if (!this->HasFreeGuests()) return; // No more quests available.
	/* New guest! */
	Guest *g = this->GetFree();
	g->Activate(this->start_voxel, PERSON_GUEST);
}

/**
 * Notification that the ride is being removed.
 * @param ri Ride being removed.
 */
void Guests::NotifyRideDeletion(const RideInstance *ri) {
	for (int i = 0; i < GUEST_BLOCK_SIZE; i++) {
		Guest *p = this->block.Get(i);
		if (!p->IsActive()) continue;

		p->NotifyRideDeletion(ri);
	}
}

/**
 * A guest complains about something.
 * May send a message to the player.
 * @param type Subject of the complaint.
 */
void Guests::Complain(const ComplaintType type)
{
	assert(type < COMPLAINT_COUNT);
	Complaint &c = this->complaints[type];
	c.counter++;
	if (c.time_since_message > COMPLAINT_TIMEOUT && c.counter >= COMPLAINT_THRESHOLD[type]) {
		c.counter = 0;
		c.time_since_message = 0;
		_inbox.SendMessage(new Message(COMPLAINT_MESSAGES[type]));
	}
}

/**
 * Return whether there are still non-active guests.
 * @return \c true if there are non-active guests, else \c false.
 */
bool Guests::HasFreeGuests() const
{
	return this->FindNextFreeGuest();
}

/**
 * Add a guest to the non-active list.
 * @param g %Guest to add.
 */
void Guests::AddFree(Guest *g)
{
	this->free_idx = std::min(this->block.Index(g), this->free_idx);
}

/**
 * Get a non-active guest.
 * @return A non-active guest.
 * @pre #HasFreeGuests() should hold.
 */
Guest *Guests::GetFree()
{
#ifndef NDEBUG
	assert(this->FindNextFreeGuest());
#else
	this->FindNextFreeGuest();
#endif

	Guest *g = this->block.Get(this->free_idx);
	this->free_idx++;
	return g;
}

Staff::Staff()
{
	/* Nothing to do currently. */
}

Staff::~Staff()
{
	/* Nothing to do currently. */
}

static const uint16 STAFF_BASE_ID = std::numeric_limits<uint16>::max();  // Counting staff IDs backwards to avoid conflicts with %Guests.

/** Remove all staff and reset all variables. */
void Staff::Uninitialize()
{
	this->mechanics.clear();  // Do this first, it may generate new requests.
	this->handymen.clear();
	this->guards.clear();
	this->entertainers.clear();
	this->mechanic_requests.clear();
	this->last_person_id = STAFF_BASE_ID;
}

static const uint32 CURRENT_VERSION_STAF = 3;   ///< Currently supported version of the STAF Pattern.

/**
 * Load staff from the save game.
 * @param ldr Input stream to read.
 */
void Staff::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("STAF");
	switch (version) {
		case 0:
			break;
		case 1:
		case 2:
		case 3:
			if (version >= 3){
				this->last_person_id = ldr.GetWord();
			}
			for (uint i = ldr.GetLong(); i > 0; i--) {
				this->mechanic_requests.push_back(_rides_manager.GetRideInstance(ldr.GetWord()));
			}
			if (version >= 2) {
				for (uint i = ldr.GetLong(); i > 0; i--) {
					Mechanic *m = new Mechanic;
					m->Load(ldr);
					this->mechanics.push_back(std::unique_ptr<Mechanic>(m));
				}
			}
			if (version >= 3) {
				for (uint i = ldr.GetLong(); i > 0; i--) {
					Handyman *m = new Handyman;
					m->Load(ldr);
					this->handymen.push_back(std::unique_ptr<Handyman>(m));
				}
				for (uint i = ldr.GetLong(); i > 0; i--) {
					Guard *m = new Guard;
					m->Load(ldr);
					this->guards.push_back(std::unique_ptr<Guard>(m));
				}
				for (uint i = ldr.GetLong(); i > 0; i--) {
					Entertainer *m = new Entertainer;
					m->Load(ldr);
					this->entertainers.push_back(std::unique_ptr<Entertainer>(m));
				}
			}
			break;
		default:
			ldr.version_mismatch(version, CURRENT_VERSION_STAF);
	}
	ldr.ClosePattern();
}

/**
 * Save staff to the save game.
 * @param svr Output stream to save to.
 */
void Staff::Save(Saver &svr)
{
	svr.CheckNoOpenPattern();
	svr.StartPattern("STAF", CURRENT_VERSION_STAF);
	svr.PutWord(this->last_person_id);
	svr.PutLong(this->mechanic_requests.size());
	for (RideInstance *ride : this->mechanic_requests) svr.PutWord(ride->GetIndex());
	svr.PutLong(this->mechanics.size());
	for (auto &m : this->mechanics) m->Save(svr);
	svr.PutLong(this->handymen.size());
	for (auto &m : this->handymen) m->Save(svr);
	svr.PutLong(this->guards.size());
	for (auto &m : this->guards) m->Save(svr);
	svr.PutLong(this->entertainers.size());
	for (auto &m : this->entertainers) m->Save(svr);
	svr.EndPattern();
}

/**
 * Generates a unique ID for a newly hired staff member.
 * @return The ID to use.
 */
uint16 Staff::GenerateID()
{
	return --last_person_id;
}

/**
 * Request that a mechanic should inspect or repair a ride as soon as possible.
 * @param ride Ride to inspect or repair.
 */
void Staff::RequestMechanic(RideInstance *ride)
{
	mechanic_requests.push_back(ride);
}

/**
 * Generate the name for a newly hired staff member.
 * @oaram m Staff member to name.
 * @param text Base text for the name.
 */
static void NameNewStaff(StaffMember *m, StringID text)
{
	static char buffer[1024];
	snprintf(buffer, lengthof(buffer), _language.GetSgText(text).c_str(), STAFF_BASE_ID - m->id);
	m->SetName(buffer);
}

/**
 * Hire a new mechanic.
 * @return The new mechanic.
 */
Mechanic *Staff::HireMechanic()
{
	Mechanic *m = new Mechanic;
	m->id = this->GenerateID();
	m->Activate(Point16(9, 2), PERSON_MECHANIC);  // \todo Allow the player to decide where to put the new mechanic.
	this->mechanics.push_back(std::unique_ptr<Mechanic>(m));
	NameNewStaff(m, GUI_STAFF_NAME_MECHANIC);
	return m;
}

/**
 * Hire a new handyman.
 * @return The new handyman.
 */
Handyman *Staff::HireHandyman()
{
	Handyman *m = new Handyman;
	m->id = this->GenerateID();
	m->Activate(Point16(9, 2), PERSON_HANDYMAN);  // \todo Allow the player to decide where to put the new handyman.
	this->handymen.push_back(std::unique_ptr<Handyman>(m));
	NameNewStaff(m, GUI_STAFF_NAME_HANDYMAN);
	return m;
}

/**
 * Hire a new security guard.
 * @return The new guard.
 */
Guard *Staff::HireGuard()
{
	Guard *m = new Guard;
	m->id = this->GenerateID();
	m->Activate(Point16(9, 2), PERSON_GUARD);  // \todo Allow the player to decide where to put the new guard.
	this->guards.push_back(std::unique_ptr<Guard>(m));
	NameNewStaff(m, GUI_STAFF_NAME_GUARD);
	return m;
}

/**
 * Hire a new entertainer.
 * @return The new entertainer.
 */
Entertainer *Staff::HireEntertainer()
{
	Entertainer *m = new Entertainer;
	m->id = this->GenerateID();
	m->Activate(Point16(9, 2), PERSON_ENTERTAINER);  // \todo Allow the player to decide where to put the new entertainer.
	this->entertainers.push_back(std::unique_ptr<Entertainer>(m));
	NameNewStaff(m, GUI_STAFF_NAME_ENTERTAINER);
	return m;
}

/**
 * Returns the number of currently employed mechanics in the park.
 * @return Number of mechanics.
 */
uint16 Staff::CountMechanics() const
{
	return this->mechanics.size();
}

/**
 * Returns the number of currently employed handymen in the park.
 * @return Number of handymen.
 */
uint16 Staff::CountHandymen() const
{
	return this->handymen.size();
}

/**
 * Returns the number of currently employed guards in the park.
 * @return Number of guards.
 */
uint16 Staff::CountGuards() const
{
	return this->guards.size();
}

/**
 * Returns the number of currently employed entertainers in the park.
 * @return Number of entertainers.
 */
uint16 Staff::CountEntertainers() const
{
	return this->entertainers.size();
}

/**
 * Returns the number of currently employed staff of a given type in the park.
 * @param t Type of staff (use \c PERSON_ANY for all).
 * @return Number of staff.
 */
uint16 Staff::Count(const PersonType t) const
{
	switch (t) {
		case PERSON_MECHANIC:    return this->CountMechanics();
		case PERSON_HANDYMAN:    return this->CountHandymen();
		case PERSON_GUARD:       return this->CountGuards();
		case PERSON_ENTERTAINER: return this->CountEntertainers();

		case PERSON_ANY: return (this->CountMechanics() + this->CountHandymen() + this->CountGuards() + this->CountEntertainers());
		default: NOT_REACHED();
	}
}

/**
 * Get a staff member of given type.
 * @param t Type of staff.
 * @param list_index The index of this person in its respective category.
 * @return The person.
 */
StaffMember *Staff::Get(const PersonType t, const uint list_index) const
{
	switch (t) {
		case PERSON_MECHANIC:    { auto it = this->mechanics.begin();    std::advance(it, list_index); return it->get(); }
		case PERSON_HANDYMAN:    { auto it = this->handymen.begin();     std::advance(it, list_index); return it->get(); }
		case PERSON_GUARD:       { auto it = this->guards.begin();       std::advance(it, list_index); return it->get(); }
		case PERSON_ENTERTAINER: { auto it = this->entertainers.begin(); std::advance(it, list_index); return it->get(); }
		default: NOT_REACHED();
	}
}

/**
 * Dismiss a staff member from the staff.
 * @param person Person to dismiss.
 * @note Invalidates the pointer.
 */
void Staff::Dismiss(const StaffMember *person)
{
	switch (person->type) {
		case PERSON_MECHANIC:
			for (auto it = this->mechanics.begin(); it != this->mechanics.end(); it++) {
				if (it->get() == person) {
					this->mechanics.erase(it);  // This deletes the mechanic.
					return;
				}
			}
			break;
		case PERSON_HANDYMAN:
			for (auto it = this->handymen.begin(); it != this->handymen.end(); it++) {
				if (it->get() == person) {
					this->handymen.erase(it);  // This deletes the handyman.
					return;
				}
			}
			break;
		case PERSON_GUARD:
			for (auto it = this->guards.begin(); it != this->guards.end(); it++) {
				if (it->get() == person) {
					this->guards.erase(it);  // This deletes the guard.
					return;
				}
			}
			break;
		case PERSON_ENTERTAINER:
			for (auto it = this->entertainers.begin(); it != this->entertainers.end(); it++) {
				if (it->get() == person) {
					this->entertainers.erase(it);  // This deletes the entertainer.
					return;
				}
			}
			break;

		default: NOT_REACHED();
	}
	NOT_REACHED();
}

/**
 * Notification that the ride is being removed.
 * @param ri Ride being removed.
 */
void Staff::NotifyRideDeletion(const RideInstance *ri) {
	for (auto &m : this->mechanics) m->NotifyRideDeletion(ri);
}

/**
 * Some time has passed, update the animation.
 * @param delay Number of milliseconds time that have past since the last animation update.
 */
void Staff::OnAnimate(const int delay)
{
	for (auto &m : this->mechanics   ) m->OnAnimate(delay);
	for (auto &m : this->handymen    ) m->OnAnimate(delay);
	for (auto &m : this->guards      ) m->OnAnimate(delay);
	for (auto &m : this->entertainers) m->OnAnimate(delay);
}

/** A new frame arrived. */
void Staff::DoTick()
{
	/* Assign one mechanic request to the nearest available mechanic, if any. */
	if (!this->mechanic_requests.empty() && !this->mechanics.empty()) {
		EdgeCoordinate destination = this->mechanic_requests.front()->GetMechanicEntrance();
		destination.coords.x += _tile_dxy[destination.edge].x;
		destination.coords.y += _tile_dxy[destination.edge].y;

		Mechanic *best = nullptr;
		uint32 distance = 0;
		for (auto &m : this->mechanics) {
			if (m->ride != nullptr) continue;

			PathSearcher ps(m->vox_pos);
			XYZPoint16 p(destination.coords);
			ps.AddStart(p);
			p.z--;
			ps.AddStart(p);  // In case the path leading to the mechanic entrance is sloping upwards.

			if (!ps.Search()) continue;  // No path exists.
			uint32 d = 0;
			for (const WalkedPosition *it = ps.dest_pos; it->prev_pos != nullptr; it = it->prev_pos) d++;

			if (best == nullptr || d < distance) {
				best = m.get();
				distance = d;
			}
		}
		if (best != nullptr) {
			best->Assign(this->mechanic_requests.front());
			this->mechanic_requests.pop_front();
		}
	}
}

/** A new day arrived. */
void Staff::OnNewDay()
{
	/* Pay the wages for all employees. */
	for (PersonType t : {PERSON_MECHANIC, PERSON_HANDYMAN, PERSON_GUARD, PERSON_ENTERTAINER}) {
		_finances_manager.PayStaffWages(StaffMember::SALARY.at(t) * static_cast<int64>(this->Count(t)));
	}
}

/** A new month arrived. */
void Staff::OnNewMonth()
{
	/* Nothing to do currently. */
}

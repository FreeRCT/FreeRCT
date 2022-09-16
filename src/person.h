/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file person.h Persons in the park. */

#ifndef PERSON_H
#define PERSON_H

#include "map.h"
#include "random.h"
#include "money.h"
#include "ride_type.h"

class PathObjectInstance;
struct WalkInformation;
class RideInstance;

/**
 * Limits that exist at the tile.
 *
 * There are four limits in X direction (NE of tile, low x limit, high x limit, and SW of tile),
 * and four limits in Y direction (NW of tile, low y limit, high y limit, and SE of tile). Low and
 * high is created by means of a random offset from the centre, to prevent all guests from walking
 * at a single line.
 *
 * Since you can walk the tile in two directions (incrementing x/y or decrementing x/y), the middle
 * limits have a below/above notion as well.
 */
enum WalkLimit {
	WLM_MINIMAL = 0, ///< Continue until reached minimal value.
	WLM_LOW     = 1, ///< Continue until reached low value.
	WLM_CENTER  = 2, ///< Continue until reached centre value.
	WLM_HIGH    = 3, ///< Continue until reached high value.
	WLM_MAXIMAL = 4, ///< Continue until reached maximal value.
	WLM_INVALID = 7, ///< Invalid limit.

	WLM_LIMIT_LENGTH = 3, ///< Length of the limits in bits.

	WLM_X_START   = 0, ///< Destination position of X axis.
	WLM_Y_START   = 3, ///< Destination position of Y axis.
	WLM_END_LIMIT = 6, ///< Bit deciding which axis is the end-condition (\c 0 means #WLM_X_START, \c 1 means #WLM_Y_START).

	WLM_X_COND = 0,                  ///< X limit decides the end of this walk.
	WLM_Y_COND = 1 << WLM_END_LIMIT, ///< Y limit decides the end of this walk.

	WLM_NE_EDGE = WLM_MINIMAL | (WLM_INVALID << WLM_Y_START) | WLM_X_COND, ///< Continue until the north-east edge.
	WLM_LOW_X   = WLM_LOW     | (WLM_INVALID << WLM_Y_START) | WLM_X_COND, ///< Continue until we at the low x limit of the tile.
	WLM_MID_X   = WLM_CENTER  | (WLM_INVALID << WLM_Y_START) | WLM_X_COND, ///< Continue until we at the centre x limit of the tile.
	WLM_HIGH_X  = WLM_HIGH    | (WLM_INVALID << WLM_Y_START) | WLM_X_COND, ///< Continue until we at the high x limit of the tile.
	WLM_SW_EDGE = WLM_MAXIMAL | (WLM_INVALID << WLM_Y_START) | WLM_X_COND, ///< Continue until the south-west edge.

	WLM_NW_EDGE = WLM_INVALID | (WLM_MINIMAL << WLM_Y_START) | WLM_Y_COND, ///< Continue until the north-west edge.
	WLM_LOW_Y   = WLM_INVALID | (WLM_LOW     << WLM_Y_START) | WLM_Y_COND, ///< Continue until we at the low y limit of the tile.
	WLM_MID_Y   = WLM_INVALID | (WLM_CENTER  << WLM_Y_START) | WLM_Y_COND, ///< Continue until we at the centre y limit of the tile.
	WLM_HIGH_Y  = WLM_INVALID | (WLM_HIGH    << WLM_Y_START) | WLM_Y_COND, ///< Continue until we at the high y limit of the tile.
	WLM_SE_EDGE = WLM_INVALID | (WLM_MAXIMAL << WLM_Y_START) | WLM_Y_COND, ///< Continue until the south-east edge.

	WLM_NE_CENTER = WLM_MINIMAL | (WLM_CENTER  << WLM_Y_START) | WLM_X_COND, ///< Continue until the south-west edge (centre).
	WLM_SE_CENTER = WLM_CENTER  | (WLM_MAXIMAL << WLM_Y_START) | WLM_Y_COND, ///< Continue until the south-east edge (centre).
	WLM_SW_CENTER = WLM_MAXIMAL | (WLM_CENTER  << WLM_Y_START) | WLM_X_COND, ///< Continue until the south-west edge (centre).
	WLM_NW_CENTER = WLM_CENTER  | (WLM_MINIMAL << WLM_Y_START) | WLM_Y_COND, ///< Continue until the north-west edge (centre).
};

/** Walk animation to use to walk a part of the tile. */
struct WalkInformation {
	AnimationType anim_type; ///< Animation to display.
	uint8 limit_type;        ///< Limit to end use of this animation. @see WalkLimit
};

/** Exit codes of the Person::OnAnimate call. */
enum AnimateResult {
	OAR_CONTINUE,   ///< No result yet, continue the routine.
	OAR_OK,         ///< All OK, keep running.
	OAR_HALT,       ///< The person must stay where it is. Freeze the current animation.
	OAR_ANIMATING,  ///< The person stays where it is and displays a new animation.
	OAR_REMOVE,     ///< Remove person from the person-list, and de-activate.
	OAR_DEACTIVATE, ///< Person is already removed from the person-list, only de-activate.
};

/** Desire to visit a ride. */
enum RideVisitDesire {
	RVD_NO_RIDE,    ///< There is no ride here (used to distinguish between paths and rides).
	RVD_NO_VISIT,   ///< Person does not want to visit the ride.
	RVD_MAY_VISIT,  ///< Person may want to visit the ride.
	RVD_MUST_VISIT, ///< Person wants to visit the ride.
};

/**
 * Class of a person in the world.
 *
 * Persons are stored in contiguous blocks of memory, which makes the constructor and destructor useless.
 * Instead, \c Activate and \c DeActivate methods are used for this purpose. The #type variable controls the state of the entry.
 */
class Person : public VoxelObject {
public:
	Person();
	virtual ~Person() override;

	ScaledImage GetSprite(ViewOrientation orient, int zoom, const Recolouring **recolour) const override;

	virtual AnimateResult OnAnimate(int delay);
	virtual bool DailyUpdate() = 0;

	virtual void Activate(const Point16 &start, PersonType person_type);
	virtual void DeActivate(AnimateResult ar);

	void Load(Loader &ldr);
	void Save(Saver &svr);

	/**
	 * Test whether this person is active in the game or not.
	 * @return Whether the person is active in the game.
	 */
	bool IsActive() const
	{
		return this->type != PERSON_INVALID;
	}

	/**
	 * Test whether this person is a guest or not.
	 * @return Whether the person is a guest.
	 */
	bool IsGuest() const
	{
		return this->type == PERSON_GUEST;
	}

	bool IsQueuingGuest() const;
	const Person *GetQueuingGuestNearby(const XYZPoint16& vox_pos, const XYZPoint16& pix_pos, bool only_in_front);

	/**
	 * Test whether this person type treats queue paths like normal paths.
	 * @return This person doesn't treat queue paths specially.
	 */
	virtual bool WalksOnQueuePaths() const = 0;

	void SetName(const std::string &name);
	std::string GetName() const;
	std::string GetStatus() const;

protected:
	Random rnd; ///< Random number generator for deciding how the person reacts.

public:
	uint16 id;       ///< Unique id of the person.
	PersonType type; ///< Type of person.
	int16 offset;    ///< Offset with respect to centre of paths walked on (0..100).

	const WalkInformation *walk;  ///< Walk animation sequence being performed.
	const AnimationFrame *frames; ///< Animation frames of the current animation.
	uint16 frame_count;           ///< Number of frames in #frames.
	uint16 frame_index;           ///< Currently displayed frame of #frames.
	int16 frame_time;             ///< Remaining display time of this frame.
	Recolouring recolour;         ///< Person recolouring.
	RideInstance *ride;           ///< The ride we're intending to interact with, if any.

protected:
	std::string name; ///< Name of the person. \c "" means it has a default name (like "Guest XYZ").
	StringID status;  ///< What the person is doing right now, for display in the GUI.
	const Person *queuing_blocked_on;  ///< The person standing before this one in the queue, if this is a queuing guest.

	TileEdge GetCurrentEdge() const;
	uint8 GetInparkDirections();

	virtual void DecideMoveDirection() = 0;
	void DecideMoveDirectionOnPathlessLand(Voxel *former_voxel, const XYZPoint16 &former_vox_pos, const TileEdge exit_edge, const int dx, const int dy, const int dz);
	void StartAnimation(const WalkInformation *walk);
	virtual AnimateResult ActionAnimationCallback() = 0;

	RideVisitDesire ComputeExitDesire(TileEdge current_edge, XYZPoint16 cur_pos, TileEdge exit_edge, bool *seen_wanted_ride);
	virtual RideVisitDesire WantToVisit(const RideInstance *ri, const XYZPoint16 &ride_pos, TileEdge exit_edge) = 0;
	virtual AnimateResult EdgeOfWorldOnAnimate() = 0;
	virtual AnimateResult VisitRideOnAnimate(RideInstance *ri, TileEdge exit_edge) = 0;
	virtual AnimateResult InteractWithPathObject(PathObjectInstance *obj);
	virtual bool IsLeavingPath() const;
	void UpdateZPosition();
	void SetStatus(StringID s);
	bool HasCyclicQueuingDependency() const;
};

/** Activities of the guest. */
enum GuestActivity {
	GA_ENTER_PARK, ///< Find the entrance to the park.
	GA_WANDER,     ///< Wander around in the park.
	GA_QUEUING,    ///< Guest is queuing for a ride.
	GA_ON_RIDE,    ///< Guest is on the ride.
	GA_GO_HOME,    ///< Find a way to home.
	GA_RESTING,    ///< Sitting on a bench.
};

/** %Guests walking around in the world. */
class Guest : public Person {
public:
	Guest();
	~Guest();

	void Activate(const Point16 &start, PersonType person_type) override;
	void DeActivate(AnimateResult ar) override;

	void InitRidePreferences();

	void Load(Loader &ldr);
	void Save(Saver &svr);

	/**
	 * Is the guest in the park?
	 * @return Whether the guest is in the park.
	 * @todo Split #GA_GO_HOME into LEAVING_PARK and FINDING_EDGE for better estimation.
	 */
	bool IsInPark() const
	{
		return this->activity != GA_ENTER_PARK && this->activity != GA_GO_HOME;
	}

	AnimateResult OnAnimate(int delay) override;
	bool DailyUpdate() override;
	AnimateResult ActionAnimationCallback() override;

	void ChangeHappiness(int16 amount);
	ItemType SelectItem(const RideInstance *ri);
	void BuyItem(RideInstance *ri);
	void NotifyRideDeletion(const RideInstance *ri);
	void ExitRide(RideInstance *ri, TileEdge entry);
	void ExpelFromBench();
	bool WalksOnQueuePaths() const override
	{
		return false;
	}

	GuestActivity activity; ///< Activity being done by the guest currently.
	int16 happiness;        ///< Happiness of the guest (values are 0-100). Use #ChangeHappiness to change the guest happiness.
	uint16 total_happiness; ///< Sum of all good experiences (for evaluating the day after getting home, values are 0-1000).
	Money cash;             ///< Amount of money carried by the guest (should be non-negative).
	Money cash_spent;       ///< Amount of money spent by the guest (should be non-negative).

	/* Possessions of the guest. */
	bool has_map;        ///< Whether guest has a park map.
	bool has_umbrella;   ///< Whether guest has an umbrella.
	bool has_wrapper;    ///< Guest has a wrapper for the food or drink.
	bool has_balloon;    ///< Guest has a balloon.
	bool salty_food;     ///< The food in #food is salty.
	uint8 souvenirs;     ///< Number of souvenirs bought by the guest.
	int8 food;           ///< Amount of food in the hand (one unit/day).
	int8 drink;          ///< Amount of drink in the hand (one unit/day).
	uint8 hunger_level;  ///< Amount of hunger (higher means more hunger).
	uint8 thirst_level;  ///< Amount of thirst (higher means more thirst).
	uint8 stomach_level; ///< Amount of food/drink in the stomach.
	uint8 waste;         ///< Amount of food/drink waste that should be disposed.
	uint8 nausea;        ///< Amount of nausea of the guest.

	uint32 preferred_ride_intensity;   ///< Favourite ride intensity rating.
	uint32 min_ride_intensity;         ///< Lowest tolerated ride intensity rating.
	uint32 max_ride_intensity;         ///< Highest tolerated ride intensity rating.
	uint32 max_ride_nausea;            ///< Highest tolerated ride nausea rating.
	uint32 min_ride_excitement;        ///< Lowest tolerated ride excitement rating.

protected:
	void DecideMoveDirection() override;
	uint8 GetExitDirections(const Voxel *v, TileEdge start_edge, bool *seen_wanted_ride, bool *queue_mode);
	RideVisitDesire WantToVisit(const RideInstance *ri, const XYZPoint16 &ride_pos, TileEdge exit_edge) override;
	AnimateResult EdgeOfWorldOnAnimate() override;
	AnimateResult VisitRideOnAnimate(RideInstance *ri, TileEdge exit_edge) override;
	AnimateResult InteractWithPathObject(PathObjectInstance *obj) override;
	const WalkInformation *WalkForActivity(const WalkInformation **walks, uint8 walk_count, uint8 exits);

	RideVisitDesire NeedForItem(enum ItemType it, bool use_random);
	void AddItem(ItemType it);
};

/** A staff member: Mechanics, handymen, guards, entertainers. */
class StaffMember : public Person {
public:
	StaffMember() = default;

	void Load(Loader &ldr);
	void Save(Saver &svr);

	bool DailyUpdate() override;
	AnimateResult EdgeOfWorldOnAnimate() override;
	void DecideMoveDirection() override;
	AnimateResult VisitRideOnAnimate(RideInstance *ri, TileEdge exit_edge) override;
	RideVisitDesire WantToVisit(const RideInstance *ri, const XYZPoint16 &ride_pos, TileEdge exit_edge) override;

	static const std::map<PersonType, Money> SALARY;   ///< The monthly salary for each staff member.
};

/** A mechanic who can repair and inspect rides. */
class Mechanic : public StaffMember {
public:
	Mechanic() = default;
	~Mechanic();

	void Load(Loader &ldr);
	void Save(Saver &svr);

	void DecideMoveDirection() override;
	void Assign(RideInstance *ri);
	void NotifyRideDeletion(const RideInstance *ri);

	AnimateResult VisitRideOnAnimate(RideInstance *ri, TileEdge exit_edge) override;
	RideVisitDesire WantToVisit(const RideInstance *ri, const XYZPoint16 &ride_pos, TileEdge exit_edge) override;
	AnimateResult ActionAnimationCallback() override;
	bool WalksOnQueuePaths() const override
	{
		return false;
	}
};

/** A guard who walks around the park to stop guests from smashing up park property. */
class Guard : public StaffMember {
public:
	Guard() = default;

	void Load(Loader &ldr);
	void Save(Saver &svr);

	AnimateResult ActionAnimationCallback() override
	{
		NOT_REACHED();  // Guards don't have action animations.
	}
	bool WalksOnQueuePaths() const override
	{
		return true;
	}
};

/**
 * An entertainer who walks around the park to make guests more cheerful and patient.
 * @todo Happiness and impatience are not implemented yet, so the entertainers don't actually have a function currently.
 */
class Entertainer : public StaffMember {
public:
	Entertainer() = default;

	void Load(Loader &ldr);
	void Save(Saver &svr);

	AnimateResult ActionAnimationCallback() override
	{
		NOT_REACHED();  // Entertainers don't have action animations.
	}
	bool WalksOnQueuePaths() const override
	{
		return true;
	}
};

/** A handyman who walks around the park and waters the flowerbeds, empties the litter bins and sweeps the paths. */
class Handyman : public StaffMember {
public:
	/** What a handyman is doing right now. */
	enum class HandymanActivity {
		WANDER,    ///< Wandering around.
		WATER,     ///< Watering a flowerbed.
		SWEEP,     ///< Sweeping the path.
		EMPTY_NE,  ///< Emptying the bin on the north-east edge of the current tile.
		EMPTY_SE,  ///< Emptying the bin on the south-east edge of the current tile.
		EMPTY_SW,  ///< Emptying the bin on the south-west edge of the current tile.
		EMPTY_NW,  ///< Emptying the bin on the north-west edge of the current tile.
		HEADING_TO_WATERING,  ///< Currently walking to a flowerbed in need of watering.
		LOOKING_FOR_PATH,     ///< Trying to get back onto a path.
	};

	Handyman();

	void Load(Loader &ldr);
	void Save(Saver &svr);

	void DecideMoveDirection() override;
	AnimateResult ActionAnimationCallback() override;
	bool IsLeavingPath() const override;
	AnimateResult InteractWithPathObject(PathObjectInstance *obj) override;
	bool WalksOnQueuePaths() const override
	{
		return true;
	}

	HandymanActivity activity;  ///< What the handyman is doing right now.
};

#endif

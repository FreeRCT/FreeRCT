/* $Id$ */

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
	OAR_OK,         ///< All OK, keep running.
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

	const ImageData *GetSprite(const SpriteStorage *sprites, ViewOrientation orient, const Recolouring **recolour) const override;

	virtual AnimateResult OnAnimate(int delay) = 0;
	virtual bool DailyUpdate() = 0;

	virtual void Activate(const Point16 &start, PersonType person_type);
	virtual void DeActivate(AnimateResult ar);

	/**
	 * Test whether this person is active in the game or not.
	 * @return Whether the person is active in the game.
	 */
	bool IsActive() const
	{
		return this->type != PERSON_INVALID;
	}

	void SetName(const char *name);
	const char *GetName() const;

	uint16 id;       ///< Unique id of the person.
	PersonType type; ///< Type of person.
	int16 offset;    ///< Offset with respect to centre of paths walked on (0..100).

	const WalkInformation *walk;  ///< Walk animation sequence being performed.
	const AnimationFrame *frames; ///< Animation frames of the current animation.
	uint16 frame_count;           ///< Number of frames in #frames.
	uint16 frame_index;           ///< Currently displayed frame of #frames.
	int16 frame_time;             ///< Remaining display time of this frame.
	Recolouring recolour;         ///< Person recolouring.

protected:
	Random rnd; ///< Random number generator for deciding how the person reacts.
	char *name; ///< Name of the person. \c nullptr means it has a default name (like "Guest XYZ").

	TileEdge GetCurrentEdge() const;

	virtual void DecideMoveDirection() = 0;
	void StartAnimation(const WalkInformation *walk);

	virtual RideVisitDesire WantToVisit(const RideInstance *ri);
};

/** Activities of the guest. */
enum GuestActivity {
	GA_ENTER_PARK,   ///< Find the entrance to the park.
	GA_WANDER,       ///< Wander around in the park.
	GA_GO_HOME,      ///< Find a way to home.
};

/** %Guests walking around in the world. */
class Guest : public Person {
public:
	Guest();
	~Guest();

	void Activate(const Point16 &start, PersonType person_type) override;
	void DeActivate(AnimateResult ar) override;

	AnimateResult OnAnimate(int delay) override;
	bool DailyUpdate() override;

	void ChangeHappiness(int16 amount);
	void VisitShop(RideInstance *ri);
	void NotifyRideDeletion(const RideInstance *ri);

	GuestActivity activity; ///< Activity being done by the guest currently.
	int16 happiness;        ///< Happiness of the guest (values are 0-100). Use #ChangeHappiness to change the guest happiness.
	uint16 total_happiness; ///< Sum of all good experiences (for evaluating the day after getting home, values are 0-1000).
	Money cash;             ///< Amount of money carried by the guest (should be non-negative).
	const RideInstance *wants_visit; ///< Ride that the guest wants to visit, \c nullptr there is no favorite ride.
	bool queue_mode;        ///< Whether we are queueing.

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

protected:
	void DecideMoveDirection() override;
	RideVisitDesire ComputeExitDesire(TileEdge current_edge, int x, int y, int z, TileEdge exit_edge, bool *seen_wanted_ride);
	uint8 GetExitDirections(const Voxel *v, TileEdge start_edge, bool *seen_wanted_ride, bool *queue_mode);
	RideVisitDesire WantToVisit(const RideInstance *ri) override;

	RideVisitDesire NeedForItem(enum ItemType it, bool use_random);
	void AddItem(ItemType it);
};

#endif

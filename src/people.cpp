/* $Id$ */

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
#include "people.h"
#include "viewport.h"
#include "gamelevel.h"
#include "ride_type.h"

Guests _guests; ///< Guests in the world/park.

Person::Person() : rnd()
{
	this->type = PERSON_INVALID;
	this->name = NULL;
	this->next = NULL;
	this->prev = NULL;

	this->offset = this->rnd.Uniform(100);
}

Person::~Person()
{
	free(this->name);
}

/**
 * Set the name of a guest.
 * @param name New name of the guest.
 * @note Currently unused.
 */
void Person::SetName(const char *name)
{
	assert(PersonIsAGuest(this->type));

	int length = strlen(name);
	this->name = (char *)malloc(length + 1);
	memcpy(this->name, name, length);
	this->name[length] = '\0';
}

/**
 * Query the name of the person.
 * The name is returned in memory owned by the person. Do not free this data. It may change on each call.
 * @return Static buffer containing the name of the person.
 * @note Currently not used.
 */
const char *Person::GetName() const
{
	static char buffer[16];

	assert(PersonIsAGuest(this->type));
	if (this->name != NULL) return this->name;
	sprintf(buffer, "guest %u", this->id);
	return buffer;
}

/**
 * Compute the height of the path in the given voxel, at the given x/y position.
 * @param x_vox X coordinate of the voxel.
 * @param y_vox Y coordinate of the voxel.
 * @param z_vox Z coordinate of the voxel.
 * @param x_pos X position in the voxel.
 * @param y_pos Y position in the voxel.
 * @return Z height of the path in the voxel at the give position.
 * @todo Make it work at sloped surface too, in case the person ends up at path-less land.
 */
static int16 GetZHeight(int16 x_vox, int16 y_vox, int16 z_vox, int16 x_pos, int16 y_pos)
{
	const Voxel *v = _world.GetVoxel(x_vox, y_vox, z_vox);
	uint8 slope = v->GetPathRideFlags();
	if (slope < PATH_FLAT_COUNT) return 0;
	switch (slope) {
		case PATH_RAMP_NE: return x_pos;
		case PATH_RAMP_NW: return y_pos;
		case PATH_RAMP_SE: return 255 - y_pos;
		case PATH_RAMP_SW: return 255 - x_pos;
		default: NOT_REACHED();
	}
}

/**
 * Mark this person as 'in use'.
 * @param start Start x/y voxel stack for entering the world.
 * @param person_type Type of person getting activated.
 */
/* virtual */ void Person::Activate(const Point16 &start, PersonType person_type)
{
	assert(this->type == PERSON_INVALID);
	assert(person_type != PERSON_INVALID);

	this->type = person_type;
	this->name = NULL;

	/* Setup the person sprite recolouring table. */
	const PersonTypeData &person_type_data = GetPersonTypeData(this->type);
	this->recolour = person_type_data.graphics.MakeRecolouring(&this->rnd);

	/* Set up initial position. */
	this->x_vox = start.x;
	this->y_vox = start.y;
	this->z_vox = _world.GetGroundHeight(start.x, start.y);
	_world.GetPersonList(this->x_vox, this->y_vox, this->z_vox).AddFirst(this);

	if (start.x == 0) {
		this->x_pos = 0;
		this->y_pos = 128 - this->offset;
	} else if (start.x == _world.GetXSize() - 1) {
		this->x_pos = 255;
		this->y_pos = 128 + this->offset;
	} else if (start.y == 0) {
		this->x_pos = 128 + this->offset;
		this->y_pos = 0;
	} else {
		this->x_pos = 128 - this->offset;
		this->y_pos = 255;
	}
	this->z_pos = GetZHeight(this->x_vox, this->y_vox, this->z_vox, this->x_pos, this->y_pos);

	this->DecideMoveDirection();
}

/** Walk from NE edge back to NE edge. */
static const WalkInformation _walk_ne_ne[] = {
	{ANIM_WALK_SW, WLM_HIGH_X}, {ANIM_WALK_SE, WLM_HIGH_Y}, {ANIM_WALK_NE, WLM_NE_EDGE}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from NE edge to SE edge. */
static const WalkInformation _walk_ne_se[] = {
	{ANIM_WALK_SW, WLM_HIGH_X}, {ANIM_WALK_SE, WLM_SE_EDGE}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from NE edge to SW edge. */
static const WalkInformation _walk_ne_sw[] = {
	{ANIM_WALK_SW, WLM_SW_EDGE}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from NE edge to NW edge. */
static const WalkInformation _walk_ne_nw[] = {
	{ANIM_WALK_SW, WLM_LOW_X},  {ANIM_WALK_NW, WLM_NW_EDGE}, {ANIM_INVALID, WLM_INVALID}
};

/** Walk from SE edge to NE edge. */
static const WalkInformation _walk_se_ne[] = {
	{ANIM_WALK_NW, WLM_HIGH_Y}, {ANIM_WALK_NE, WLM_NE_EDGE}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from SE edge back to SE edge. */
static const WalkInformation _walk_se_se[] = {
	{ANIM_WALK_NW, WLM_LOW_Y},  {ANIM_WALK_SW, WLM_HIGH_X}, {ANIM_WALK_SE, WLM_SE_EDGE}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from SE edge to SW edge. */
static const WalkInformation _walk_se_sw[] = {
	{ANIM_WALK_NW, WLM_LOW_Y},  {ANIM_WALK_SW, WLM_SW_EDGE }, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from SE edge to NW edge. */
static const WalkInformation _walk_se_nw[] = {
	{ANIM_WALK_NW, WLM_NW_EDGE}, {ANIM_INVALID, WLM_INVALID}
};

/** Walk from SW edge to NE edge. */
static const WalkInformation _walk_sw_ne[] = {
	{ANIM_WALK_NE, WLM_NE_EDGE}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from SW edge to SE edge. */
static const WalkInformation _walk_sw_se[] = {
	{ANIM_WALK_NE, WLM_HIGH_X}, {ANIM_WALK_SE, WLM_SE_EDGE}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from SW edge back to SW edge. */
static const WalkInformation _walk_sw_sw[] = {
	{ANIM_WALK_NE, WLM_LOW_X},  {ANIM_WALK_NW, WLM_LOW_Y}, {ANIM_WALK_SW, WLM_SW_EDGE}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from SW edge to NW edge. */
static const WalkInformation _walk_sw_nw[] = {
{ANIM_WALK_NE, WLM_LOW_X},  {ANIM_WALK_NW, WLM_NW_EDGE}, {ANIM_INVALID, WLM_INVALID}
};

/** Walk from NW edge to NE edge. */
static const WalkInformation _walk_nw_ne[] = {
	{ANIM_WALK_SE, WLM_HIGH_Y}, {ANIM_WALK_NE, WLM_NE_EDGE}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from NW edge to SE edge. */
static const WalkInformation _walk_nw_se[] = {
	{ANIM_WALK_SE, WLM_SE_EDGE}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from NW edge to SW edge. */
static const WalkInformation _walk_nw_sw[] = {
	{ANIM_WALK_SE, WLM_LOW_Y},  {ANIM_WALK_SW, WLM_SW_EDGE}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from NW edge back to NW edge. */
static const WalkInformation _walk_nw_nw[] = {
	{ANIM_WALK_SE, WLM_HIGH_Y}, {ANIM_WALK_NE, WLM_LOW_X}, {ANIM_WALK_NW, WLM_NW_EDGE}, {ANIM_INVALID, WLM_INVALID}
};

/** Movement of one edge to another edge of a path tile. */
static const WalkInformation *_walk_path_tile[4][4] = {
	{_walk_ne_ne, _walk_ne_se, _walk_ne_sw, _walk_ne_nw},
	{_walk_se_ne, _walk_se_se, _walk_se_sw, _walk_se_nw},
	{_walk_sw_ne, _walk_sw_se, _walk_sw_sw, _walk_sw_nw},
	{_walk_nw_ne, _walk_nw_se, _walk_nw_sw, _walk_nw_nw},
};


/**
 * Decide at which edge the person is.
 * @return Nearest edge of the person.
 */
TileEdge Person::GetCurrentEdge() const
{
	assert(this->x_pos >= 0 && this->x_pos <= 255);
	assert(this->y_pos >= 0 && this->y_pos <= 255);

	int x = (this->x_pos < 128) ? this->x_pos : 255 - this->x_pos;
	int y = (this->y_pos < 128) ? this->y_pos : 255 - this->y_pos;

	if (x < y) {
		return (this->x_pos < 128) ? EDGE_NE : EDGE_SW;
	} else {
		return (this->y_pos < 128) ? EDGE_NW : EDGE_SE;
	}
}

/**
 * Decide where to go from the current position.
 */
void Person::DecideMoveDirection()
{
	/* Which way can the guest leave? */
	uint8 bot_exits = (1 << PATHBIT_NE) | (1 << PATHBIT_SW) | (1 << PATHBIT_SE) | (1 << PATHBIT_NW); // All exit directions by default.
	uint8 top_exits = 0;
	const Voxel *v = _world.GetVoxel(this->x_vox, this->y_vox, this->z_vox);
	if (v->GetType() == VT_SURFACE && HasValidPath(v)) {
		uint8 slope = v->GetPathRideFlags();
		if (slope < PATH_FLAT_COUNT) { // At a flat path tile.
			bot_exits &= _path_expand[slope]; // Masks to exits only, as that's what 'bot_exits' contains here.
		} else { // At a path ramp.
			switch (slope) {
				case PATH_RAMP_NE: bot_exits = 1 << PATHBIT_NE; top_exits = 1 << PATHBIT_SW; break;
				case PATH_RAMP_NW: bot_exits = 1 << PATHBIT_NW; top_exits = 1 << PATHBIT_SE; break;
				case PATH_RAMP_SE: bot_exits = 1 << PATHBIT_SE; top_exits = 1 << PATHBIT_NW; break;
				case PATH_RAMP_SW: bot_exits = 1 << PATHBIT_SW; top_exits = 1 << PATHBIT_NE; break;
				default: NOT_REACHED();
			}
		}
		/* Being at a path tile, make extra sure we don't leave the path. */
		for (TileEdge exit_edge = EDGE_BEGIN; exit_edge != EDGE_COUNT; exit_edge++) {
			if (GB(bot_exits, exit_edge + PATHBIT_NE, 1) != 0) {
				if (!PathExistsAtBottomEdge(this->x_vox, this->y_vox, this->z_vox, exit_edge)) {
					SB(bot_exits, exit_edge + PATHBIT_NE, 1, 0); // Clear exit if it leads to nowhere.
				}
			} else if (GB(top_exits, exit_edge + PATHBIT_NE, 1) != 0) {
				if (!PathExistsAtBottomEdge(this->x_vox, this->y_vox, this->z_vox + 1, exit_edge)) {
					SB(top_exits, exit_edge + PATHBIT_NE, 1, 0); // Clear exit if it leads to nowhere.
				}
			}
		}
	}

	const WalkInformation *walks[4]; // Walks that can be done at this tile.
	int walk_count = 0;

	TileEdge start_edge = this->GetCurrentEdge();
	for (TileEdge exit_edge = EDGE_BEGIN; exit_edge != EDGE_COUNT; exit_edge++) {
		if (start_edge == exit_edge) continue;
		if (GB(bot_exits, exit_edge + PATHBIT_NE, 1) != 0 || GB(top_exits, exit_edge + PATHBIT_NE, 1) != 0) {
			walks[walk_count++] = _walk_path_tile[start_edge][exit_edge];
		}
	}
	if (walk_count == 0) walks[walk_count++] = _walk_path_tile[start_edge][start_edge];

	const WalkInformation *new_walk;
	if (walk_count == 1) {
		new_walk = walks[0];
	} else {
		new_walk = walks[this->rnd.Uniform(walk_count - 1)];
	}

	this->StartAnimation(new_walk);
}

/**
 * Perform the animation sequence as provided.
 * @param walk Walk information describing the animations to perform.
 */
void Person::StartAnimation(const WalkInformation *walk)
{
	const Animation *anim = _sprite_manager.GetAnimation(walk->anim_type, this->type);
	assert(anim != NULL && anim->frame_count != 0);

	this->walk = walk;
	this->frames = anim->frames;
	this->frame_count = anim->frame_count;
	this->frame_index = 0;
	this->frame_time = this->frames[this->frame_index].duration;
	this->MarkDirty();
}

/**
 * Mark this person as 'not in use'. (Called by #Guests.)
 * @param ar How to de-activate the person.
 */
/* virtual */ void Person::DeActivate(AnimateResult ar)
{
	if (this->type == PERSON_INVALID) return;

	if (ar == OAR_REMOVE && _world.VoxelExists(this->x_vox, this->y_vox, this->z_vox)) {
		/* If not wandered off-world, remove the person from the voxel person list. */
		_world.GetPersonList(this->x_vox, this->y_vox, this->z_vox).Remove(this);
	}
	this->type = PERSON_INVALID;
	free(this->name);
	this->name = NULL;
}

/**
 * Update the animation of the person.
 * @param delay Amount of milli seconds since the last update.
 * @return If \c false, de-activate the person.
 */
AnimateResult Person::OnAnimate(int delay)
{
	this->frame_time -= delay;
	if (this->frame_time > 0) return OAR_OK;

	this->MarkDirty(); // Marks the entire voxel dirty, which should be big enough even after moving.

	if (this->frames == NULL || this->frame_count == 0) return OAR_REMOVE;

	int16 x_limit = -1;
	switch (GB(this->walk->limit_type, WLM_X_START, WLM_LIMIT_LENGTH)) {
		case WLM_MINIMAL: x_limit =   0;                break;
		case WLM_LOW:     x_limit = 128 - this->offset; break;
		case WLM_CENTER:  x_limit = 128;                break;
		case WLM_HIGH:    x_limit = 128 + this->offset; break;
		case WLM_MAXIMAL: x_limit = 255;                break;
	}

	int16 y_limit = -1;
	switch (GB(this->walk->limit_type, WLM_Y_START, WLM_LIMIT_LENGTH)) {
		case WLM_MINIMAL: y_limit =   0;                break;
		case WLM_LOW:     y_limit = 128 - this->offset; break;
		case WLM_CENTER:  y_limit = 128;                break;
		case WLM_HIGH:    y_limit = 128 + this->offset; break;
		case WLM_MAXIMAL: y_limit = 255;                break;
	}

	uint16 index = this->frame_index;
	const AnimationFrame *frame = &this->frames[index];
	this->x_pos += frame->dx;
	this->y_pos += frame->dy;

	bool reached = false; // Set to true when we are beyond the limit!
	if ((this->walk->limit_type & (1 << WLM_END_LIMIT)) == WLM_X_COND) {
		if (frame->dx > 0) reached |= this->x_pos > x_limit;
		if (frame->dx < 0) reached |= this->x_pos < x_limit;
	} else {
		if (frame->dy > 0) reached |= this->y_pos > y_limit;
		if (frame->dy < 0) reached |= this->y_pos < y_limit;
	}

	if (!reached) {
		/* Not reached the end, do the next frame. */
		index++;
		if (this->frame_count <= index) index = 0;
		this->frame_index = index;
		this->frame_time = this->frames[index].duration;

		this->z_pos = GetZHeight(this->x_vox, this->y_vox, this->z_vox, this->x_pos, this->y_pos);
		return OAR_OK;
	}

	/* Reached the goal, start the next walk. */
	if (this->walk[1].anim_type == ANIM_INVALID) {
		_world.GetPersonList(this->x_vox, this->y_vox, this->z_vox).Remove(this);
		/* Not only the end of this walk, but the end of the entire walk at the tile. */
		if (this->x_pos < 0) {
			this->x_vox--;
			this->x_pos += 256;
		} else if (this->x_pos > 255) {
			this->x_vox++;
			this->x_pos -= 256;
		}
		if (this->y_pos < 0) {
			this->y_vox--;
			this->y_pos += 256;
		} else if (this->y_pos > 255) {
			this->y_vox++;
			this->y_pos -= 256;
		}
		assert(this->x_pos >= 0 && this->x_pos < 256);
		assert(this->y_pos >= 0 && this->y_pos < 256);

		/* If the guest ended up off-world, quit. */
		if (this->x_vox < 0 || this->x_vox >= _world.GetXSize() * 256 ||
				this->y_vox < 0 || this->y_vox >= _world.GetYSize() * 256) {
			return OAR_DEACTIVATE;
		}

		/* Handle z position. */
		if (this->z_pos > 128) {
			this->z_vox++;
			this->z_pos = 0;
		}
		/* At bottom of the voxel, the path either stays on the same level or goes down. */
		Voxel *v = _world.GetCreateVoxel(this->x_vox, this->y_vox, this->z_vox, false);
		if ((v == NULL || v->GetType() != VT_SURFACE) && this->z_vox > 0) {
			this->z_vox--;
			this->z_pos = 255;
			v = _world.GetCreateVoxel(this->x_vox, this->y_vox, this->z_vox, false);
		}
		/* We seem to have wandered off-path in some way, abort. */
		if (v == NULL || v->GetType() != VT_SURFACE || !HasValidPath(v)) return OAR_DEACTIVATE;

		v->persons.AddFirst(this);
		this->DecideMoveDirection();
		return OAR_OK;
	}
	this->StartAnimation(this->walk + 1);
	return OAR_OK;
}

/** Mark the screen where this person is, as dirty so it is repainted the next time. */
void Person::MarkDirty()
{
	Viewport *vp = GetViewport();
	if (vp != NULL) vp->MarkVoxelDirty(this->x_vox, this->y_vox, this->z_vox);
}

/**
 * How much does the person desire to visit the given ride?
 * @param ri Ride that can be visited.
 * @return Desire of the person to visit the ride.
 */
/* virtual */ RideVisitDesire Person::WantToVisit(const RideInstance *ri) const
{
	return RVD_NO_VISIT;
}

/**
 * @fn bool Person::DailyUpdate()
 * Daily ponderings of a person.
 * @return If \c false, de-activate the person.
 */

Guest::Guest() : Person()
{
}

Guest::~Guest()
{
}


/* virtual */ void Guest::Activate(const Point16 &start, PersonType person_type)
{
	this->Person::Activate(start, person_type);

	this->happiness = 50 + this->rnd.Uniform(50);
	this->cash = 3000 + this->rnd.Uniform(4095);
}

/**
 * Daily ponderings of a guest.
 * @return If \c false, de-activate the guest.
 * @todo Make de-activation a bit more random.
 */
/* virtual */ bool Guest::DailyUpdate()
{
	if (!PersonIsAGuest(this->type)) return false;

	this->happiness = max(0, this->happiness - 2);
	return this->happiness > 10; // De-activate if at or below 10.
}

/* virtual */ RideVisitDesire Guest::WantToVisit(const RideInstance *ri) const
{
	return RVD_MAY_VISIT;
}

/** Default constructor of the person list. */
PersonList::PersonList()
{
	this->first = NULL;
	this->last = NULL;
}

/** Destructor of the person list. */
PersonList::~PersonList()
{
	// Silently let go of the persons in the list.
}

/**
 * Are there any persons in the list?
 * @return \c true iff at least one person is in the list.
 */
bool PersonList::IsEmpty() const
{
	return this->first == NULL;
}

/**
 * Add person to the head of the list.
 * @param p %Person to add.
 */
void PersonList::AddFirst(Person *p)
{
	p->prev = NULL;
	p->next = this->first;
	if (this->first == NULL) {
		this->last = p;
		this->first = p;
	} else {
		this->first->prev = p;
		this->first = p;
	}
}

/**
 * Remove a person from the list.
 * @param p %Person to remove.
 */
void PersonList::Remove(Person *p)
{
	/* Paranoia check, the person should be in this list. */
	assert(p != NULL);
	Person *here = this->first;
	while (here != NULL && here != p) here = here->next;
	assert(here == p);

	if (p->prev == NULL) {
		assert(this->first == p);
		this->first = p->next;
	} else {
		p->prev->next = p->next;
	}

	if (p->next == NULL) {
		assert(this->last == p);
		this->last = p->prev;
	} else {
		p->next->prev = p->prev;
	}
}

/**
 * Get the person from the head of the list.
 * @return %Person at the head of the list.
 */
Person *PersonList::RemoveHead()
{
	Person *p = this->first;
	assert(p != NULL);
	this->Remove(p);
	return p;
}

/**
 * Copy a person list. The source is not modified.
 * @param dest Destination of the copying process.
 * @param src Source of the copying process.
 */
void CopyPersonList(PersonList &dest, const PersonList &src)
{
	dest.first = src.first;
	dest.last  = src.last;
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
 * Add all persons to the provided list.
 * @param pl %Person list to add the persons to.
 */
void GuestBlock::AddAll(PersonList *pl)
{
	/* Add in reverse order so the first retrieval is the first person in this block. */
	uint i = lengthof(this->guests) - 1;
	for (;;) {
		pl->AddFirst(this->Get(i));
		if (i == 0) break;
		i--;
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
	int16 z = _world.GetGroundHeight(x, y);
	const Voxel *vs = _world.GetVoxel(x, y, z);
	return HasValidPath(vs) && vs->GetPathRideFlags() < PATH_FLAT_COUNT;
}

/**
 * Try to find a voxel at the edge of the world that can be used as entry point for guests.
 * @return The x/y coordinate at the edge of the world of a usable voxelstack, else an off-world coordinate.
 */
static Point16 FindEdgeRoad()
{
	Point16 pt;

	int16 highest_x = _world.GetXSize() - 1;
	int16 highest_y = _world.GetYSize() - 1;
	for (int x = 1; x < highest_x; x++) {
		if (IsGoodEdgeRoad(x, 0)) {
			pt.x = x;
			pt.y = 0;
			return pt;
		}
		if (IsGoodEdgeRoad(x, highest_y)) {
			pt.x = x;
			pt.y = highest_y;
			return pt;
		}
	}
	for (int y = 1; y < highest_y; y++) {
		if (IsGoodEdgeRoad(0, y)) {
			pt.x = 0;
			pt.y = y;
			return pt;
		}
		if (IsGoodEdgeRoad(highest_x, y)) {
			pt.x = highest_x;
			pt.y = y;
			return pt;
		}
	}

	pt.x = -1;
	pt.y = -1;
	return pt;
}


Guests::Guests() : block(0), rnd()
{
	this->block.AddAll(&this->free);

	this->start_voxel.x = -1;
	this->start_voxel.y = -1;
	this->daily_frac = 0;
	this->next_daily_index = 0;
	this->valid_ptypes = 0;
}

Guests::~Guests()
{
}

/** After loading the RCD files, check what resources actually exist. */
void Guests::Initialize()
{
	this->valid_ptypes = 0;
	for (PersonType pertype = PERSON_MIN_GUEST; pertype <= PERSON_MAX_GUEST; pertype++) {
		bool usable = true;
		for (AnimationType antype = ANIM_BEGIN; antype <= ANIM_LAST; antype++) {
			if (_sprite_manager.GetAnimation(antype, pertype) == NULL) {
				usable = false;
				break;
			}
		}
		if (usable) this->valid_ptypes |= 1u << (pertype - PERSON_MIN_GUEST);
	}
}

/**
 * Can guests of the given person type be used for the level?
 * @param ptype %Person type to examine.
 * @return Whether the given person type can be used for playing a level.
 */
bool Guests::CanUsePersonType(PersonType ptype)
{
	if (ptype < PERSON_MIN_GUEST || ptype > PERSON_MAX_GUEST) return false;
	return (this->valid_ptypes & (1 << (ptype - PERSON_MIN_GUEST))) != 0;
}

/**
 * Count the number of active guests.
 * @return The number of active guests.
 */
uint Guests::CountActiveGuests()
{
	uint count = GUEST_BLOCK_SIZE;
	const Person *pers = this->free.first;
	while (pers != NULL) {
		assert(count > 0);
		count--;
		pers = pers->next;
	}
	return count;
}

/**
 * Some time has passed, update the animation.
 * @param delay Number of milli seconds time that have past since the last animation update.
 */
void Guests::OnAnimate(int delay)
{
	for (uint i = 0; i < GUEST_BLOCK_SIZE; i++) {
		Person *p = this->block.Get(i);
		if (p->type == PERSON_INVALID) continue;

		AnimateResult ar = p->OnAnimate(delay);
		if (ar != OAR_OK) {
			p->DeActivate(ar);
			this->free.AddFirst(p);
		}
	}
}

/** A new frame arrived, perform the daily call for some of the guests. */
void Guests::DoTick()
{
	this->daily_frac++;
	int end_index = min(this->daily_frac * GUEST_BLOCK_SIZE / TICK_COUNT_PER_DAY, GUEST_BLOCK_SIZE);
	while (this->next_daily_index < end_index) {
		Person *p = this->block.Get(this->next_daily_index);
		if (p->type != PERSON_INVALID && !p->DailyUpdate()) {
			p->DeActivate(OAR_REMOVE);
			this->free.AddFirst(p);
		}
		this->next_daily_index++;
	}
	if (this->next_daily_index >= (int)GUEST_BLOCK_SIZE) {
		this->daily_frac = 0;
		this->next_daily_index = 0;
	}
}

/**
 * A new day arrived, handle daily chores of the park.
 * @todo Add popularity rating concept.
 * @todo Person type should be configurable too.
 */
void Guests::OnNewDay()
{
	PersonType ptype = PERSON_PILLAR;
	if (!this->CanUsePersonType(ptype)) return;
	if (this->CountActiveGuests() >= _scenario.max_guests) return;
	if (!this->rnd.Success1024(_scenario.GetSpawnProbability(512))) return;

	if (!IsGoodEdgeRoad(this->start_voxel.x, this->start_voxel.y)) {
		/* New guest, but no road. */
		this->start_voxel = FindEdgeRoad();
		if (!IsGoodEdgeRoad(this->start_voxel.x, this->start_voxel.y)) return;
	}

	if (this->free.IsEmpty()) return; // No more quests available.
	/* New guest! */
	Person *p = this->free.RemoveHead();
	if (p != NULL) p->Activate(this->start_voxel, ptype);
}


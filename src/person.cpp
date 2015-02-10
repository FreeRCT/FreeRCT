/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file person.cpp Person-related functions. */

#include "stdafx.h"
#include "enum_type.h"
#include "geometry.h"
#include "math_func.h"
#include "person_type.h"
#include "sprite_store.h"
#include "ride_type.h"
#include "person.h"
#include "people.h"
#include "fileio.h"
#include "map.h"
#include "path_finding.h"
#include "ride_type.h"
#include "viewport.h"
#include "weather.h"

static PersonTypeData _person_type_datas[PERSON_TYPE_COUNT]; ///< Data about each type of person.

/**
 * Construct a recolour mapping of this person type.
 * @return The constructed recolouring.
 */
Recolouring PersonTypeGraphics::MakeRecolouring() const
{
	Recolouring recolour(this->recolours);
	recolour.AssignRandomColours();
	return recolour;
}

/**
 * Get the data about a person type with the intention to change it.
 * @param pt Type of person queried.
 * @return Reference to the data of the queried person type.
 * @note Use #GetPersonTypeData if the data is only read.
 */
PersonTypeData &ModifyPersonTypeData(PersonType pt)
{
	assert(pt < lengthof(_person_type_datas));
	return _person_type_datas[pt];
}

/**
 * Load graphics settings of person types from an RCD file.
 * @param rcd_file RCD file to read, pointing at the start of the PRSG block data (behind the header information).
 * @return Loading was a success.
 */
bool LoadPRSG(RcdFileReader *rcd_file)
{
	uint32 length = rcd_file->size;
	if (rcd_file->version < 1 || rcd_file->version > 2 || length < 1) return false;
	uint8 count = rcd_file->GetUInt8();
	length--;

	if (length != 13 * count) return false;
	while (count > 0) {
		uint8 ps = rcd_file->GetUInt8();
		uint32 rc1 = rcd_file->GetUInt32();
		uint32 rc2 = rcd_file->GetUInt32();
		uint32 rc3 = rcd_file->GetUInt32();

		PersonType pt;
		switch (ps) {
			case  8:
			case 16: pt = PERSON_GUEST; break;
			case 17: pt = PERSON_HANDYMAN; break;
			case 18: pt = PERSON_MECHANIC; break;
			case 19: pt = PERSON_GUARD; break;
			case 20: pt = PERSON_ENTERTAINER; break;
			default: pt = PERSON_INVALID; break;
		}

		if (pt != PERSON_INVALID) {
			PersonTypeData &ptd = ModifyPersonTypeData(pt);
			ptd.graphics.recolours.Reset();
			ptd.graphics.recolours.Set(0, RecolourEntry(rc1));
			ptd.graphics.recolours.Set(1, RecolourEntry(rc2));
			ptd.graphics.recolours.Set(2, RecolourEntry(rc3));
		}
		count--;
	}
	return true;
}

Person::Person() : VoxelObject(), rnd()
{
	this->type = PERSON_INVALID;
	this->name = nullptr;

	this->offset = this->rnd.Uniform(100);
}

Person::~Person()
{
	delete[] this->name;
}

const ImageData *Person::GetSprite(const SpriteStorage *sprites, ViewOrientation orient, const Recolouring **recolour) const
{
	*recolour = &this->recolour;
	AnimationType anim_type = this->walk->anim_type;
	return sprites->GetAnimationSprite(anim_type, this->frame_index, this->type, orient);
}

/**
 * Set the name of a guest.
 * @param name New name of the guest.
 * @note Currently unused.
 */
void Person::SetName(const char *name)
{
	assert(this->IsGuest());

	int length = strlen(name);
	this->name = new char[length];
	std::copy_n(name, length, this->name);
}

/**
 * Query the name of the person.
 * The name is returned in memory owned by the person. Do not free this data. It may change on each call.
 * @return Static buffer containing the name of the person.
 */
const char *Person::GetName() const
{
	static char buffer[16];

	assert(this->IsGuest());
	if (this->name != nullptr) return this->name;
	sprintf(buffer, "Guest %u", this->id);
	return buffer;
}

/**
 * Compute the height of the path in the given voxel, at the given x/y position.
 * @param vox Coordinates of the voxel.
 * @param x_pos X position in the voxel.
 * @param y_pos Y position in the voxel.
 * @return Z height of the path in the voxel at the give position.
 * @todo Make it work at sloped surface too, in case the person ends up at path-less land.
 */
static int16 GetZHeight(const XYZPoint16 &vox, int16 x_pos, int16 y_pos)
{
	const Voxel *v = _world.GetVoxel(vox);
	if (HasValidPath(v)) {
		uint8 slope = GetImplodedPathSlope(v);
		if (slope < PATH_FLAT_COUNT) return 0;
		switch (slope) {
			case PATH_RAMP_NE: return x_pos;
			case PATH_RAMP_NW: return y_pos;
			case PATH_RAMP_SE: return 255 - y_pos;
			case PATH_RAMP_SW: return 255 - x_pos;
			default: NOT_REACHED();
		}
	}
	NOT_REACHED(); /// \todo No path here!
}

/**
 * Mark this person as 'in use'.
 * @param start Start x/y voxel stack for entering the world.
 * @param person_type Type of person getting activated.
 */
void Person::Activate(const Point16 &start, PersonType person_type)
{
	assert(!this->IsActive());
	assert(person_type != PERSON_INVALID);

	this->type = person_type;
	this->name = nullptr;

	/* Set up the person sprite recolouring table. */
	const PersonTypeData &person_type_data = GetPersonTypeData(this->type);
	this->recolour = person_type_data.graphics.MakeRecolouring();

	/* Set up initial position. */
	this->vox_pos.x = start.x;
	this->vox_pos.y = start.y;
	this->vox_pos.z = _world.GetGroundHeight(start.x, start.y);
	this->AddSelf(_world.GetCreateVoxel(this->vox_pos, false));

	if (start.x == 0) {
		this->pix_pos.x = 0;
		this->pix_pos.y = 128 - this->offset;
	} else if (start.x == _world.GetXSize() - 1) {
		this->pix_pos.x = 255;
		this->pix_pos.y = 128 + this->offset;
	} else if (start.y == 0) {
		this->pix_pos.x = 128 + this->offset;
		this->pix_pos.y = 0;
	} else {
		this->pix_pos.x = 128 - this->offset;
		this->pix_pos.y = 255;
	}
	this->pix_pos.z = GetZHeight(this->vox_pos, this->pix_pos.x, this->pix_pos.y);

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

/** Walk from NE edge back to centre NE edge. */
static const WalkInformation _center_ne_ne[] = {
	{ANIM_WALK_SW, WLM_HIGH_X}, {ANIM_WALK_SE, WLM_MID_Y}, {ANIM_WALK_NE, WLM_NE_CENTER}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from NE edge to centre SE edge. */
static const WalkInformation _center_ne_se[] = {
	{ANIM_WALK_SW, WLM_MID_X}, {ANIM_WALK_SE, WLM_SE_CENTER}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from NE edge to centre SW edge. */
static const WalkInformation _center_ne_sw[] = {
	{ANIM_WALK_SW, WLM_SW_CENTER}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from NE edge to centre NW edge. */
static const WalkInformation _center_ne_nw[] = {
	{ANIM_WALK_SW, WLM_MID_X},  {ANIM_WALK_NW, WLM_NW_CENTER}, {ANIM_INVALID, WLM_INVALID}
};

/** Walk from SE edge to centre NE edge. */
static const WalkInformation _center_se_ne[] = {
	{ANIM_WALK_NW, WLM_MID_Y}, {ANIM_WALK_NE, WLM_NE_CENTER}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from SE edge back to centre SE edge. */
static const WalkInformation _center_se_se[] = {
	{ANIM_WALK_NW, WLM_LOW_Y},  {ANIM_WALK_SW, WLM_MID_X}, {ANIM_WALK_SE, WLM_SE_CENTER}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from SE edge to centre SW edge. */
static const WalkInformation _center_se_sw[] = {
	{ANIM_WALK_NW, WLM_MID_Y},  {ANIM_WALK_SW, WLM_SW_CENTER }, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from SE edge to centre NW edge. */
static const WalkInformation _center_se_nw[] = {
	{ANIM_WALK_NW, WLM_NW_CENTER}, {ANIM_INVALID, WLM_INVALID}
};

/** Walk from SW edge to centre NE edge. */
static const WalkInformation _center_sw_ne[] = {
	{ANIM_WALK_NE, WLM_NE_CENTER}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from SW edge to centre SE edge. */
static const WalkInformation _center_sw_se[] = {
	{ANIM_WALK_NE, WLM_MID_X}, {ANIM_WALK_SE, WLM_SE_CENTER}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from SW edge back to centre SW edge. */
static const WalkInformation _center_sw_sw[] = {
	{ANIM_WALK_NE, WLM_LOW_X},  {ANIM_WALK_NW, WLM_MID_Y}, {ANIM_WALK_SW, WLM_SW_CENTER}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from SW edge to centre NW edge. */
static const WalkInformation _center_sw_nw[] = {
{ANIM_WALK_NE, WLM_MID_X},  {ANIM_WALK_NW, WLM_NW_CENTER}, {ANIM_INVALID, WLM_INVALID}
};

/** Walk from NW edge to centre NE edge. */
static const WalkInformation _center_nw_ne[] = {
	{ANIM_WALK_SE, WLM_MID_Y}, {ANIM_WALK_NE, WLM_NE_CENTER}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from NW edge to centre SE edge. */
static const WalkInformation _center_nw_se[] = {
	{ANIM_WALK_SE, WLM_SE_CENTER}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from NW edge to centre SW edge. */
static const WalkInformation _center_nw_sw[] = {
	{ANIM_WALK_SE, WLM_MID_Y},  {ANIM_WALK_SW, WLM_SW_CENTER}, {ANIM_INVALID, WLM_INVALID}
};
/** Walk from NW edge back to centre NW edge. */
static const WalkInformation _center_nw_nw[] = {
	{ANIM_WALK_SE, WLM_HIGH_Y}, {ANIM_WALK_NE, WLM_MID_X}, {ANIM_WALK_NW, WLM_NW_CENTER}, {ANIM_INVALID, WLM_INVALID}
};

/** Movement of one edge to another centre edge of a path tile. */
static const WalkInformation *_center_path_tile[4][4] = {
	{_center_ne_ne, _center_ne_se, _center_ne_sw, _center_ne_nw},
	{_center_se_ne, _center_se_se, _center_se_sw, _center_se_nw},
	{_center_sw_ne, _center_sw_se, _center_sw_sw, _center_sw_nw},
	{_center_nw_ne, _center_nw_se, _center_nw_sw, _center_nw_nw},
};

/**
 * Decide at which edge the person is.
 * @return Nearest edge of the person.
 */
TileEdge Person::GetCurrentEdge() const
{
	assert(this->pix_pos.x >= 0 && this->pix_pos.x <= 255);
	assert(this->pix_pos.y >= 0 && this->pix_pos.y <= 255);

	int x = (this->pix_pos.x < 128) ? this->pix_pos.x : 255 - this->pix_pos.x;
	int y = (this->pix_pos.y < 128) ? this->pix_pos.y : 255 - this->pix_pos.y;

	if (x < y) {
		return (this->pix_pos.x < 128) ? EDGE_NE : EDGE_SW;
	} else {
		return (this->pix_pos.y < 128) ? EDGE_NW : EDGE_SE;
	}
}

/**
 * Decide whether visiting the exit edge is useful.
 * @param current_edge Edge at the current position.
 * @param cur_pos Coordinate of the current voxel.
 * @param exit_edge Exit edge being examined.
 * @param seen_wanted_ride [inout] Whether the wanted ride is seen.
 * @return Desire of the guest to visit the indicated edge.
 */
RideVisitDesire Guest::ComputeExitDesire(TileEdge current_edge, XYZPoint16 cur_pos, TileEdge exit_edge, bool *seen_wanted_ride)
{
	if (current_edge == exit_edge) return RVD_NO_VISIT; // Skip incoming edge (may get added later if no other options exist).

	bool travel = TravelQueuePath(&cur_pos, &exit_edge);
	if (!travel) return RVD_NO_VISIT; // Path leads to nowhere.

	if (PathExistsAtBottomEdge(cur_pos, exit_edge)) return RVD_NO_RIDE; // Found a path.

	RideInstance *ri = RideExistsAtBottom(cur_pos, exit_edge);
	if (ri == nullptr || ri->state != RIS_OPEN) return RVD_NO_VISIT; // No ride, or a closed one.

	if (ri == this->ride) { // Guest decided before that this shop/ride should be visited.
		*seen_wanted_ride = true;
		return RVD_MUST_VISIT;
	}

	Point16 dxy = _tile_dxy[exit_edge];
	if (!ri->CanBeVisited(cur_pos + XYZPoint16(dxy.x, dxy.y, 0), exit_edge)) return RVD_NO_VISIT; // Ride cannot be entered here.

	RideVisitDesire rvd = this->WantToVisit(ri);
	if ((rvd == RVD_MAY_VISIT || rvd == RVD_MUST_VISIT) && this->ride == nullptr) {
		/* Decided to want to visit one ride, and no wanted ride yet. */
		// \todo Add a timeout so a guest gets bored waiting for the ride at some point.
		this->ride = ri;
		*seen_wanted_ride = true;
		return RVD_MUST_VISIT;
	}
	return rvd;
}

/**
 * Notify the guest of removal of a ride.
 * @param ri Ride being deleted.
 */
void Guest::NotifyRideDeletion(const RideInstance *ri)
{
	if (this->ride == ri) {
		switch (this->activity) {
			case GA_QUEUING:
				this->activity = GA_WANDER;
				this->ride = nullptr;
				break;

			case GA_ON_RIDE:
				NOT_REACHED(); // The ride should throw out its guests before deleting itself.

			default:
				this->ride = nullptr;
				break;
		}
	}
}

/**
 * Exit the ride, and continue walking in the park.
 * @param ri Ride to exit.
 * @param entry Entry direction of the ride.
 */
void Guest::ExitRide(RideInstance *ri, TileEdge entry)
{
	assert(this->activity == GA_ON_RIDE);
	assert(this->ride == ri);

	XYZPoint32 exit_pos = ri->GetExit(this->id, entry);
	this->vox_pos.x = exit_pos.x >> 8; this->pix_pos.x = exit_pos.x & 0xff;
	this->vox_pos.y = exit_pos.y >> 8; this->pix_pos.y = exit_pos.y & 0xff;
	this->vox_pos.z = exit_pos.z >> 8; this->pix_pos.z = exit_pos.z & 0xff;
	this->activity = GA_WANDER;
	this->AddSelf(_world.GetCreateVoxel(this->vox_pos, false));
	this->DecideMoveDirection();
}

/**
 * Which way can the guest leave?
 * @param v %Voxel to cross next for the guest.
 * @param start_edge %Edge where the person is currently (entry edge of the voxel).
 * @param seen_wanted_ride [out] Whether the wanted ride was seen.
 * @param queue_path [out] Whether the quest is walking on a queue path (that may or may not lead to a ride).
 * @return Possible exit directions in low nibble, exits with a shop in high nibble.
 * @pre \a v must have a path.
 */
uint8 Guest::GetExitDirections(const Voxel *v, TileEdge start_edge, bool *seen_wanted_ride, bool *queue_path)
{
	assert(HasValidPath(v));

	/* If walking on a queue path, enable queue mode. */
	// \todo Only walk in queue mode when going to a ride.
	*queue_path = _sprite_manager.GetPathStatus(GetPathType(v->GetInstanceData())) == PAS_QUEUE_PATH;
	*seen_wanted_ride = false;

	uint8 shops = 0;       // Number of exits with a shop with normal desire to go there.
	uint8 must_shops = 0;  // Shops with a high desire to visit.

	uint8 exits = GetPathExits(v);
	uint8 bot_exits = exits & 0x0F; // Exits at the bottom of the voxel.
	uint8 top_exits = (exits >> 4) & 0x0F; // Exits at the top of the voxel.

	/* Being at a path tile, make extra sure we don't leave the path. */
	for (TileEdge exit_edge = EDGE_BEGIN; exit_edge != EDGE_COUNT; exit_edge++) {
		int extra_z; // Decide z position of the exit.
		if (GB(bot_exits, exit_edge, 1) != 0) {
			extra_z = 0;
		} else if (GB(top_exits, exit_edge, 1) != 0) {
			extra_z = 1;
		} else {
			continue;
		}

		RideVisitDesire rvd = ComputeExitDesire(start_edge, this->vox_pos + XYZPoint16(0, 0, extra_z), exit_edge, seen_wanted_ride);
		switch (rvd) {
			case RVD_NO_RIDE:
				break; // A path is one of the options.

			case RVD_NO_VISIT: // No desire to visit this exit; clear it.
				SB(bot_exits, exit_edge, 1, 0);
				SB(top_exits, exit_edge, 1, 0);
				break;

			case RVD_MAY_VISIT: // It's one of the options (since the person is not coming from it).
				SB(shops, exit_edge, 1, 1);
				break;

			case RVD_MUST_VISIT: // It is very desirable to visit this shop (since the person is not coming from it).
				SB(must_shops, exit_edge, 1, 1);
				break;

			default: NOT_REACHED();
		}
	}
	bot_exits |= top_exits; // Difference between bottom-exit and top-exit is not relevant any more.
	if (must_shops != 0) {  // If there are shops that must be visited, ignore everything else.
		bot_exits &= must_shops;
		shops = must_shops;
	}
	return (shops << 4) | bot_exits;
}

/**
 * From a junction, find the direction that leads to an entrance of the park.
 * @param pos Current position.
 * @return Edge to go to to go to an entrance of the park, or #INVALID_EDGE if no path could be found.
 */
static TileEdge GetParkEntryDirection(const XYZPoint16 &pos)
{
	PathSearcher ps(pos); // Current position is the destination.

	/* Add path tiles with a connection to outside the park to the initial starting points. */
	for (int x = 0; x < _world.GetXSize() - 1; x++) {
		for (int y = 0; y < _world.GetYSize() - 1; y++) {
			const VoxelStack *vs = _world.GetStack(x, y);
			if (vs->owner == OWN_PARK) {
				if (_world.GetStack(x + 1, y)->owner != OWN_PARK || _world.GetStack(x, y + 1)->owner != OWN_PARK) {
					int offset = vs->GetGroundOffset();
					const Voxel *v = vs->voxels + offset;
					if (HasValidPath(v) && GetImplodedPathSlope(v) < PATH_FLAT_COUNT &&
							(GetPathExits(v) & ((1 << EDGE_SE) | (1 << EDGE_SW))) != 0) {
						ps.AddStart(XYZPoint16(x, y, vs->base + offset));
					}
				}
			} else {
				vs = _world.GetStack(x + 1, y);
				if (vs->owner == OWN_PARK) {
					int offset = vs->GetGroundOffset();
					const Voxel *v = vs->voxels + offset;
					if (HasValidPath(v) && GetImplodedPathSlope(v) < PATH_FLAT_COUNT &&
							(GetPathExits(v) & (1 << EDGE_NE)) != 0) {
						ps.AddStart(XYZPoint16(x + 1, y, vs->base + offset));
					}
				}

				vs = _world.GetStack(x, y + 1);
				if (vs->owner == OWN_PARK) {
					int offset = vs->GetGroundOffset();
					const Voxel *v = vs->voxels + offset;
					if (HasValidPath(v) && GetImplodedPathSlope(v) < PATH_FLAT_COUNT &&
							(GetPathExits(v) & (1 << EDGE_NW)) != 0) {
						ps.AddStart(XYZPoint16(x, y + 1, vs->base + offset));
					}
				}
			}
		}
	}
	if (!ps.Search()) return INVALID_EDGE; // Search failed.

	const WalkedPosition *dest = ps.dest_pos;
	const WalkedPosition *prev = dest->prev_pos;
	if (prev == nullptr) return INVALID_EDGE; // Already at tile.

	return GetAdjacentEdge(dest->cur_vox.x, dest->cur_vox.y, prev->cur_vox.x, prev->cur_vox.y);
}

/**
 * From a junction, find the direction that leads to the 'go home' tile.
 * @param pos Current position.
 * @return Edge to go to to go to the 'go home' tile, or #INVALID_EDGE if no path could be found.
 */
static TileEdge GetGoHomeDirection(const XYZPoint16 &pos)
{
	PathSearcher ps(pos); // Current position is the destination.

	int x = _guests.start_voxel.x;
	int y = _guests.start_voxel.y;
	ps.AddStart(XYZPoint16(x, y, _world.GetGroundHeight(x, y)));

	if (!ps.Search()) return INVALID_EDGE;

	const WalkedPosition *dest = ps.dest_pos;
	const WalkedPosition *prev = dest->prev_pos;
	if (prev == nullptr) return INVALID_EDGE; // Already at tile.

	return GetAdjacentEdge(dest->cur_vox.x, dest->cur_vox.y, prev->cur_vox.x, prev->cur_vox.y);
}

/**
 * Get the index of the exit edge to use.
 * @param desired_edge Edge to use for leaving.
 * @param exits Available exits.
 * @return Index of the desired edge in the available edges, or \c -1
 */
static int GetDesiredEdgeIndex(TileEdge desired_edge, uint8 exits)
{
	int i = 0;
	for (TileEdge exit_edge = EDGE_BEGIN; exit_edge != EDGE_COUNT; exit_edge++) {
		if (GB(exits, exit_edge, 1) != 0) {
			if (exit_edge == desired_edge) return i;
			i++;
		}
	}
	return -1;
}

/**
 * @fn Person::DecideMoveDirection()
 * Decide where to go from the current position.
 */
void Guest::DecideMoveDirection()
{
	const VoxelStack *vs = _world.GetStack(this->vox_pos.x, this->vox_pos.y);
	const Voxel *v = vs->Get(this->vox_pos.z);
	TileEdge start_edge = this->GetCurrentEdge(); // Edge the person is currently.

	if (this->activity == GA_ENTER_PARK && vs->owner == OWN_PARK) {
		// \todo Pay the park fee, go home if insufficient monies.
		this->activity = GA_WANDER;
		// Add some happiness?? (Somewhat useless as every guest enters the park. On the other hand, a nice point to configure difficulty level perhaps?)
	}

	/* Find feasible exits and shops. */
	uint8 exits, shops;
	bool queue_path;
	if (HasValidPath(v)) {
		bool seen_wanted_ride;
		exits = GetExitDirections(v, start_edge, &seen_wanted_ride, &queue_path);
		shops = exits >> 4;
		exits &= 0xF;

		if (!seen_wanted_ride) this->ride = nullptr; // Wanted ride has gone missing, stop looking for it.
	} else { // Not at a path -> lost.
		exits = 0xF;
		shops = 0;
		queue_path = false;
		this->ride = nullptr;
	}

	/* Switch between wandering and queuing depending on being on a queue path and having a desired ride. */
	if (this->activity == GA_WANDER) {
		if (queue_path && this->ride != nullptr) {
			this->activity = GA_QUEUING;
		} else {
			queue_path = false;
		}
	} else if (this->activity == GA_QUEUING) {
		if (this->ride == nullptr) {
			this->activity = GA_WANDER;
			queue_path = false;
		}
	}

	if (this->activity == GA_WANDER || this->activity == GA_QUEUING) { // Prevent wandering and queuing guests from walking out the park.
		uint8 exits_viable = this->GetInparkDirections();
		exits &= exits_viable;
		shops &= exits_viable;
	}

	/* Decide which direction to go. */
	SB(exits, start_edge, 1, 0); // Drop 'return' option until we find there are no other directions.
	uint8 walk_count = 0;
	uint8 shop_count = 0;
	for (TileEdge exit_edge = EDGE_BEGIN; exit_edge != EDGE_COUNT; exit_edge++) {
		if (GB(exits, exit_edge, 1) == 0) continue;
		walk_count++;
		if (GB(shops, exit_edge, 1) != 0) shop_count++;
	}
	/* No exits, or all normal shops: Add 'return' as option. */
	if (walk_count == 0 || (walk_count == shop_count && this->ride == nullptr)) SB(exits, start_edge, 1, 1);

	const WalkInformation *walks[4]; // Walks that can be done at this tile.
	walk_count = 0;
	for (TileEdge exit_edge = EDGE_BEGIN; exit_edge != EDGE_COUNT; exit_edge++) {
		if (GB(exits, exit_edge, 1) != 0) {
			if (GB(shops, exit_edge, 1) != 0) { // Moving to a shop.
				walks[walk_count++] = _center_path_tile[start_edge][exit_edge];
			} else if (queue_path) { // Queue path, walk at the center.
				walks[walk_count++] = _center_path_tile[start_edge][exit_edge];
			} else {
				walks[walk_count++] = _walk_path_tile[start_edge][exit_edge];
			}
		}
	}

	const WalkInformation *new_walk;
	if (walk_count == 1) {
		new_walk = walks[0];
	} else {
		new_walk = this->WalkForActivity(walks, walk_count, exits);
	}

	this->StartAnimation(new_walk);
}

/**
 * Get the directions to neighboring tiles that lead to or stay in the park.
 * @return Exits from the current tile that stay or lead into the park in the low nibble.
 */
uint8 Person::GetInparkDirections()
{
	uint8 exits = 0;
	for (TileEdge exit_edge = EDGE_BEGIN; exit_edge != EDGE_COUNT; exit_edge++) {
		Point16 dxy = _tile_dxy[exit_edge];

		if (this->vox_pos.x + dxy.x < 0 || this->vox_pos.x + dxy.x >= _world.GetXSize()) continue;
		if (this->vox_pos.y + dxy.y < 0 || this->vox_pos.y + dxy.y >= _world.GetYSize()) continue;

		if (_world.GetTileOwner(this->vox_pos.x + dxy.x, this->vox_pos.y + dxy.y) == OWN_PARK) {
			SB(exits, exit_edge, 1, 1);
		}
	}

	return exits;
}

/**
 * Find new walk based on activity.
 * @param walks Possible walks.
 * @param walk_count Number of available walks.
 * @param exits Valid exits for the walk.
 * @return Walk info for the activity.
 */
const WalkInformation *Guest::WalkForActivity(const WalkInformation **walks, uint8 walk_count, uint8 exits)
{
	const WalkInformation *new_walk;
	switch (this->activity) {
		case GA_ENTER_PARK: { // Find the park entrance.
			TileEdge desired = GetParkEntryDirection(this->vox_pos);
			int selected = GetDesiredEdgeIndex(desired, exits);
			if (selected < 0) selected = this->rnd.Uniform(walk_count - 1);
			new_walk = walks[selected];
			break;
		}

		case GA_GO_HOME: {
			TileEdge desired = GetGoHomeDirection(this->vox_pos);
			int selected = GetDesiredEdgeIndex(desired, exits);
			if (selected < 0) selected = this->rnd.Uniform(walk_count - 1);
			new_walk = walks[selected];
			break;
		}

		case GA_ON_RIDE:
			NOT_REACHED();

		default:
			new_walk = walks[this->rnd.Uniform(walk_count - 1)];
			break;
	}
	return new_walk;
}

/**
 * Perform the animation sequence as provided.
 * @param walk Walk information describing the animations to perform.
 */
void Person::StartAnimation(const WalkInformation *walk)
{
	const Animation *anim = _sprite_manager.GetAnimation(walk->anim_type, this->type);
	assert(anim != nullptr && anim->frame_count != 0);

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
void Person::DeActivate(AnimateResult ar)
{
	if (!this->IsActive()) return;

	if (ar == OAR_REMOVE && _world.VoxelExists(this->vox_pos)) {
		/* If not wandered off-world, remove the person from the voxel person list. */
		this->RemoveSelf(_world.GetCreateVoxel(this->vox_pos, false));
	}

	this->type = PERSON_INVALID;
	delete[] this->name;
	this->name = nullptr;
}

/**
 * Update the animation of a person.
 * @param delay Amount of milliseconds since the last update.
 * @return Whether to keep the person active or how to deactivate him/her.
 * @return Result code of the visit.
 */
AnimateResult Person::OnAnimate(int delay)
{
	this->frame_time -= delay;
	if (this->frame_time > 0) return OAR_OK;

	this->MarkDirty(); // Marks the entire voxel dirty, which should be big enough even after moving.

	if (this->frames == nullptr || this->frame_count == 0) return OAR_REMOVE;

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
	this->pix_pos.x += frame->dx;
	this->pix_pos.y += frame->dy;

	bool reached = false; // Set to true when we are beyond the limit!
	if ((this->walk->limit_type & (1 << WLM_END_LIMIT)) == WLM_X_COND) {
		if (frame->dx > 0) reached |= this->pix_pos.x > x_limit;
		if (frame->dx < 0) reached |= this->pix_pos.x < x_limit;

		if (y_limit >= 0) this->pix_pos.y += sign(y_limit - this->pix_pos.y); // Also slowly move the other axis in the right direction.
	} else {
		if (frame->dy > 0) reached |= this->pix_pos.y > y_limit;
		if (frame->dy < 0) reached |= this->pix_pos.y < y_limit;

		if (x_limit >= 0) this->pix_pos.x += sign(x_limit - this->pix_pos.x); // Also slowly move the other axis in the right direction.
	}

	if (!reached) {
		/* Not reached the end, do the next frame. */
		index++;
		if (this->frame_count <= index) index = 0;
		this->frame_index = index;
		this->frame_time = this->frames[index].duration;

		this->pix_pos.z = GetZHeight(this->vox_pos, this->pix_pos.x, this->pix_pos.y);
		return OAR_OK;
	}

	/* Reached the goal, start the next walk. */
	if (this->walk[1].anim_type != ANIM_INVALID) {
		this->StartAnimation(this->walk + 1);
		return OAR_OK;
	}

	/* Not only the end of this walk, but the end of the entire walk at the tile. */
	int dx = 0;
	int dy = 0;
	int dz = 0;
	TileEdge exit_edge = INVALID_EDGE;

	this->RemoveSelf(_world.GetCreateVoxel(this->vox_pos, false));
	if (this->pix_pos.x < 0) {
		dx--;
		this->vox_pos.x--;
		this->pix_pos.x += 256;
		exit_edge = EDGE_NE;
	} else if (this->pix_pos.x > 255) {
		dx++;
		this->vox_pos.x++;
		this->pix_pos.x -= 256;
		exit_edge = EDGE_SW;
	}
	if (this->pix_pos.y < 0) {
		dy--;
		this->vox_pos.y--;
		this->pix_pos.y += 256;
		exit_edge = EDGE_NW;
	} else if (this->pix_pos.y > 255) {
		dy++;
		this->vox_pos.y++;
		this->pix_pos.y -= 256;
		exit_edge = EDGE_SE;
	}
	assert(this->pix_pos.x >= 0 && this->pix_pos.x < 256);
	assert(this->pix_pos.y >= 0 && this->pix_pos.y < 256);

	AnimateResult ar = this->EdgeOfWorldOnAnimate();
	if (ar != OAR_CONTINUE) return ar;

	/* Handle raising of z position. */
	if (this->pix_pos.z > 128) {
		dz++;
		this->vox_pos.z++;
		this->pix_pos.z = 0;
	}
	/* At bottom of the voxel. */
	Voxel *v = _world.GetCreateVoxel(this->vox_pos, false);
	if (v != nullptr) {
		SmallRideInstance instance = v->GetInstance();
		if (instance >= SRI_FULL_RIDES) {
			assert(exit_edge != INVALID_EDGE);
			RideInstance *ri = _rides_manager.GetRideInstance(instance);
			AnimateResult ar = this->VisitRideOnAnimate(ri, exit_edge);
			if (ar != OAR_CONTINUE) return ar;

			/* Ride is could not be visited, fall-through to reversing movement. */

		} else if (HasValidPath(v)) {
			this->AddSelf(v);
			this->DecideMoveDirection();
			return OAR_OK;

		} else if (this->vox_pos.z > 0) { // Maybe a path below this voxel?
			dz--;
			this->vox_pos.z--;
			this->pix_pos.z = 255;
			Voxel *w = _world.GetCreateVoxel(this->vox_pos, false);
			if (w != nullptr && HasValidPath(w)) {
				this->AddSelf(w);
				this->DecideMoveDirection();
				return OAR_OK;
			}
		}

		/* Restore the person at the previous tile (ie reverse movement). */
		if (dx != 0) { this->vox_pos.x -= dx; this->pix_pos.x = (dx > 0) ? 255 : 0; }
		if (dy != 0) { this->vox_pos.y -= dy; this->pix_pos.y = (dy > 0) ? 255 : 0; }
		if (dz != 0) { this->vox_pos.z -= dz; this->pix_pos.z = (dz > 0) ? 255 : 0; }

		this->AddSelf(_world.GetCreateVoxel(this->vox_pos, false));
		this->DecideMoveDirection();
		return OAR_OK;
	}
	/* No voxel here, try one level below. */
	if (this->vox_pos.z > 0) {
		dz--;
		this->vox_pos.z--;
		this->pix_pos.z = 255;
		v = _world.GetCreateVoxel(this->vox_pos, false);
	}
	if (v != nullptr && HasValidPath(v)) {
		this->AddSelf(v);
		this->DecideMoveDirection();
		return OAR_OK;
	}
	return OAR_DEACTIVATE; // We are truly lost now.
}

/**
 * How much does the person desire to visit the given ride?
 * @param ri Ride that can be visited.
 * @return Desire of the person to visit the ride.
 */
RideVisitDesire Person::WantToVisit(const RideInstance *ri)
{
	return RVD_NO_VISIT;
}

/**
 * @fn bool Person::DailyUpdate()
 * Daily ponderings of a person.
 * @return If \c false, de-activate the person.
 */

/**
 * @fn AnimateResult Person::EdgeOfWorldOnAnimate()
 * Handle the case of a guest reaching the end of the game world.
 * @return Result code of the visit.
 */

/**
 * @fn AnimateResult Person::VisitRideOnAnimate(RideInstance *ri, TileEdge exit_edge)
 * Handle guest ride visiting.
 * @param ri Ride that can be visited.
 * @param exit_edge Exit edge being examined.
 * @return Result code of the visit.
 */

Guest::Guest() : Person()
{
}

Guest::~Guest()
{
}

void Guest::Activate(const Point16 &start, PersonType person_type)
{
	this->Person::Activate(start, person_type);

	this->activity = GA_ENTER_PARK;
	this->happiness = 50 + this->rnd.Uniform(50);
	this->total_happiness = 0;
	this->cash = 3000 + this->rnd.Uniform(4095);

	this->has_map = false;
	this->has_umbrella = false;
	this->has_balloon = false;
	this->has_wrapper = false;
	this->salty_food = false;
	this->food = 0;
	this->drink = 0;
	this->hunger_level = 50;
	this->thirst_level = 50;
	this->stomach_level = 0;
	this->waste = 0;
	this->nausea = 0;
	this->souvenirs = 0;
	this->ride = nullptr;
}

void Guest::DeActivate(AnimateResult ar)
{
	if (this->IsActive()) {
		/* Close possible Guest Info window */
		Window *wi = GetWindowByType(WC_GUEST_INFO, this->id);
		if (wi != nullptr) {
			_window_manager.DeleteWindow(wi);
		}

		/// \todo Evaluate Guest::total_happiness against scenario requirements for evaluating the park value.
	}

	this->Person::DeActivate(ar);
}

AnimateResult Guest::OnAnimate(int delay)
{
	if (this->activity == GA_ON_RIDE) return OAR_OK; // Guest is not animated while on ride.
	return this->Person::OnAnimate(delay);
}

AnimateResult Guest::EdgeOfWorldOnAnimate()
{
	/* If the guest ended up off-world, quit. */
	if (!IsVoxelstackInsideWorld(this->vox_pos.x, this->vox_pos.y)) return OAR_DEACTIVATE;

	/* If the guest arrived at the 'go home' tile while going home, quit. */
	if (this->activity == GA_GO_HOME && this->vox_pos.x == _guests.start_voxel.x && this->vox_pos.y == _guests.start_voxel.y) {
		return OAR_DEACTIVATE;
	}

	return OAR_CONTINUE;
}

AnimateResult Guest::VisitRideOnAnimate(RideInstance *ri, TileEdge exit_edge)
{
	if (ri->CanBeVisited(this->vox_pos, exit_edge) && this->SelectItem(ri) != ITP_NOTHING) {
		/* All lights are green, let's try to enter the ride. */
		this->activity = GA_ON_RIDE;
		this->ride = ri;
		RideEntryResult rer = ri->EnterRide(this->id, exit_edge);
		if (rer != RER_REFUSED) {
			this->BuyItem(ri);
			/* Either the guest is already back at a path or he will be (through ExitRide). */
			return OAR_OK;
		}

		/* Could not enter, find another ride. */
		this->ride = nullptr;
		this->activity = GA_WANDER;
	}
	return OAR_CONTINUE;
}

/**
 * Update the happiness of the guest.
 * @param amount Amount of change.
 */
void Guest::ChangeHappiness(int16 amount)
{
	if (amount == 0) return;

	int16 old_happiness = this->happiness;
	this->happiness = Clamp(this->happiness + amount, 0, 100);
	if (amount > 0) this->total_happiness = std::min(1000, this->total_happiness + this->happiness - old_happiness);
	NotifyChange(WC_GUEST_INFO, this->id, CHG_DISPLAY_OLD, 0);
}

/**
 * Daily ponderings of a guest.
 * @return If \c false, de-activate the guest.
 * @todo Make going home a bit more random.
 * @todo Implement dropping litter (Guest::has_wrapper) to the path, and also drop the wrapper when passing a non-empty litter bin.
 * @todo Implement nausea (Guest::nausea).
 * @todo Implement energy (for tiredness of guests).
 */
bool Guest::DailyUpdate()
{
	assert(this->IsGuest());

	/* Handle eating and drinking. */
	bool eating = false;
	if (this->food > 0) {
		this->food--;
		if (this->hunger_level >= 20) this->hunger_level -= 20;
		if (this->salty_food && this->thirst_level < 200) this->thirst_level += 5;
		eating = true;
	} else if (this->drink > 0) {
		this->drink--;
		if (this->thirst_level >= 20) this->thirst_level -= 20;
		eating = true;
	}
	if (this->hunger_level < 255) this->hunger_level++;
	if (this->thirst_level < 255) this->thirst_level++;

	if (eating && this->stomach_level < 250) this->stomach_level += 6;
	if (this->stomach_level > 0) {
		this->stomach_level--;
		if (this->waste < 255) this->waste++;
	}

	int16 happiness_change = 0;
	if (!eating) {
		if (this->has_wrapper && this->rnd.Success1024(25)) this->has_wrapper = false; // XXX Drop litter.
		if (this->hunger_level > 200) happiness_change--;
	}
	if (this->waste > 170) happiness_change -= 2;

	switch (_weather.GetWeatherType()) {
		case WTP_SUNNY:
			if (this->happiness < 80) happiness_change += 1;
			break;

		case WTP_LIGHT_CLOUDS:
		case WTP_THICK_CLOUDS:
			break;

		case WTP_RAINING:
		case WTP_THUNDERSTORM:
			if (!this->has_umbrella) happiness_change -= 5;
			break;

		default: NOT_REACHED();
	}

	this->ChangeHappiness(happiness_change);

	if (this->activity == GA_WANDER && this->happiness <= 10) this->activity = GA_GO_HOME; // Go home when bored.
	return true;
}

static const int WASTE_STOP_BUYING_FOOD = 150; ///< Waste level where the guest stop buying food.
static const int WASTE_MAY_TOILET       = 100; ///< Minimal level of waste before desiring to visit a toilet at all.
static const int WASTE_MUST_TOILET      = 200; ///< Level of waste before really needing to visit a toilet.
static const int NAUSEA_MUST_FIRST_AID  = 200; ///< Level of nausea before really needing help in reducing nausea.

/** Ensure that guests have a desire to visit a toilet before stopping to buy more food (and thus stop raising the waste level). */
assert_compile(WASTE_STOP_BUYING_FOOD > WASTE_MAY_TOILET);

/**
 * How useful is the item for the guest?
 * @param it Item offered.
 * @param use_random May use random to reject an item (else it should be accepted).
 * @return Desire of getting the item.
 */
RideVisitDesire Guest::NeedForItem(ItemType it, bool use_random)
{
	if (this->activity == GA_ENTER_PARK || this->activity == GA_GO_HOME) return RVD_NO_VISIT; // Not arrived yet, or going home -> no ride.

	/// \todo Make warm food attractive on cold days.
	switch (it) {
		case ITP_NOTHING:
			return RVD_NO_VISIT;

		case ITP_DRINK:
		case ITP_ICE_CREAM:
			if (this->food > 0 || this->drink > 0) return RVD_NO_VISIT;
			if (this->waste >= WASTE_STOP_BUYING_FOOD || this->stomach_level > 100) return RVD_NO_VISIT;
			if (_weather.temperature < 20) return RVD_NO_VISIT;
			if (use_random) return this->rnd.Success1024(this->thirst_level * 4 + _weather.temperature * 2) ? RVD_MAY_VISIT : RVD_NO_VISIT;
			return RVD_MAY_VISIT;

		case ITP_NORMAL_FOOD:
		case ITP_SALTY_FOOD:
			if (this->food > 0 || this->drink > 0) return RVD_NO_VISIT;
			if (this->waste >= WASTE_STOP_BUYING_FOOD || this->stomach_level > 100) return RVD_NO_VISIT;
			if (use_random) return this->rnd.Success1024(this->hunger_level * 4) ? RVD_MAY_VISIT : RVD_NO_VISIT;
			return RVD_MAY_VISIT;

		case ITP_UMBRELLA:
			return (this->has_umbrella) ? RVD_NO_VISIT : RVD_MAY_VISIT;

		case ITP_BALLOON:
			/// \todo Add some form or age? (just a "is_child" boolean would suffice)
			return (this->has_balloon) ? RVD_NO_VISIT : RVD_MAY_VISIT;

		case ITP_PARK_MAP:
			return (this->has_map) ? RVD_NO_VISIT : RVD_MAY_VISIT;

		case ITP_SOUVENIR:
			return (this->souvenirs < 2) ? RVD_MAY_VISIT : RVD_NO_VISIT;

		case ITP_MONEY:
			return (this->cash < 2000) ? RVD_MAY_VISIT : RVD_NO_VISIT;

		case ITP_TOILET:
			if (this->waste > WASTE_MUST_TOILET) return RVD_MUST_VISIT;
			return (this->waste >= WASTE_MAY_TOILET) ? RVD_MAY_VISIT : RVD_NO_VISIT;

		case ITP_FIRST_AID:
			return (this->nausea >= NAUSEA_MUST_FIRST_AID) ? RVD_MUST_VISIT : RVD_NO_VISIT;

		default: NOT_REACHED();
	}
}

RideVisitDesire Guest::WantToVisit(const RideInstance *ri)
{
	for (int i = 0; i < NUMBER_ITEM_TYPES_SOLD; i++) {
		if (this->NeedForItem(ri->GetSaleItemType(i), true) != RVD_NO_VISIT) return RVD_MAY_VISIT;
	}
	return RVD_NO_VISIT;
}

/**
 * Add an item to the possessions of the guest.
 * @param it Item to add.
 */
void Guest::AddItem(ItemType it)
{
	switch (it) {
		case ITP_NOTHING:
			break;

		case ITP_DRINK:
			this->drink = 5;
			this->has_wrapper = true;
			break;

		case ITP_ICE_CREAM:
			this->drink = 7;
			this->has_wrapper = false;
			break;

		case ITP_NORMAL_FOOD:
			this->food = 10;
			this->has_wrapper = true;
			this->salty_food = false;
			break;

		case ITP_SALTY_FOOD:
			this->food = 15;
			this->has_wrapper = true;
			this->salty_food = true;
			break;

		case ITP_UMBRELLA:
			this->has_umbrella = true;
			break;

		case ITP_BALLOON:
			this->has_balloon = true;
			break;

		case ITP_PARK_MAP:
			this->has_map = true;
			break;

		case ITP_SOUVENIR:
			if (this->souvenirs < 100) souvenirs++; // Arbitrary upper limit, unlikely to be ever reached.
			break;

		case ITP_MONEY:
			this->cash += 5000;
			break;

		case ITP_TOILET:
			this->waste = std::min<uint8>(this->waste, 10);
			break;

		case ITP_FIRST_AID:
			this->nausea = std::min<uint8>(this->nausea, 10);
			break;

		default: NOT_REACHED();
	}
}

/**
 * Select an item to buy from the ride.
 * @param ri Ride instance to buy from.
 * @return Item to buy, or #ITP_NOTHING if there is nothing of interest.
 */
ItemType Guest::SelectItem(const RideInstance *ri)
{
	bool can_buy[NUMBER_ITEM_TYPES_SOLD];
	int count = 0;
	for (int i = 0; i < NUMBER_ITEM_TYPES_SOLD; i++) {
		ItemType it = ri->GetSaleItemType(i);
		bool canbuy = true;
		if (it == ITP_NOTHING) canbuy = false;
		if (canbuy && ri->GetSaleItemPrice(i) > this->cash) canbuy = false;
		if (canbuy && this->NeedForItem(it, false) == RVD_NO_VISIT) canbuy = false;

		can_buy[i] = canbuy;
		if (canbuy) count++;
	}
	if (count == 0) return ITP_NOTHING;

	count = (count == 1) ? 1 : (1 + this->rnd.Uniform(count - 1));
	for (int i = 0; i < NUMBER_ITEM_TYPES_SOLD; i++) {
		if (!can_buy[i]) continue;
		if (count == 1) return ri->GetSaleItemType(i);
		count--;
	}
	return ITP_NOTHING;
}

/**
 * Buy an item from the ride.
 * @param ri Ride being visited.
 */
void Guest::BuyItem(RideInstance *ri)
{
	ItemType it = this->SelectItem(ri);
	if (it != ITP_NOTHING) {
		for (int i = 0; i < NUMBER_ITEM_TYPES_SOLD; i++) {
			if (it == ri->GetSaleItemType(i)) {
				ri->SellItem(i);
				this->cash -= ri->GetSaleItemPrice(i);
				this->AddItem(ri->GetSaleItemType(i));
				this->ChangeHappiness(10);
			}
		}
	}
	this->ChangeHappiness(-10);
}

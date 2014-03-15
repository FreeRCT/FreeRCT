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
#include "fileio.h"
#include "map.h"
#include "ride_type.h"
#include "viewport.h"

static PersonTypeData _person_type_datas[PERSON_TYPE_COUNT]; ///< Data about each type of person.

/**
 * Construct a recolour mapping of this person type.
 * @param rnd %Random number generator.
 * @return The constructed recolouring.
 */
Recolouring PersonTypeGraphics::MakeRecolouring(Random *rnd) const
{
	Recolouring recolour;
	for (int i = 0; i < (int)lengthof(this->random_recolours); i++) {
		const RandomRecolouringMapping &rrcm = this->random_recolours[i];
		if (rrcm.range_number == COL_RANGE_INVALID) continue;
		recolour.SetRecolouring(rrcm.range_number, rrcm.DrawRandomColour(rnd));
	}
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
 * @param length Length of the remaining block data.
 * @return Loading was a success.
 */
bool LoadPRSG(RcdFile *rcd_file, uint32 length)
{
	if (length < 1) return false;
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
			case  8: pt = PERSON_PILLAR;  break;
			case 16: pt = PERSON_EARTH;   break;
			default: pt = PERSON_INVALID; break;
		}

		if (pt != PERSON_INVALID) {
			PersonTypeData &ptd = ModifyPersonTypeData(pt);
			assert(NUMBER_PERSON_TYPE_RECOLOURINGS >= 3);
			ptd.graphics.random_recolours[0].Set(rc1);
			ptd.graphics.random_recolours[2].Set(rc2);
			ptd.graphics.random_recolours[3].Set(rc3);
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
	assert(PersonIsAGuest(this->type));

	int length = strlen(name);
	this->name = new char[length];
	memcpy(this->name, name, length);
	this->name[length] = '\0';
}

/**
 * Query the name of the person.
 * The name is returned in memory owned by the person. Do not free this data. It may change on each call.
 * @return Static buffer containing the name of the person.
 */
const char *Person::GetName() const
{
	static char buffer[16];

	assert(PersonIsAGuest(this->type));
	if (this->name != nullptr) return this->name;
	sprintf(buffer, "Guest %u", this->id);
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
	assert(this->type == PERSON_INVALID);
	assert(person_type != PERSON_INVALID);

	this->type = person_type;
	this->name = nullptr;

	/* Set up the person sprite recolouring table. */
	const PersonTypeData &person_type_data = GetPersonTypeData(this->type);
	this->recolour = person_type_data.graphics.MakeRecolouring(&this->rnd);

	/* Set up initial position. */
	this->x_vox = start.x;
	this->y_vox = start.y;
	this->z_vox = _world.GetGroundHeight(start.x, start.y);
	this->AddSelf(_world.GetCreateVoxel(this->x_vox, this->y_vox, this->z_vox, false));

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
	TileEdge start_edge = this->GetCurrentEdge(); // Edge the person is currently.

	/* Which way can the guest leave? */
	uint8 bot_exits = (1 << PATHBIT_NE) | (1 << PATHBIT_SW) | (1 << PATHBIT_SE) | (1 << PATHBIT_NW); // All exit directions at the bottom by default.
	uint8 shops = 0; // Number of exits with a shop with normal desire to go there.
	const Voxel *v = _world.GetVoxel(this->x_vox, this->y_vox, this->z_vox);
	if (HasValidPath(v)) {
		uint8 top_exits = 0; // Exits at the top of the voxel.

		uint8 slope = GetImplodedPathSlope(v);
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

		uint8 must_shops = 0; // Shops with a high desire to visit.

		/* Being at a path tile, make extra sure we don't leave the path. */
		for (TileEdge exit_edge = EDGE_BEGIN; exit_edge != EDGE_COUNT; exit_edge++) {
			int z; // Decide z position of the exit.
			if (GB(bot_exits, exit_edge + PATHBIT_NE, 1) != 0) {
				z = this->z_vox;
			} else if (GB(top_exits, exit_edge + PATHBIT_NE, 1) != 0) {
				z = this->z_vox + 1;
			} else {
				continue;
			}

			/* Decide whether to visit the exit at all. */
			RideVisitDesire rvd = RVD_NO_VISIT;
			if (exit_edge == start_edge) {
				/* Nothing to do, the edge is already blocked by the default value above. */
			} else if (PathExistsAtBottomEdge(this->x_vox, this->y_vox, z, exit_edge)) {
				rvd = RVD_NO_RIDE; // but a path instead.
			} else {
				const RideInstance *ri = RideExistsAtBottom(this->x_vox, this->y_vox, z, exit_edge);
				if (ri != nullptr) {
					Point16 dxy = _tile_dxy[exit_edge];
					if (ri->CanBeVisited(this->x_vox + dxy.x, this->y_vox + dxy.y, z, exit_edge)) rvd = this->WantToVisit(ri);
				}
			}
			switch (rvd) {
				case RVD_NO_RIDE:
					break; // A path is one of the options.

				case RVD_NO_VISIT: // No desire to visit this exit; clear it.
					SB(bot_exits, exit_edge + PATHBIT_NE, 1, 0);
					SB(top_exits, exit_edge + PATHBIT_NE, 1, 0);
					break;

				case RVD_MAY_VISIT: // It's one of the options (since the person is not coming from it).
					SB(shops, exit_edge + PATHBIT_NE, 1, 1);
					break;

				case RVD_MUST_VISIT: // It is very desirable to visit this shop (since the person is not coming from it).
					SB(must_shops, exit_edge + PATHBIT_NE, 1, 1);
					break;

				default: NOT_REACHED();
			}
		}
		bot_exits |= top_exits; // Difference between bottom-exit and top-exit is not relevant any more.
		if (must_shops != 0) {  // If there are shops that must be visited, ignore everything else.
			bot_exits &= must_shops;
			shops = must_shops;
		}
	}

	/* At this point:
	 *
	 * bot_exits = bit-set of exit directions,
	 * shops     = bit-set of directions that lead to a shop.
	 */

	const WalkInformation *walks[4]; // Walks that can be done at this tile.
	uint8 walk_count = 0;
	uint8 shop_count = 0;

	for (TileEdge exit_edge = EDGE_BEGIN; exit_edge != EDGE_COUNT; exit_edge++) {
		if (start_edge == exit_edge) continue;
		if (GB(bot_exits, exit_edge + PATHBIT_NE, 1) != 0) {
			if (GB(shops, exit_edge + PATHBIT_NE, 1) != 0) {
				walks[walk_count++] = _center_path_tile[start_edge][exit_edge];
				shop_count++;
			} else {
				walks[walk_count++] = _walk_path_tile[start_edge][exit_edge];
			}
		}
	}
	/* No exits, or all normal shops: Add 'return' as option. */
	if (walk_count == 0 || walk_count == shop_count) walks[walk_count++] = _walk_path_tile[start_edge][start_edge];

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
	if (this->type == PERSON_INVALID) return;

	/* Close possible Guest Info window */
	Window *wi = GetWindowByType(WC_GUEST_INFO, this->id);
	if (wi != nullptr) {
		_manager.DeleteWindow(wi);
	}

	if (ar == OAR_REMOVE && _world.VoxelExists(this->x_vox, this->y_vox, this->z_vox)) {
		/* If not wandered off-world, remove the person from the voxel person list. */
		this->RemoveSelf(_world.GetCreateVoxel(this->x_vox, this->y_vox, this->z_vox, false));
	}
	this->type = PERSON_INVALID;
	delete[] this->name;
	this->name = nullptr;
}

/**
 * Update the animation of the person.
 * @param delay Amount of milliseconds since the last update.
 * @return If \c false, de-activate the person.
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
	this->x_pos += frame->dx;
	this->y_pos += frame->dy;

	bool reached = false; // Set to true when we are beyond the limit!
	if ((this->walk->limit_type & (1 << WLM_END_LIMIT)) == WLM_X_COND) {
		if (frame->dx > 0) reached |= this->x_pos > x_limit;
		if (frame->dx < 0) reached |= this->x_pos < x_limit;

		if (y_limit >= 0) this->y_pos += sign(y_limit - this->y_pos); // Also slowly move the other axis in the right direction.
	} else {
		if (frame->dy > 0) reached |= this->y_pos > y_limit;
		if (frame->dy < 0) reached |= this->y_pos < y_limit;

		if (x_limit >= 0) this->x_pos += sign(x_limit - this->x_pos); // Also slowly move the other axis in the right direction.
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
	if (this->walk[1].anim_type != ANIM_INVALID) {
		this->StartAnimation(this->walk + 1);
		return OAR_OK;
	}

	/* Not only the end of this walk, but the end of the entire walk at the tile. */
	int dx = 0;
	int dy = 0;
	int dz = 0;
	TileEdge exit_edge = INVALID_EDGE;

	this->RemoveSelf(_world.GetCreateVoxel(this->x_vox, this->y_vox, this->z_vox, false));
	if (this->x_pos < 0) {
		dx--;
		this->x_vox--;
		this->x_pos += 256;
		exit_edge = EDGE_NE;
	} else if (this->x_pos > 255) {
		dx++;
		this->x_vox++;
		this->x_pos -= 256;
		exit_edge = EDGE_SW;
	}
	if (this->y_pos < 0) {
		dy--;
		this->y_vox--;
		this->y_pos += 256;
		exit_edge = EDGE_NW;
	} else if (this->y_pos > 255) {
		dy++;
		this->y_vox++;
		this->y_pos -= 256;
		exit_edge = EDGE_SE;
	}
	assert(this->x_pos >= 0 && this->x_pos < 256);
	assert(this->y_pos >= 0 && this->y_pos < 256);

	/* If the guest ended up off-world, quit. */
	if (this->x_vox < 0 || this->x_vox >= _world.GetXSize() * 256 ||
			this->y_vox < 0 || this->y_vox >= _world.GetYSize() * 256) {
		return OAR_DEACTIVATE;
	}

	/* Handle raising of z position. */
	if (this->z_pos > 128) {
		dz++;
		this->z_vox++;
		this->z_pos = 0;
	}
	/* At bottom of the voxel. */
	Voxel *v = _world.GetCreateVoxel(this->x_vox, this->y_vox, this->z_vox, false);
	if (v != nullptr) {
		SmallRideInstance instance = v->GetInstance();
		if (instance >= SRI_FULL_RIDES) {
			assert(exit_edge != INVALID_EDGE);
			RideInstance *ri = _rides_manager.GetRideInstance(instance);
			if (ri->CanBeVisited(this->x_vox, this->y_vox, this->z_vox, exit_edge)) {
				Guest *guest = dynamic_cast<Guest *>(this);
				if (guest != nullptr) guest->VisitShop(ri);
			}
			/* Ride is closed, or not a guest, fall-through to reversing movement. */

		} else if (HasValidPath(v)) {
			this->AddSelf(v);
			this->DecideMoveDirection();
			return OAR_OK;
		}

		/* Restore the person at the previous tile (ie reverse movement). */
		if (dx != 0) { this->x_vox -= dx; this->x_pos = (dx > 0) ? 255 : 0; }
		if (dy != 0) { this->y_vox -= dy; this->y_pos = (dy > 0) ? 255 : 0; }
		if (dz != 0) { this->z_vox -= dz; this->z_pos = (dz > 0) ? 255 : 0; }

		this->AddSelf(_world.GetCreateVoxel(this->x_vox, this->y_vox, this->z_vox, false));
		this->DecideMoveDirection();
		return OAR_OK;
	}
	/* No voxel here, try one level below. */
	if (this->z_vox > 0) {
		dz--;
		this->z_vox--;
		this->z_pos = 255;
		v = _world.GetCreateVoxel(this->x_vox, this->y_vox, this->z_vox, false);
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

Guest::Guest() : Person()
{
}

Guest::~Guest()
{
}

void Guest::Activate(const Point16 &start, PersonType person_type)
{
	this->Person::Activate(start, person_type);

	this->happiness = 50 + this->rnd.Uniform(50);
	this->cash = 3000 + this->rnd.Uniform(4095);

	this->has_map = false;
	this->has_umbrella = false;
	this->has_wrapper = false;
	this->salty_food = false;
	this->food = 0;
	this->drink = 0;
	this->hunger_level = 50;
	this->thirst_level = 50;
	this->stomach_level = 0;
	this->waste = 0;
}

/**
 * Daily ponderings of a guest.
 * @return If \c false, de-activate the guest.
 * @todo Make de-activation a bit more random.
 * @todo Implement dropping litter (Guest::has_wrapper) to the path, and also drop the wrapper when passing a non-empty litter bin.
 */
bool Guest::DailyUpdate()
{
	assert(PersonIsAGuest(this->type));

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

	if (!eating) {
		if (this->has_wrapper && this->rnd.Success1024(25)) this->has_wrapper = false; // XXX Drop litter.
		if (this->happiness > 0) this->happiness--;
	}
	if (this->waste > 100) this->happiness = std::max(0, this->happiness - 2);

	NotifyChange(WC_GUEST_INFO, this->id, CHG_DISPLAY_OLD, 0);

	return this->happiness > 10; // De-activate if at or below 10.
}

/**
 * How useful is the item for the guest?
 * @param it Item offered.
 * @param use_random May use random to reject an item (else it should be accepted).
 * @return Desire of getting the item.
 */
RideVisitDesire Guest::NeedForItem(ItemType it, bool use_random)
{
	switch (it) {
		case ITP_NOTHING:
			return RVD_NO_VISIT;

		case ITP_DRINK:
		case ITP_ICE_CREAM:
			if (this->food > 0 || this->drink > 0) return RVD_NO_VISIT;
			if (this->waste > 100 || this->stomach_level > 100) return RVD_NO_VISIT;
			if (use_random) return this->rnd.Success1024(this->thirst_level * 4) ? RVD_MAY_VISIT : RVD_NO_VISIT;
			return RVD_MAY_VISIT;

		case ITP_NORMAL_FOOD:
		case ITP_SALTY_FOOD:
			if (this->food > 0 || this->drink > 0) return RVD_NO_VISIT;
			if (this->waste > 100 || this->stomach_level > 100) return RVD_NO_VISIT;
			if (use_random) return this->rnd.Success1024(this->hunger_level * 4) ? RVD_MAY_VISIT : RVD_NO_VISIT;
			return RVD_MAY_VISIT;

		case ITP_UMBRELLA:
			return (this->has_umbrella) ? RVD_NO_VISIT : RVD_MAY_VISIT;

		case ITP_PARK_MAP:
			return (this->has_map) ? RVD_NO_VISIT : RVD_MAY_VISIT;

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

		case ITP_PARK_MAP:
			this->has_map = true;
			break;

		default: NOT_REACHED();
	}
}

/**
 * Visit the shop.
 * @param ri Ride being visited.
 * @note It is called 'shop', but it is quite generic, so it may be usable for more rides (without queue in front of it).
 */
void Guest::VisitShop(RideInstance *ri)
{
	bool can_buy[NUMBER_ITEM_TYPES_SOLD];
	int count = 0;
	for (int i = 0; i < NUMBER_ITEM_TYPES_SOLD; i++) {
		can_buy[i] = ri->GetSaleItemPrice(i) <= this->cash && this->NeedForItem(ri->GetSaleItemType(i), false) != RVD_NO_VISIT;
		if (can_buy[i]) count++;
	}
	if (count > 1) {
		count = 1 + this->rnd.Uniform(count - 1);
		for (int i = 0; i < NUMBER_ITEM_TYPES_SOLD; i++) {
			if (!can_buy[i]) continue;
			if (count != 1) can_buy[i] = false;
			count = std::max(0, count - 1);
		}
	}

	for (int i = 0; i < NUMBER_ITEM_TYPES_SOLD; i++) {
		if (can_buy[i]) {
			ri->SellItem(i);
			this->cash -= ri->GetSaleItemPrice(i);
			this->AddItem(ri->GetSaleItemType(i));
			this->happiness = std::min(100, this->happiness + 10);
			return;
		}
	}

	this->happiness = std::max(0, this->happiness - 10); // Cannot buy anything!

	NotifyChange(WC_GUEST_INFO, this->id, CHG_DISPLAY_OLD, 0);
}

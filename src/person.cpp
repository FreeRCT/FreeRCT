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
#include "person.h"
#include "people.h"
#include "fileio.h"
#include "map.h"
#include "messages.h"
#include "path_finding.h"
#include "scenery.h"
#include "viewport.h"
#include "weather.h"

#include <cmath>

static PersonTypeData _person_type_datas[PERSON_TYPE_COUNT]; ///< Data about each type of person.

static const int QUEUE_DISTANCE = 64;  // The pixel distance between two guests queuing for a ride.
assert_compile(256 % QUEUE_DISTANCE == 0);

const std::map<PersonType, Money> StaffMember::SALARY = {
	{PERSON_MECHANIC,    Money(270)},  ///< Daily salary of a mechanic.
	{PERSON_HANDYMAN,    Money(150)},  ///< Daily salary of a handyman.
	{PERSON_GUARD,       Money(200)},  ///< Daily salary of a guard.
	{PERSON_ENTERTAINER, Money(200)},  ///< Daily salary of a entertainer.
};

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
	this->ride = nullptr;
	this->status = GUI_PERSON_STATUS_WANDER;

	this->offset = this->rnd.Uniform(100);
}

Person::~Person()
{
	NotifyChange(WC_PERSON_INFO, this->id, CHG_PERSON_DELETED, 0);
}

const ImageData *Person::GetSprite(const SpriteStorage *sprites, ViewOrientation orient, const Recolouring **recolour) const
{
	*recolour = &this->recolour;
	AnimationType anim_type = this->walk->anim_type;
	return sprites->GetAnimationSprite(anim_type, this->frame_index, this->type, orient);
}

/**
 * Set the name of a person.
 * @param name New name of the person.
 */
void Person::SetName(const std::string &name)
{
	this->name = name;
}

/**
 * Query the name of the person.
 * @return The name of the person.
 */
std::string Person::GetName() const
{
	if (!this->name.empty()) return this->name;

	static char buffer[16];
	sprintf(buffer, "Guest %u", this->id);  // \todo Use a translatable string for this.
	return buffer;
}

/** Recompute the height of the person. */
void Person::UpdateZPosition()
{
	Voxel *v = _world.GetCreateVoxel(this->vox_pos, false);
	assert(v != nullptr);

	if (HasValidPath(v)) {
		uint8 slope = GetImplodedPathSlope(v);
		if (slope < PATH_FLAT_COUNT) {
			this->pix_pos.z = 0;
			return;
		}
		switch (slope) {
			case PATH_RAMP_NE: this->pix_pos.z =       this->pix_pos.x; return;
			case PATH_RAMP_NW: this->pix_pos.z =       this->pix_pos.y; return;
			case PATH_RAMP_SE: this->pix_pos.z = 255 - this->pix_pos.y; return;
			case PATH_RAMP_SW: this->pix_pos.z = 255 - this->pix_pos.x; return;
			default: NOT_REACHED();
		}
	}
	/* No path here. */

	if (v->GetGroundType() == GTP_INVALID) {
		/* The person ended up above or below the ground. Fall down or teleport upwards. */
		const VoxelStack *vs = _world.GetStack(this->vox_pos.x, this->vox_pos.y);
		const int base_z = vs->base + vs->GetTopGroundOffset();
		assert(base_z != this->vox_pos.z);

		this->RemoveSelf(v);
		this->vox_pos.z = base_z;
		this->pix_pos.z = 0;
		v = _world.GetCreateVoxel(this->vox_pos, false);
		assert(v != nullptr);
		assert(v->GetGroundType() != GTP_INVALID);
		this->AddSelf(v);
		if (HasValidPath(v)) return this->UpdateZPosition();  // Ended up on a path.
		/* Fall through to slope detection. */
	}

	/* Pathless land. Discover on which part of the slope the person is standing. */

	switch (v->GetGroundSlope()) {
		/* Flat land. */
		case ISL_FLAT: this->pix_pos.z = 0; return;

		/* Ramps. */
		case ISL_SOUTH_WEST: this->pix_pos.z =       this->pix_pos.x; return;
		case ISL_EAST_SOUTH: this->pix_pos.z =       this->pix_pos.y; return;
		case ISL_NORTH_WEST: this->pix_pos.z = 255 - this->pix_pos.y; return;
		case ISL_NORTH_EAST: this->pix_pos.z = 255 - this->pix_pos.x; return;

		/* One corner up. The voxel is split into a flat and a raised triangle. */
		case ISL_NORTH:
			if (this->pix_pos.x + this->pix_pos.y >= 255) {  // Flat triangle.
				this->pix_pos.z = 0;
			} else {  // Raised triangle.
				this->pix_pos.z = 255 - (this->pix_pos.x + this->pix_pos.y);
			}
			return;
		case ISL_SOUTH:
			if (this->pix_pos.x + this->pix_pos.y <= 255) {  // Flat triangle.
				this->pix_pos.z = 0;
			} else {  // Raised triangle.
				this->pix_pos.z = (this->pix_pos.x + this->pix_pos.y) - 255;
			}
			return;
		case ISL_WEST:
			if (this->pix_pos.x <= this->pix_pos.y) {  // Flat triangle.
				this->pix_pos.z = 0;
			} else {  // Raised triangle.
				this->pix_pos.z = std::max(this->pix_pos.x, this->pix_pos.y) - std::min(this->pix_pos.x, this->pix_pos.y);
			}
			return;
		case ISL_EAST:
			if (this->pix_pos.x >= this->pix_pos.y) {  // Flat triangle.
				this->pix_pos.z = 0;
			} else {  // Raised triangle.
				this->pix_pos.z = std::max(this->pix_pos.x, this->pix_pos.y) - std::min(this->pix_pos.x, this->pix_pos.y);
			}
			return;

		/* Two opposite corners up. The voxel consists of two triangles raised in opposite directions. */
		case ISL_NORTH_SOUTH:
			if (this->pix_pos.x + this->pix_pos.y <= 255) {  // Like the raised triangle of a TSB_NORTH slope.
				this->pix_pos.z = 255 - (this->pix_pos.x + this->pix_pos.y);
			} else {  // Like the raised triangle of a TSB_SOUTH slope.
				this->pix_pos.z = (this->pix_pos.x + this->pix_pos.y) - 255;
			}
			return;
		case ISL_EAST_WEST:  // Like the raised triangles of a TSB_WEST or TSB_EAST slope.
			this->pix_pos.z = std::max(this->pix_pos.x, this->pix_pos.y) - std::min(this->pix_pos.x, this->pix_pos.y);
			return;

		/* One corner down. The voxel is split into a flat and a lowered triangle, similar to the 'one corner up' case. */
		case ISL_NORTH_EAST_WEST:  // South corner down.
			if (this->pix_pos.x + this->pix_pos.y <= 255) {  // Flat triangle.
				this->pix_pos.z = 255;
			} else {  // Lowered triangle.
				this->pix_pos.z = 255 - ((this->pix_pos.x + this->pix_pos.y) - 255);
			}
			return;
		case ISL_EAST_SOUTH_WEST:  // North corner down.
			if (this->pix_pos.x + this->pix_pos.y >= 255) {  // Flat triangle.
				this->pix_pos.z = 255;
			} else {  // Lowered triangle.
				this->pix_pos.z = this->pix_pos.x + this->pix_pos.y;
			}
			return;
		case ISL_NORTH_EAST_SOUTH:  // West corner down.
			if (this->pix_pos.x <= this->pix_pos.y) {  // Flat triangle.
				this->pix_pos.z = 255;
			} else {  // Lowered triangle.
				this->pix_pos.z = 255 - (std::max(this->pix_pos.x, this->pix_pos.y) - std::min(this->pix_pos.x, this->pix_pos.y));
			}
			return;
		case ISL_NORTH_SOUTH_WEST:  // East corner down.
			if (this->pix_pos.x >= this->pix_pos.y) {  // Flat triangle.
				this->pix_pos.z = 255;
			} else {  // Lowered triangle.
				this->pix_pos.z = 255 - (std::max(this->pix_pos.x, this->pix_pos.y) - std::min(this->pix_pos.x, this->pix_pos.y));
			}
			return;

		/* Imploded slopes below. We additionally need to check if the person is in the correct half of the slope. */
		case ISL_BOTTOM_STEEP_NORTH:
			if (this->pix_pos.x + this->pix_pos.y <= 255) {  // Wrong part, move up.
				this->RemoveSelf(v);
				this->vox_pos.z++;
				v = _world.GetCreateVoxel(this->vox_pos, false);
				assert(v != nullptr);
				assert(v->GetGroundType() != GTP_INVALID);
				assert(v->GetGroundSlope() == ISL_TOP_STEEP_NORTH);
				this->AddSelf(v);
				this->UpdateZPosition();
			} else {  // Like for voxels with a lowered south corner.
				this->pix_pos.z = 255 - ((this->pix_pos.x + this->pix_pos.y) - 255);
			}
			return;
		case ISL_BOTTOM_STEEP_SOUTH:
			if (this->pix_pos.x + this->pix_pos.y >= 255) {  // Wrong part, move up.
				this->RemoveSelf(v);
				this->vox_pos.z++;
				v = _world.GetCreateVoxel(this->vox_pos, false);
				assert(v != nullptr);
				assert(v->GetGroundType() != GTP_INVALID);
				assert(v->GetGroundSlope() == ISL_TOP_STEEP_SOUTH);
				this->AddSelf(v);
				this->UpdateZPosition();
			} else {  // Like for voxels with a lowered north corner.
				this->pix_pos.z = this->pix_pos.x + this->pix_pos.y;
			}
			return;
		case ISL_BOTTOM_STEEP_EAST:
			if (this->pix_pos.x <= this->pix_pos.y) {  // Wrong part, move up.
				this->RemoveSelf(v);
				this->vox_pos.z++;
				v = _world.GetCreateVoxel(this->vox_pos, false);
				assert(v != nullptr);
				assert(v->GetGroundType() != GTP_INVALID);
				assert(v->GetGroundSlope() == ISL_TOP_STEEP_EAST);
				this->AddSelf(v);
				this->UpdateZPosition();
			} else {  // Like for voxels with a lowered west corner.
				this->pix_pos.z = 255 - (std::max(this->pix_pos.x, this->pix_pos.y) - std::min(this->pix_pos.x, this->pix_pos.y));
			}
			return;
		case ISL_BOTTOM_STEEP_WEST:
			if (this->pix_pos.x >= this->pix_pos.y) {  // Wrong part, move up.
				this->RemoveSelf(v);
				this->vox_pos.z++;
				v = _world.GetCreateVoxel(this->vox_pos, false);
				assert(v != nullptr);
				assert(v->GetGroundType() != GTP_INVALID);
				assert(v->GetGroundSlope() == ISL_TOP_STEEP_WEST);
				this->AddSelf(v);
				this->UpdateZPosition();
			} else {  // Like for voxels with a lowered east corner.
				this->pix_pos.z = 255 - (std::max(this->pix_pos.x, this->pix_pos.y) - std::min(this->pix_pos.x, this->pix_pos.y));
			}
			return;
		case ISL_TOP_STEEP_NORTH:
			if (this->pix_pos.x + this->pix_pos.y > 255) {  // Wrong part, move down.
				this->RemoveSelf(v);
				this->vox_pos.z--;
				v = _world.GetCreateVoxel(this->vox_pos, false);
				assert(v != nullptr);
				assert(v->GetGroundType() != GTP_INVALID);
				assert(v->GetGroundSlope() == ISL_BOTTOM_STEEP_NORTH);
				this->AddSelf(v);
				this->UpdateZPosition();
			} else {  // Like for voxels with a raised north corner.
				this->pix_pos.z = 255 - (this->pix_pos.x + this->pix_pos.y);
			}
			return;
		case ISL_TOP_STEEP_SOUTH:
			if (this->pix_pos.x + this->pix_pos.y < 255) {  // Wrong part, move down.
				this->RemoveSelf(v);
				this->vox_pos.z--;
				v = _world.GetCreateVoxel(this->vox_pos, false);
				assert(v != nullptr);
				assert(v->GetGroundType() != GTP_INVALID);
				assert(v->GetGroundSlope() == ISL_BOTTOM_STEEP_SOUTH);
				this->AddSelf(v);
				this->UpdateZPosition();
			} else {  // Like for voxels with a raised south corner.
				this->pix_pos.z = (this->pix_pos.x + this->pix_pos.y) - 255;
			}
			return;
		case ISL_TOP_STEEP_EAST:
			if (this->pix_pos.x > this->pix_pos.y) {  // Wrong part, move down.
				this->RemoveSelf(v);
				this->vox_pos.z--;
				v = _world.GetCreateVoxel(this->vox_pos, false);
				assert(v != nullptr);
				assert(v->GetGroundType() != GTP_INVALID);
				assert(v->GetGroundSlope() == ISL_BOTTOM_STEEP_EAST);
				this->AddSelf(v);
				this->UpdateZPosition();
			} else {  // Like for voxels with a raised east corner.
				this->pix_pos.z = std::max(this->pix_pos.x, this->pix_pos.y) - std::min(this->pix_pos.x, this->pix_pos.y);
			}
			return;
		case ISL_TOP_STEEP_WEST:
			if (this->pix_pos.x < this->pix_pos.y) {  // Wrong part, move down.
				this->RemoveSelf(v);
				this->vox_pos.z--;
				v = _world.GetCreateVoxel(this->vox_pos, false);
				assert(v != nullptr);
				assert(v->GetGroundType() != GTP_INVALID);
				assert(v->GetGroundSlope() == ISL_BOTTOM_STEEP_WEST);
				this->AddSelf(v);
				this->UpdateZPosition();
			} else {  // Like for voxels with a raised west corner.
				this->pix_pos.z = std::max(this->pix_pos.x, this->pix_pos.y) - std::min(this->pix_pos.x, this->pix_pos.y);
			}
			return;

		default: NOT_REACHED();
	}
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
	this->name.clear();
	this->SetStatus(GUI_PERSON_STATUS_WANDER);

	/* Set up the person sprite recolouring table. */
	const PersonTypeData &person_type_data = GetPersonTypeData(this->type);
	this->recolour = person_type_data.graphics.MakeRecolouring();

	/* Set up initial position. */
	this->vox_pos.x = start.x;
	this->vox_pos.y = start.y;
	this->vox_pos.z = _world.GetBaseGroundHeight(start.x, start.y);
	this->AddSelf(_world.GetCreateVoxel(this->vox_pos, false));
	this->queuing_blocked_on = nullptr;

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
	this->UpdateZPosition();

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

/** Motionless "walks" when a guest sits on a bench. */
static const WalkInformation _guest_bench[4][4] = {
	{{ANIM_GUEST_BENCH_NE, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
	{{ANIM_GUEST_BENCH_SE, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
	{{ANIM_GUEST_BENCH_SW, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
	{{ANIM_GUEST_BENCH_NW, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
};

/** Motionless "walks" when a mechanic repairs a ride. */
static const WalkInformation _mechanic_repair[4][4] = {
	{{ANIM_MECHANIC_REPAIR_NE, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
	{{ANIM_MECHANIC_REPAIR_SE, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
	{{ANIM_MECHANIC_REPAIR_SW, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
	{{ANIM_MECHANIC_REPAIR_NW, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
};

/** Motionless "walks" when a handyman waters the flowerbeds. */
static const WalkInformation _handyman_water[4][4] = {
	{{ANIM_HANDYMAN_WATER_NE, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
	{{ANIM_HANDYMAN_WATER_SE, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
	{{ANIM_HANDYMAN_WATER_SW, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
	{{ANIM_HANDYMAN_WATER_NW, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
};

/** Motionless "walks" when a handyman sweeps the paths. */
static const WalkInformation _handyman_sweep[4][4] = {
	{{ANIM_HANDYMAN_SWEEP_NE, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
	{{ANIM_HANDYMAN_SWEEP_SE, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
	{{ANIM_HANDYMAN_SWEEP_SW, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
	{{ANIM_HANDYMAN_SWEEP_NW, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
};

/** Motionless "walks" when a handyman empties a litter bin. */
static const WalkInformation _handyman_empty[4][4] = {
	{{ANIM_HANDYMAN_EMPTY_NE, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
	{{ANIM_HANDYMAN_EMPTY_SE, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
	{{ANIM_HANDYMAN_EMPTY_SW, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
	{{ANIM_HANDYMAN_EMPTY_NW, WLM_INVALID}, {ANIM_INVALID, WLM_INVALID}},
};

/** Encodes and decodes walk information for use in savegames. */
struct WalkEncoder {
	/**
	 * Encodes a given walk.
	 * @param wi Walk to encode.
	 * @return The encoded walk.
	 */
	static uint16 Encode(const WalkInformation *const wi)
	{
		WalkEncoder encoder;

		for (int i = 0; i < 4; i++) {
			if (wi == _mechanic_repair[i]) {
				encoder.SetType(2);
				encoder.SetSubtype(0);
				encoder.SetLowerParam(i);
				return encoder.value;
			}
		}
		for (int i = 0; i < 4; i++) {
			if (wi == _handyman_water[i]) {
				encoder.SetType(2);
				encoder.SetSubtype(1);
				encoder.SetLowerParam(i);
				return encoder.value;
			}
		}
		for (int i = 0; i < 4; i++) {
			if (wi == _handyman_sweep[i]) {
				encoder.SetType(2);
				encoder.SetSubtype(2);
				encoder.SetLowerParam(i);
				return encoder.value;
			}
		}
		for (int i = 0; i < 4; i++) {
			if (wi == _guest_bench[i]) {
				encoder.SetType(2);
				encoder.SetSubtype(3);
				encoder.SetLowerParam(i);
				return encoder.value;
			}
		}
		for (int i = 0; i < 4; i++) {
			if (wi == _handyman_sweep[i]) {
				encoder.SetType(2);
				encoder.SetSubtype(4);
				encoder.SetLowerParam(i);
				return encoder.value;
			}
		}

		for (uint8 subtype = 0; subtype < 4; subtype++) {
			for (uint8 upper_param = 0; upper_param < 4; upper_param++) {
				const WalkInformation *walk = _center_path_tile[subtype][upper_param];
				uint8 lower_param = 0;
				while (walk->anim_type != ANIM_INVALID) {
					if (walk == wi) {
						encoder.SetType(1);
						encoder.SetSubtype(subtype);
						encoder.SetUpperParam(upper_param);
						encoder.SetLowerParam(lower_param);
						return encoder.value;
					}
					lower_param++;
					walk++;
				}

				walk = _walk_path_tile[subtype][upper_param];
				lower_param = 0;
				while (walk->anim_type != ANIM_INVALID) {
					if (walk == wi) {
						encoder.SetType(0);
						encoder.SetSubtype(subtype);
						encoder.SetUpperParam(upper_param);
						encoder.SetLowerParam(lower_param);
						return encoder.value;
					}
					lower_param++;
					walk++;
				}
			}
		}

		NOT_REACHED();
	}

	/**
	 * Decodes a given walk.
	 * @param code Encoded walk
	 * @return The decoded walk.
	 */
	static const WalkInformation *Decode(const uint16 code)
	{
		const WalkEncoder decoder(code);
		switch (decoder.GetType()) {
			case 0: return &_walk_path_tile  [decoder.GetSubtype()][decoder.GetUpperParam()][decoder.GetLowerParam()];
			case 1: return &_center_path_tile[decoder.GetSubtype()][decoder.GetUpperParam()][decoder.GetLowerParam()];
			case 2:
				switch (decoder.GetSubtype()) {
					case 0: return _mechanic_repair[decoder.GetLowerParam()];
					case 1: return _handyman_water [decoder.GetLowerParam()];
					case 2: return _handyman_sweep [decoder.GetLowerParam()];
					case 3: return _guest_bench    [decoder.GetLowerParam()];
					case 4: return _handyman_empty [decoder.GetLowerParam()];
					default: NOT_REACHED();
				}
			default: NOT_REACHED();
		}
	}

private:
	/**
	 * Retrieve the "type" field of this encoded walk.
	 * @return The type in (0..15).
	 */
	uint8 GetType() const
	{
		return (value >> 12) & 0xF;
	}

	/**
	 * Retrieve the "subtype" field of this encoded walk.
	 * @return The subtype in (0..15).
	 */
	uint8 GetSubtype() const
	{
		return (value >> 8) & 0xF;
	}

	/**
	 * Retrieve the "upper parameter" field of this encoded walk.
	 * @return The upper parameter in (0..15).
	 */
	uint8 GetUpperParam() const
	{
		return (value >> 4) & 0xF;
	}

	/**
	 * Retrieve the "lower parameter" field of this encoded walk.
	 * @return The lower parameter in (0..15).
	 */
	uint8 GetLowerParam() const
	{
		return value & 0xF;
	}

	/**
	 * Set the "type" field of this encoded walk.
	 * @param val Value for this field in (0..15).
	 */
	void SetType(const uint8 val)
	{
		value &= 0x0FFF;
		value |= (val << 12);
	}

	/**
	 * Set the "subtype" field of this encoded walk.
	 * @param val Value for this field in (0..15).
	 */
	void SetSubtype(const uint8 val)
	{
		value &= 0xF0FF;
		value |= (val << 8);
	}

	/**
	 * Set the "upper parameter" field of this encoded walk.
	 * @param val Value for this field in (0..15).
	 */
	void SetUpperParam(const uint8 val)
	{
		value &= 0xFF0F;
		value |= (val << 4);
	}

	/**
	 * Set the "lower parameter" field of this encoded walk.
	 * @param val Value for this field in (0..15).
	 */
	void SetLowerParam(const uint8 val)
	{
		value &= 0xFFF0;
		value |= val;
	}

	/** Private constructor. */
	WalkEncoder(uint16 v = 0) : value(v) {}

	uint16 value;  ///< Encoded value to store in savegames.
};

static const uint32 CURRENT_VERSION_Person      = 3;   ///< Currently supported version of %Person.
static const uint32 CURRENT_VERSION_Guest       = 3;   ///< Currently supported version of %Guest.
static const uint32 CURRENT_VERSION_StaffMember = 2;   ///< Currently supported version of %StaffMember.
static const uint32 CURRENT_VERSION_Mechanic    = 2;   ///< Currently supported version of %Mechanic.
static const uint32 CURRENT_VERSION_Handyman    = 1;   ///< Currently supported version of %Handyman.
static const uint32 CURRENT_VERSION_Guard       = 1;   ///< Currently supported version of %Guard.
static const uint32 CURRENT_VERSION_Entertainer = 1;   ///< Currently supported version of %Entertainer.

/**
 * Load a person from the save game.
 * @param ldr Input stream to read.
 */
void Person::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("prsn");
	if (version < 1 || version > CURRENT_VERSION_Person) ldr.version_mismatch(version, CURRENT_VERSION_Person);
	this->VoxelObject::Load(ldr);

	this->queuing_blocked_on = nullptr;  // Will be recalculated later in OnAnimate().

	this->type = (PersonType)ldr.GetByte();
	this->offset = ldr.GetWord();
	this->name = ldr.GetText();

	if (version > 1) {
		const uint16 ride_index = ldr.GetWord();
		if (ride_index != INVALID_RIDE_INSTANCE) this->ride = _rides_manager.GetRideInstance(ride_index);
	}

	const PersonTypeData &person_type_data = GetPersonTypeData(this->type);
	this->recolour = person_type_data.graphics.MakeRecolouring();
	this->recolour.Load(ldr);

	this->walk = WalkEncoder::Decode(ldr.GetWord());
	this->frame_index = ldr.GetWord();
	this->frame_time = (int16)ldr.GetWord();

	if (version >= 3) this->status = GUI_PERSON_STATUS_WANDER + ldr.GetWord();

	const Animation *anim = _sprite_manager.GetAnimation(walk->anim_type, this->type);
	assert(anim != nullptr && anim->frame_count != 0);

	this->frames = anim->frames.get();
	this->frame_count = anim->frame_count;

	this->AddSelf(_world.GetCreateVoxel(this->vox_pos, false));
	this->MarkDirty();
	ldr.ClosePattern();
}

/**
 * Save person data to the save game file.
 * @param svr Output stream to write.
 */
void Person::Save(Saver &svr)
{
	svr.StartPattern("prsn", CURRENT_VERSION_Person);
	this->VoxelObject::Save(svr);

	svr.PutByte(this->type);
	svr.PutWord(this->offset);
	svr.PutText(this->name);
	svr.PutWord((this->ride != nullptr) ? this->ride->GetIndex() : INVALID_RIDE_INSTANCE);

	this->recolour.Save(svr);

	svr.PutWord(WalkEncoder::Encode(this->walk));
	svr.PutWord(this->frame_index);
	svr.PutWord((uint16)this->frame_time);
	svr.PutWord(this->status - GUI_PERSON_STATUS_WANDER);
	svr.EndPattern();
}

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
 * Checks whether this person is in the process of deliberately walking from a path onto pathless land.
 * @return The person wants to leave the path.
 */
bool Person::IsLeavingPath() const
{
	return false;
}

/**
 * Decide whether visiting the exit edge is useful.
 * @param current_edge Edge at the current position.
 * @param cur_pos Coordinate of the current voxel.
 * @param exit_edge Exit edge being examined.
 * @param seen_wanted_ride [inout] Whether the wanted ride is seen.
 * @return Desire of the person to visit the indicated edge.
 */
RideVisitDesire Person::ComputeExitDesire(TileEdge current_edge, XYZPoint16 cur_pos, TileEdge exit_edge, bool *seen_wanted_ride)
{
	if (current_edge == exit_edge) return RVD_NO_VISIT; // Skip incoming edge (may get added later if no other options exist).

	const TileEdge original_exit_edge = exit_edge;
	const XYZPoint16 original_cur_pos = cur_pos;
	bool travel = TravelQueuePath(&cur_pos, &exit_edge);
	if (!travel) return RVD_NO_VISIT; // Path leads to nowhere.

	if (PathExistsAtBottomEdge(cur_pos, exit_edge)) return RVD_NO_RIDE; // Found a path.

	RideInstance *ri = RideExistsAtBottom(cur_pos, exit_edge);
	if (ri == nullptr) return RVD_NO_VISIT;  // No ride here.

	if (this->type != PERSON_MECHANIC) {  // Some limitations that apply to guests but not to mechanics.
		Point16 dxy = _tile_dxy[exit_edge];
		if (!ri->CanBeVisited(cur_pos + XYZPoint16(dxy.x, dxy.y, 0), exit_edge)) return RVD_NO_VISIT; // Ride cannot be entered here.

		/* Check whether the queue is so long that someone is queuing near the tile edge. */
		XYZPoint32 tile_edge_pix_pos(128, 128, 0);
		switch (original_exit_edge) {
			case EDGE_NE: tile_edge_pix_pos.x =   0; break;
			case EDGE_NW: tile_edge_pix_pos.y =   0; break;
			case EDGE_SW: tile_edge_pix_pos.x = 255; break;
			case EDGE_SE: tile_edge_pix_pos.y = 255; break;
			default: NOT_REACHED();
		}
		if (!this->IsQueuingGuest() && this->GetQueuingGuestNearby(original_cur_pos, tile_edge_pix_pos, false) != nullptr) {
			ri->NotifyLongQueue();
			return RVD_NO_VISIT;
		}

		if (ri == this->ride) {  // Guest decided before that this shop/ride should be visited.
			*seen_wanted_ride = true;
			return RVD_MUST_VISIT;
		}
	}

	switch (exit_edge) {
		case EDGE_NE: cur_pos.x--; break;
		case EDGE_SW: cur_pos.x++; break;
		case EDGE_NW: cur_pos.y--; break;
		case EDGE_SE: cur_pos.y++; break;
		default: NOT_REACHED();
	}
	RideVisitDesire rvd = this->WantToVisit(ri, cur_pos, exit_edge);
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
	this->UpdateZPosition();
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
					int offset = vs->GetBaseGroundOffset();
					const Voxel *v = vs->voxels[offset].get();
					if (HasValidPath(v) && GetImplodedPathSlope(v) < PATH_FLAT_COUNT &&
							(GetPathExits(v) & ((1 << EDGE_SE) | (1 << EDGE_SW))) != 0) {
						ps.AddStart(XYZPoint16(x, y, vs->base + offset));
					}
				}
			} else {
				vs = _world.GetStack(x + 1, y);
				if (vs->owner == OWN_PARK) {
					int offset = vs->GetBaseGroundOffset();
					const Voxel *v = vs->voxels[offset].get();
					if (HasValidPath(v) && GetImplodedPathSlope(v) < PATH_FLAT_COUNT &&
							(GetPathExits(v) & (1 << EDGE_NE)) != 0) {
						ps.AddStart(XYZPoint16(x + 1, y, vs->base + offset));
					}
				}

				vs = _world.GetStack(x, y + 1);
				if (vs->owner == OWN_PARK) {
					int offset = vs->GetBaseGroundOffset();
					const Voxel *v = vs->voxels[offset].get();
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
	ps.AddStart(XYZPoint16(x, y, _world.GetBaseGroundHeight(x, y)));

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
 * @fn AnimateResult Person::ActionAnimationCallback()
 * Callback when an action animation finished playing.
 * @return Result code of the visit.
 */

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
		NotifyChange(WC_BOTTOM_TOOLBAR, ALL_WINDOWS_OF_TYPE, CHG_GUEST_COUNT, 1);
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

	if (this->activity == GA_WANDER) {
		/* Consider interacting with a nearby path object. */
		const PathObjectInstance *obj = _scenery.GetPathObject(this->vox_pos);
		if (obj != nullptr) {
			if (obj->type == &PathObjectType::LITTERBIN && this->has_wrapper) {
				for (TileEdge e = EDGE_BEGIN; e != EDGE_COUNT; e++) {
					if (obj->GetExistsOnTileEdge(e) && !obj->GetDemolishedOnTileEdge(e) && obj->GetFreeBinCapacity(e) > 0) {
						SB(exits, e, 1, 1);
					}
				}
			} else if (obj->type == &PathObjectType::BENCH && this->nausea > 40) {
				for (TileEdge e = EDGE_BEGIN; e != EDGE_COUNT; e++) {
					if (obj->GetExistsOnTileEdge(e) && !obj->GetDemolishedOnTileEdge(e) &&
							(obj->GetLeftGuest(e) == PathObjectType::NO_GUEST_ON_BENCH ||
							obj->GetRightGuest(e) == PathObjectType::NO_GUEST_ON_BENCH)) {
						SB(exits, e, 1, 1);
					}
				}
			} else if (this->happiness < 40) {
				for (TileEdge e = EDGE_BEGIN; e != EDGE_COUNT; e++) {
					if (obj->GetExistsOnTileEdge(e) && !obj->GetDemolishedOnTileEdge(e)) {
						SB(exits, e, 1, 1);
					}
				}
			}
		}
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
	this->SetStatus(this->ride != nullptr ? GUI_PERSON_STATUS_HEADING_TO_RIDE : GUI_PERSON_STATUS_WANDER);
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
	this->frames = anim->frames.get();
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

	_inbox.NotifyGuestDeletion(this->id);
	this->type = PERSON_INVALID;
	this->name.clear();
}


/**
 * Test whether this person is a guest queuing for a ride.
 * @return Whether the person is a guest and queuing.
 */
bool Person::IsQueuingGuest() const
{
	return this->IsGuest() && static_cast<const Guest*>(this)->activity == GA_QUEUING;
}

/**
 * Check whether another guest who is queuing for a ride is standing close to the specified position.
 * @param vox_pos Coordinates of the voxel in the world.
 * @param pix_pos Pixel position inside the voxel.
 * @param only_in_front Whether to consider only guests standing in front of this one.
 * @return A queuing guest close by, or \c nullptr if there isn't one.
 */
const Person *Person::GetQueuingGuestNearby(const XYZPoint16& vox_pos, const XYZPoint16& pix_pos, const bool only_in_front)
{
	/*
	 * To ensure that guests on a neighbouring tile are also considered, we also need to check
	 * the next voxel in all four directions, as well as the one above and the one below that.
	 */
	const XYZPoint32 merged_pos = MergeCoordinates(vox_pos, pix_pos);
	std::vector<XYZPoint16> voxels = {vox_pos};
	for (const XYZPoint16& vx : {
			vox_pos,
			XYZPoint16(vox_pos.x + 1, vox_pos.y, vox_pos.z),
			XYZPoint16(vox_pos.x - 1, vox_pos.y, vox_pos.z),
			XYZPoint16(vox_pos.x, vox_pos.y + 1, vox_pos.z),
			XYZPoint16(vox_pos.x, vox_pos.y - 1, vox_pos.z)}) {
		if (!IsVoxelstackInsideWorld(vx.x, vx.y)) continue;

		for (const XYZPoint16& checkme : {vx, XYZPoint16(vx.x, vx.y, vx.z + 1), XYZPoint16(vx.x, vx.y, vx.z - 1)}) {
			const Voxel *voxel = _world.GetVoxel(checkme);
			if (voxel == nullptr) continue;

			for (VoxelObject *v = voxel->voxel_objects; v != nullptr; v = v->next_object) {
				if (v == this) continue;
				Guest *g = dynamic_cast<Guest*>(v);
				if (g == nullptr || !g->IsQueuingGuest()) continue;

				const XYZPoint32 coords = g->MergeCoordinates();
				if (hypot(coords.x - merged_pos.x, coords.y - merged_pos.y) < QUEUE_DISTANCE) {
					if (!only_in_front) return g;
					const AnimationFrame &frame = this->frames[this->frame_index];
					if (frame.dx > 0 && coords.x > merged_pos.x) return g;
					if (frame.dx < 0 && coords.x < merged_pos.x) return g;
					if (frame.dy > 0 && coords.y > merged_pos.y) return g;
					if (frame.dy < 0 && coords.y < merged_pos.y) return g;
				}
			}
		}
	}
	return nullptr;
}

/**
 * The person is standing in front of a path object.
 * @param obj Object to interact with.
 * @return Result code of the visit.
 */
AnimateResult Person::InteractWithPathObject([[maybe_unused]] PathObjectInstance *obj)
{
	return OAR_CONTINUE;
}

/**
 * Detect whether a group of people queuing behind each other contains a cyclic dependency.
 * @return This person has a cyclic queuing dependency.
 */
bool Person::HasCyclicQueuingDependency() const
{
	std::set<const Person *> iterated = {this};
	for (const Person *it = this->queuing_blocked_on; it != nullptr; it = it->queuing_blocked_on) {
		if (iterated.count(it) > 0) return true;
		iterated.insert(it);
	}
	return false;
}

/**
 * Update the animation of a person.
 * @param delay Amount of milliseconds since the last update.
 * @return Whether to keep the person active or how to deactivate him/her.
 * @return Result code of the visit.
 */
AnimateResult Person::OnAnimate(int delay)
{
	this->queuing_blocked_on = nullptr;
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

	const AnimationFrame *frame = &this->frames[this->frame_index];
	if (this->IsQueuingGuest()) {
		this->queuing_blocked_on = this->GetQueuingGuestNearby(this->vox_pos, this->pix_pos, true);
		if (this->queuing_blocked_on != nullptr && !this->HasCyclicQueuingDependency()) {
			/* Freeze in place if we are too close to the person queuing in front of us. */
			this->frame_time += delay;
			return OAR_OK;
		}
		/* Either there is no one in front of this person, or each person is waiting for the other one to make the first move. */
		this->queuing_blocked_on = nullptr;
	}
	this->pix_pos.x += frame->dx;
	this->pix_pos.y += frame->dy;

	bool reached = false; // Set to true when we are beyond the limit!
	if (this->walk->limit_type == WLM_INVALID) {
		if (this->frame_index + 1 >= this->frame_count) {
			reached = true;
			AnimateResult ar = this->ActionAnimationCallback();
			if (ar != OAR_CONTINUE) return ar;
		}
	} else if ((this->walk->limit_type & (1 << WLM_END_LIMIT)) == WLM_X_COND) {
		if (frame->dx > 0) reached |= this->pix_pos.x > x_limit;
		if (frame->dx < 0) reached |= this->pix_pos.x < x_limit;

		if (y_limit >= 0) this->pix_pos.y += sign(y_limit - this->pix_pos.y); // Also slowly move the other axis in the right direction.
	} else {
		if (frame->dy > 0) reached |= this->pix_pos.y > y_limit;
		if (frame->dy < 0) reached |= this->pix_pos.y < y_limit;

		if (x_limit >= 0) this->pix_pos.x += sign(x_limit - this->pix_pos.x); // Also slowly move the other axis in the right direction.
	}

	this->UpdateZPosition();
	if (!reached) {
		/* Not reached the end, do the next frame. */
		this->frame_index++;
		this->frame_index %= this->frame_count;
		this->frame_time = this->frames[this->frame_index].duration;
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
	PathObjectInstance *path_object_to_interact_with = _scenery.GetPathObject(this->vox_pos);

	const XYZPoint16 former_vox_pos = this->vox_pos;
	Voxel *former_voxel = _world.GetCreateVoxel(this->vox_pos, false);
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

	if (exit_edge == INVALID_EDGE) {
		/* Nothing actually changed. */
		this->DecideMoveDirection();
		return OAR_OK;
	}
	this->RemoveSelf(former_voxel);

	AnimateResult ar = this->EdgeOfWorldOnAnimate();
	if (ar != OAR_CONTINUE) return ar;

	if (path_object_to_interact_with != nullptr && !path_object_to_interact_with->GetExistsOnTileEdge(exit_edge)) path_object_to_interact_with = nullptr;
	/* Handle raising of z position. */
	if (this->pix_pos.z > 128) {
		dz++;
		this->vox_pos.z++;
		this->pix_pos.z = 0;
	}
	/* At bottom of the voxel. */
	Voxel *v = _world.GetCreateVoxel(this->vox_pos, false);
	if (v != nullptr || path_object_to_interact_with != nullptr) {
		bool move_on = true;
		bool freeze_animation = false;
		if (path_object_to_interact_with == nullptr) {
			SmallRideInstance instance = v->GetInstance();
			if (instance >= SRI_FULL_RIDES) {
				assert(exit_edge != INVALID_EDGE);
				RideInstance *ri = _rides_manager.GetRideInstance(instance);
				AnimateResult ar = this->VisitRideOnAnimate(ri, exit_edge);
				if (ar != OAR_CONTINUE && ar != OAR_HALT && ar != OAR_ANIMATING) return ar;
				move_on = (ar == OAR_CONTINUE);
				freeze_animation = (ar == OAR_HALT);

				/* Ride is could not be visited, fall-through to reversing movement. */

			} else if (HasValidPath(v) || this->IsLeavingPath()) {
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
				if (!HasValidPath(former_voxel)) {
					this->DecideMoveDirectionOnPathlessLand(former_voxel, former_vox_pos, exit_edge, dx, dy, dz);
					return OAR_OK;
				}
				/* Fall through to reversing movement. */
			} else {
				this->DecideMoveDirectionOnPathlessLand(former_voxel, former_vox_pos, exit_edge, dx, dy, dz);
				return OAR_OK;
			}
		}

		/* Restore the person at the previous tile (ie reverse movement). */
		if (dx != 0) { this->vox_pos.x -= dx; this->pix_pos.x = (dx > 0) ? 255 : 0; }
		if (dy != 0) { this->vox_pos.y -= dy; this->pix_pos.y = (dy > 0) ? 255 : 0; }
		if (dz != 0) { this->vox_pos.z -= dz; this->pix_pos.z = (dz > 0) ? 255 : 0; }

		this->AddSelf(_world.GetCreateVoxel(this->vox_pos, false));

		/* Check if there's a path object to interact with here. */
		if (path_object_to_interact_with != nullptr) {
			AnimateResult ar = this->InteractWithPathObject(path_object_to_interact_with);
			if (ar != OAR_CONTINUE && ar != OAR_HALT && ar != OAR_ANIMATING) return ar;
			move_on = (ar == OAR_CONTINUE);
			freeze_animation = (ar == OAR_HALT);
		}

		if (move_on) {
			this->DecideMoveDirection();
		} else if (freeze_animation) {
			/* Freeze the animation until we may continue. */
			this->frame_time += delay;
		}
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

	/* The person is walking at random over pathless land. */
	this->DecideMoveDirectionOnPathlessLand(former_voxel, former_vox_pos, exit_edge, dx, dy, dz);
	return OAR_OK;
}

/**
 * The person is walking on pathless land, decide whether the current movement is allowed and where to go next.
 * @param former_voxel The voxel from which the person comes.
 * @param former_vox_pos The coordinates from which the person comes.
 * @param exit_edge The edge over which the person left the voxel.
 * @param dx The X coordinate change between the initial and the current position.
 * @param dy The Y coordinate change between the initial and the current position.
 * @param dz The Z coordinate change between the initial and the current position.
 * @pre The person is located in (but not yet added to) the destination voxel, or somewhere below or above it.
 */
void Person::DecideMoveDirectionOnPathlessLand(Voxel *former_voxel, const XYZPoint16 &former_vox_pos,
		const TileEdge exit_edge, const int dx, const int dy, const int dz)
{
	const XYZPoint16 init_pos = this->vox_pos;
	Voxel *new_voxel = _world.GetCreateVoxel(this->vox_pos, true);
	this->AddSelf(new_voxel);
	this->UpdateZPosition();
	new_voxel = _world.GetCreateVoxel(this->vox_pos, false);

	assert(former_voxel->GetGroundType() != GTP_INVALID);
	assert(new_voxel != nullptr && new_voxel->GetGroundType() != GTP_INVALID);
	TileSlope old_voxel_slope = ExpandTileSlope(former_voxel->GetGroundSlope());
	TileSlope new_voxel_slope = ExpandTileSlope(   new_voxel->GetGroundSlope());
	/* Convert the bottom part of steep slopes to a format that is easier to handle here. */
	auto convert_slope = [](TileSlope &slope) {
		if ((slope & TSB_STEEP) == 0 || (slope & TSB_TOP) != 0) return;
		slope = (TSB_MASK_ALL ^ static_cast<TileSlope>(ROL(static_cast<uint8>(slope & ~TSB_STEEP), 4, 2))) & TSB_MASK_ALL;
	};
	convert_slope(old_voxel_slope);
	convert_slope(new_voxel_slope);

	uint16 node_heights[4];
	for (uint8 i = 0; i < 4; i++) {
		node_heights[i] = (i < 2 ? former_vox_pos : this->vox_pos).z + (((i < 2 ? old_voxel_slope : new_voxel_slope) >> ((exit_edge + i) % 4)) & 1);
	}

	if (node_heights[0] != node_heights[3] || node_heights[1] != node_heights[2]) {
		/* Climbing up or falling down a cliff? Better go back instead. */
		this->RemoveSelf(new_voxel);
		this->vox_pos = init_pos;
		if (dx != 0) { this->vox_pos.x -= dx; this->pix_pos.x = (dx > 0) ? 255 : 0; }
		if (dy != 0) { this->vox_pos.y -= dy; this->pix_pos.y = (dy > 0) ? 255 : 0; }
		if (dz != 0) { this->vox_pos.z -= dz; this->pix_pos.z = (dz > 0) ? 255 : 0; }
		this->AddSelf(_world.GetCreateVoxel(this->vox_pos, false));
	}
	this->DecideMoveDirection();
}

/**
 * @fn RideVisitDesire Person::WantToVisit(const RideInstance *ri, const XYZPoint16 &ride_pos, TileEdge exit_edge)
 * How much does the person desire to visit the given ride?
 * @param ri Ride that can be visited.
 * @param ride_pos Position of the ride.
 * @param exit_edge Edge through which the ride could be visited.
 * @return Desire of the person to visit the ride.
 */

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

/** Initialize this guest's ride preferences with random values. */
void Guest::InitRidePreferences()
{
	this->preferred_ride_intensity = this->rnd.Uniform(800) + 10;
	this->min_ride_intensity       = this->rnd.Uniform(this->preferred_ride_intensity - 5);
	this->max_ride_intensity       = this->rnd.Uniform(this->min_ride_intensity) + this->preferred_ride_intensity + 5;
	this->max_ride_nausea          = this->rnd.Uniform(this->max_ride_intensity) + this->min_ride_intensity;
	this->min_ride_excitement      = this->rnd.Uniform(this->preferred_ride_intensity);
}

void Guest::Activate(const Point16 &start, PersonType person_type)
{
	this->activity = GA_ENTER_PARK;
	this->Person::Activate(start, person_type);

	this->happiness = 50 + this->rnd.Uniform(50);
	this->total_happiness = 0;
	this->cash = 3000 + this->rnd.Uniform(4095);
	this->cash_spent = 0;

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
	this->InitRidePreferences();
}

void Guest::DeActivate(AnimateResult ar)
{
	if (this->IsActive()) {
		/* Close possible Guest Info window */
		Window *wi = GetWindowByType(WC_PERSON_INFO, this->id);
		delete wi;

		/// \todo Evaluate Guest::total_happiness against scenario requirements for evaluating the park value.
	}

	this->Person::DeActivate(ar);
}

/**
 * Load a guest from the save game.
 * @param ldr Input stream to read.
 */
void Guest::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("gues");
	if (version < 1 || version > CURRENT_VERSION_Guest) ldr.version_mismatch(version, CURRENT_VERSION_Guest);
	this->Person::Load(ldr);

	this->activity = static_cast<GuestActivity>(ldr.GetByte());
	this->happiness = ldr.GetWord();
	this->total_happiness = ldr.GetWord();
	this->cash = static_cast<Money>(ldr.GetLongLong());
	this->cash_spent = static_cast<Money>(ldr.GetLongLong());

	if (version < 3) {
		uint16 ride_index = ldr.GetWord();
		if (ride_index != INVALID_RIDE_INSTANCE) this->ride = _rides_manager.GetRideInstance(ride_index);
	}

	this->has_map = ldr.GetByte();
	this->has_umbrella = ldr.GetByte();
	this->has_wrapper = ldr.GetByte();
	this->has_balloon = ldr.GetByte();
	this->salty_food = ldr.GetByte();
	this->souvenirs = ldr.GetByte();
	this->food = ldr.GetByte();
	this->drink = ldr.GetByte();
	this->hunger_level = ldr.GetByte();
	this->thirst_level = ldr.GetByte();
	this->stomach_level = ldr.GetByte();
	this->waste = ldr.GetByte();
	this->nausea = ldr.GetByte();

	if (version > 1) {
		this->preferred_ride_intensity = ldr.GetLong();
		this->min_ride_intensity = ldr.GetLong();
		this->max_ride_intensity = ldr.GetLong();
		this->max_ride_nausea = ldr.GetLong();
		this->min_ride_excitement = ldr.GetLong();
	} else {
		this->InitRidePreferences();
	}

	if (this->activity == GA_ON_RIDE) this->RemoveSelf(_world.GetCreateVoxel(this->vox_pos, false));
	ldr.ClosePattern();
}

/**
 * Save guest data to the save game file.
 * @param svr Output stream to save to.
 */
void Guest::Save(Saver &svr)
{
	svr.StartPattern("gues", CURRENT_VERSION_Guest);
	this->Person::Save(svr);

	svr.PutByte(this->activity);
	svr.PutWord(this->happiness);
	svr.PutWord(this->total_happiness);
	svr.PutLongLong(static_cast<uint64>(this->cash));
	svr.PutLongLong(static_cast<uint64>(this->cash_spent));

	svr.PutByte(this->has_map);
	svr.PutByte(this->has_umbrella);
	svr.PutByte(this->has_wrapper);
	svr.PutByte(this->has_balloon);
	svr.PutByte(this->salty_food);
	svr.PutByte(this->souvenirs);
	svr.PutByte(this->food);
	svr.PutByte(this->drink);
	svr.PutByte(this->hunger_level);
	svr.PutByte(this->thirst_level);
	svr.PutByte(this->stomach_level);
	svr.PutByte(this->waste);
	svr.PutByte(this->nausea);

	svr.PutLong(this->preferred_ride_intensity);
	svr.PutLong(this->min_ride_intensity);
	svr.PutLong(this->max_ride_intensity);
	svr.PutLong(this->max_ride_nausea);
	svr.PutLong(this->min_ride_excitement);
	svr.EndPattern();
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

	for (uint  i = _scenery.CountLitterAndVomit (this->vox_pos); i > 0; i--) _guests.Complain(Guests::COMPLAINT_LITTER);
	for (uint8 i = _scenery.CountDemolishedItems(this->vox_pos); i > 0; i--) _guests.Complain(Guests::COMPLAINT_VANDALISM);

	return OAR_CONTINUE;
}

AnimateResult Guest::VisitRideOnAnimate(RideInstance *ri, TileEdge exit_edge)
{
	if (ri->CanBeVisited(this->vox_pos, exit_edge) && this->SelectItem(ri) != ITP_NOTHING) {
		/* All lights are green, let's try to enter the ride. */
		this->activity = GA_ON_RIDE;
		this->ride = ri;
		const RideEntryResult rer = ri->EnterRide(this->id, this->vox_pos, exit_edge);
		if (rer == RER_WAIT) {
			this->activity = GA_QUEUING;
			return OAR_HALT;
		}
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

/** Pixel positions of guests sitting on a bench. */
static XYZPoint16 _bench_pix_pos[4 /* TileEdge */][2 /* Left = 0, Right = 1 */] = {
	{XYZPoint16(  0, 160, 0), XYZPoint16(  0,  96, 0)},
	{XYZPoint16(160, 255, 0), XYZPoint16( 96, 255, 0)},
	{XYZPoint16(255,  96, 0), XYZPoint16(255, 160, 0)},
	{XYZPoint16( 96,   0, 0), XYZPoint16(160,   0, 0)},
};

AnimateResult Guest::InteractWithPathObject(PathObjectInstance *obj)
{
	const TileEdge edge = this->GetCurrentEdge();
	if (obj->GetDemolishedOnTileEdge(edge)) return OAR_CONTINUE;

	if (obj->type == &PathObjectType::LITTERBIN && this->has_wrapper && obj->GetFreeBinCapacity(edge) > 0) {
		/* Throw litter in the bin, then keep walking. */
		obj->AddItemToBin(edge);
		this->has_wrapper = false;
	} else if (obj->type == &PathObjectType::BENCH && this->nausea > 40 &&
			(obj->GetLeftGuest(edge) == PathObjectType::NO_GUEST_ON_BENCH ||
			obj->GetRightGuest(edge) == PathObjectType::NO_GUEST_ON_BENCH)) {
		/* Sit down and remain there for a while. */
		if (obj->GetRightGuest(edge) == PathObjectType::NO_GUEST_ON_BENCH) {
			obj->SetRightGuest(edge, this->id);
			this->pix_pos = _bench_pix_pos[edge][1];
		} else {
			obj->SetLeftGuest(edge, this->id);
			this->pix_pos = _bench_pix_pos[edge][0];
		}
		this->activity = GA_RESTING;
		this->StartAnimation(_guest_bench[edge]);
		return OAR_OK;
	} else if (this->happiness < 40) {
		/* Smash something up, then keep walking. */
		for (int8 dx = -2; dx <= 2; dx++) {
			for (int8 dy = -2; dy <= 2; dy++) {
				if (!IsVoxelstackInsideWorld(this->vox_pos.x + dx, this->vox_pos.y + dy)) continue;
				for (int8 dz = -1; dz <= 1; dz++) {
					const Voxel *vx = _world.GetVoxel(XYZPoint16(this->vox_pos.x + dx, this->vox_pos.y + dy, this->vox_pos.z + dz));
					if (vx == nullptr) continue;
					for (VoxelObject *o = vx->voxel_objects; o != nullptr; o = o->next_object) {
						if (dynamic_cast<Guard*>(o) != nullptr) {
							/* A security guard is nearby, so no vandalism just yet. */
							return OAR_CONTINUE;
						}
					}
				}
			}
		}

		obj->Demolish(edge);
	}
	return OAR_CONTINUE;
}

AnimateResult Guest::ActionAnimationCallback()
{
	assert(this->activity == GA_RESTING);
	const TileEdge edge = this->GetCurrentEdge();

	if (this->food > 0 || this->drink > 0 || this->rnd.Uniform(255) > this->nausea) {
		/* Remain sitting while eating, drinking, or nauseous. */
		this->StartAnimation(_guest_bench[edge]);
		return OAR_OK;
	}

	/* Get up and keep walking. */
	PathObjectInstance *obj = _scenery.GetPathObject(this->vox_pos);
	assert(obj != nullptr && obj->type == &PathObjectType::BENCH);
	if (obj->GetLeftGuest(edge) == this->id) {
		obj->SetLeftGuest(edge, PathObjectType::NO_GUEST_ON_BENCH);
	} else {
		assert(obj->GetRightGuest(edge) == this->id);
		obj->SetRightGuest(edge, PathObjectType::NO_GUEST_ON_BENCH);
	}

	this->activity = GA_WANDER;
	return OAR_CONTINUE;
}

/** Inform this guest that the bench he is sitting on has just been deleted. */
void Guest::ExpelFromBench()
{
	assert(this->activity == GA_RESTING);
	this->activity = GA_WANDER;
	this->ChangeHappiness(-10);
	this->DecideMoveDirection();
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
	NotifyChange(WC_PERSON_INFO, this->id, CHG_DISPLAY_OLD, 0);
}

/**
 * Daily ponderings of a guest.
 * @return If \c false, de-activate the guest.
 * @todo Make going home a bit more random.
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
		if (this->has_wrapper && this->activity != GA_ON_RIDE && this->rnd.Success1024(25)) {
			_scenery.AddLitter(this->vox_pos, this->pix_pos);
			this->has_wrapper = false;
		}
		if (this->hunger_level > 200) {
			happiness_change--;
			_guests.Complain(Guests::COMPLAINT_HUNGER);
		}
		if (this->thirst_level > 200) {
			happiness_change--;
			_guests.Complain(Guests::COMPLAINT_THIRST);
		}
	}
	if (this->waste > 170) {
		happiness_change -= 2;
		_guests.Complain(Guests::COMPLAINT_WASTE);
	}

	if (this->nausea > 110) {
		happiness_change -= 8;
		if (this->activity != GA_ON_RIDE && this->rnd.Success1024(4 * this->nausea)) {
			_scenery.AddVomit(this->vox_pos, this->pix_pos);
			this->nausea /= 2;
			this->stomach_level /= 2;
			happiness_change -= 20;
		}
	}

	if (this->activity == GA_ON_RIDE) {
		assert(this->ride != nullptr);
		happiness_change += this->rnd.Uniform(this->ride->excitement_rating) / 100;
		this->nausea = std::min(255, this->rnd.Uniform(this->ride->nausea_rating * this->ride->intensity_rating) / 10000 + this->nausea);
	} else if (this->activity == GA_RESTING) {
		happiness_change += 2;
		if (this->nausea > 20) this->nausea -= 3;
	}

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

	if (this->activity == GA_WANDER && this->happiness <= 10) {
		this->activity = GA_GO_HOME; // Go home when bored.
		NotifyChange(WC_BOTTOM_TOOLBAR, ALL_WINDOWS_OF_TYPE, CHG_GUEST_COUNT, 0);
	}
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
		case ITP_RIDE:
			return RVD_MAY_VISIT;

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

RideVisitDesire Guest::WantToVisit(const RideInstance *ri, [[maybe_unused]] const XYZPoint16 &ride_pos, [[maybe_unused]] TileEdge exit_edge)
{
	for (int i = 0; i < NUMBER_ITEM_TYPES_SOLD; i++) {
		if (ri->GetSaleItemPrice(i) > this->cash) continue;
		RideVisitDesire rvd = this->NeedForItem(ri->GetSaleItemType(i), true);
		if (rvd != RVD_NO_VISIT) return rvd;
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
		case ITP_RIDE:
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
				this->cash_spent += ri->GetSaleItemPrice(i);
				this->cash -= ri->GetSaleItemPrice(i);
				this->AddItem(ri->GetSaleItemType(i));
				this->ChangeHappiness(10);
			}
		}
	}
	this->ChangeHappiness(-10);
}

/* Constructor. */
StaffMember::StaffMember()
{
	/* Nothing to do currently. */
}

/* Destructor. */
StaffMember::~StaffMember()
{
	/* Nothing to do currently. */
}

void StaffMember::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("stfm");
	if (version < 1 || version > CURRENT_VERSION_StaffMember) ldr.version_mismatch(version, CURRENT_VERSION_StaffMember);
	this->Person::Load(ldr);
	if (version < 2) this->status = GUI_PERSON_STATUS_WANDER + ldr.GetWord();
	ldr.ClosePattern();
}

void StaffMember::Save(Saver &svr)
{
	svr.StartPattern("stfm", CURRENT_VERSION_StaffMember);
	this->Person::Save(svr);
	svr.EndPattern();
}

/**
 * Create this person's current status string.
 * @return The status string.
 */
std::string Person::GetStatus() const
{
	const std::string text = _language.GetText(this->status);
	if (this->ride == nullptr) return text;

	static char text_buffer[1024];
	snprintf(text_buffer, lengthof(text_buffer), text.c_str(), this->ride->name.c_str());
	return text_buffer;
}

/**
 * Change this person's current status.
 * @param s New status.
 */
void Person::SetStatus(StringID s)
{
	this->status = s;
	NotifyChange(WC_PERSON_INFO, this->id, CHG_DISPLAY_OLD, 0);
}

bool StaffMember::DailyUpdate()
{
	/* Nothing to do currently. */
	return true;
}

RideVisitDesire StaffMember::WantToVisit([[maybe_unused]] const RideInstance *ri, [[maybe_unused]] const XYZPoint16 &ride_pos, [[maybe_unused]] TileEdge exit_edge)
{
	return RVD_NO_VISIT;
}

AnimateResult StaffMember::VisitRideOnAnimate([[maybe_unused]] RideInstance *ri, [[maybe_unused]] const TileEdge exit_edge)
{
	return OAR_CONTINUE;
}

AnimateResult StaffMember::EdgeOfWorldOnAnimate()
{
	return OAR_CONTINUE;
}

void StaffMember::DecideMoveDirection()
{
	/* \todo Lots of shared code with Guest::DecideMoveDirection and Guest::GetExitDirections. */
	this->SetStatus(this->ride != nullptr ? GUI_PERSON_STATUS_HEADING_TO_RIDE : GUI_PERSON_STATUS_WANDER);

	const VoxelStack *vs = _world.GetStack(this->vox_pos.x, this->vox_pos.y);
	const Voxel *v = vs->Get(this->vox_pos.z);
	const bool is_on_path = HasValidPath(v);
	const TileEdge start_edge = this->GetCurrentEdge();

	uint8 exits = (is_on_path ? GetPathExits(v) : 0xF);
	uint8 bot_exits = exits & 0x0F; // Exits at the bottom of the voxel.
	uint8 top_exits = (exits >> 4) & 0x0F; // Exits at the top of the voxel.
	uint8 found_ride = 0;

	for (TileEdge exit_edge = EDGE_BEGIN; exit_edge != EDGE_COUNT; exit_edge++) {
		int extra_z;  // Decide z position of the exit.
		if (GB(bot_exits, exit_edge, 1) != 0) {
			extra_z = 0;
		} else if (GB(top_exits, exit_edge, 1) != 0) {
			extra_z = 1;
		} else {
			continue;
		}

		bool b;
		RideVisitDesire rvd = ComputeExitDesire(start_edge, this->vox_pos + XYZPoint16(0, 0, extra_z), exit_edge, &b);
		switch (rvd) {
			case RVD_NO_RIDE:
				break;

			case RVD_NO_VISIT:
				if (is_on_path) {
					SB(bot_exits, exit_edge, 1, 0);
					SB(top_exits, exit_edge, 1, 0);
				}
				break;

			case RVD_MUST_VISIT:
				SB(found_ride, exit_edge, 1, 1);
				break;

			default: NOT_REACHED();
		}
	}
	bot_exits |= top_exits;
	if (found_ride != 0) bot_exits &= found_ride;

	exits = (found_ride << 4) | bot_exits;
	exits &= this->GetInparkDirections();  // Don't leave the park.

	/* Decide which direction to go. */
	SB(exits, start_edge, 1, 0); // Drop 'return' option until we find there are no other directions.
	uint8 walk_count = 0;
	for (TileEdge exit_edge = EDGE_BEGIN; exit_edge != EDGE_COUNT; exit_edge++) {
		if (GB(exits, exit_edge, 1) == 0) continue;
		walk_count++;
	}
	if (walk_count == 0) SB(exits, start_edge, 1, 1);  // No exits: Add 'return' as option.

	const WalkInformation *walks[4]; // Walks that can be done at this tile.
	walk_count = 0;
	for (TileEdge exit_edge = EDGE_BEGIN; exit_edge != EDGE_COUNT; exit_edge++) {
		if (GB(exits, exit_edge, 1) != 0) {
			walks[walk_count++] = _walk_path_tile[start_edge][exit_edge];
		}
	}

	const WalkInformation *new_walk;
	if (walk_count == 1) {
		new_walk = walks[0];
	} else {
		new_walk = walks[this->rnd.Uniform(walk_count - 1)];
	}
	this->StartAnimation(new_walk);
}

/* Constructor. */
Mechanic::Mechanic()
{
	/* Nothing to do currently. */
}

/* Destructor. */
Mechanic::~Mechanic()
{
	if (this->ride != nullptr) _staff.RequestMechanic(this->ride);
}

void Mechanic::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("mchc");
	if (version < 1 || version > CURRENT_VERSION_Mechanic) ldr.version_mismatch(version, CURRENT_VERSION_Mechanic);

	if (version == 1) {
		this->Person::Load(ldr);
		const uint16 ride_index = ldr.GetWord();
		if (ride_index != INVALID_RIDE_INSTANCE) this->ride = _rides_manager.GetRideInstance(ride_index);
	} else {
		this->StaffMember::Load(ldr);
	}

	ldr.ClosePattern();
}

void Mechanic::Save(Saver &svr)
{
	svr.StartPattern("mchc", CURRENT_VERSION_Mechanic);
	this->StaffMember::Save(svr);
	/* No specific data to save currently. */
	svr.EndPattern();
}

/**
 * Order this mechanic to inspect a ride.
 * @param ri Ride to inspect.
 */
void Mechanic::Assign(RideInstance *ri)
{
	assert(this->ride == nullptr);
	this->ride = ri;
	this->SetStatus(GUI_PERSON_STATUS_HEADING_TO_RIDE);
}

/**
 * Notify the mechanic of removal of a ride.
 * @param ri Ride being deleted.
 */
void Mechanic::NotifyRideDeletion(const RideInstance *ri)
{
	if (ri == this->ride) this->ride = nullptr;
}

void Mechanic::DecideMoveDirection()
{
	if (this->ride == nullptr) return StaffMember::DecideMoveDirection();

	EdgeCoordinate destination = this->ride->GetMechanicEntrance();
	destination.coords.x += _tile_dxy[destination.edge].x;
	destination.coords.y += _tile_dxy[destination.edge].y;

	PathSearcher ps(this->vox_pos);
	ps.AddStart(destination.coords);
	destination.coords.z--;
	ps.AddStart(destination.coords);  // In case the path leading to the mechanic entrance is sloping upwards.

	if (!ps.Search()) {
		/* Could not find a path from our position to the destination ride, probably because no such path exists. */
		_staff.RequestMechanic(this->ride);
		this->ride = nullptr;
		return StaffMember::DecideMoveDirection();
	}

	this->SetStatus(GUI_PERSON_STATUS_HEADING_TO_RIDE);
	const WalkedPosition *dest = ps.dest_pos;
	const WalkedPosition *prev = dest->prev_pos;

	if (prev == nullptr) {
		/* Already at destination. Entering the ride is handled by the parent class method. */
		return StaffMember::DecideMoveDirection();
	}

	const TileEdge edge = GetAdjacentEdge(dest->cur_vox.x, dest->cur_vox.y, prev->cur_vox.x, prev->cur_vox.y);
	this->StartAnimation(_walk_path_tile[this->GetCurrentEdge()][edge]);
}

RideVisitDesire Mechanic::WantToVisit(const RideInstance *ri, const XYZPoint16 &ride_pos, TileEdge exit_edge)
{
	if (ri != this->ride) return RVD_NO_VISIT;  // Not our destination ride.

	const EdgeCoordinate destination = this->ride->GetMechanicEntrance();
	if (destination.coords                  != ride_pos                          ) return RVD_NO_VISIT;  // Wrong location.
	if (static_cast<int>(exit_edge + 2) % 4 != static_cast<int>(destination.edge)) return RVD_NO_VISIT;  // Wrong direction.

	return RVD_MUST_VISIT;  // All checks passed, we may enter the ride here.
}

AnimateResult Mechanic::VisitRideOnAnimate(RideInstance *ri, const TileEdge exit_edge)
{
	if (!this->WantToVisit(ri, this->vox_pos, exit_edge)) {
		/* Not our destination ride, or approaching at the wrong place. */
		return OAR_CONTINUE;
	}

	this->SetStatus(ri->broken ? GUI_PERSON_STATUS_REPAIRING : GUI_PERSON_STATUS_INSPECTING);
	this->StartAnimation(_mechanic_repair[exit_edge]);
	return OAR_ANIMATING;
}

AnimateResult Mechanic::ActionAnimationCallback()
{
	if (this->ride == nullptr) return OAR_CONTINUE;  // The ride was deleted while we were inspecting it.

	this->ride->MechanicArrived();
	this->ride = nullptr;

	return OAR_CONTINUE;
}

/* Constructor. */
Guard::Guard()
{
	/* Nothing to do currently. */
}

/* Destructor. */
Guard::~Guard()
{
	/* Nothing to do currently. */
}

void Guard::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("gard");
	if (version < 1 || version > CURRENT_VERSION_Guard) ldr.version_mismatch(version, CURRENT_VERSION_Guard);
	this->StaffMember::Load(ldr);
	/* No specific data to load currently. */
	ldr.ClosePattern();
}

void Guard::Save(Saver &svr)
{
	svr.StartPattern("gard", CURRENT_VERSION_Guard);
	this->StaffMember::Save(svr);
	/* No specific data to save currently. */
	svr.EndPattern();
}

/* Constructor. */
Entertainer::Entertainer()
{
	/* Nothing to do currently. */
}

/* Destructor. */
Entertainer::~Entertainer()
{
	/* Nothing to do currently. */
}

void Entertainer::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("etai");
	if (version < 1 || version > CURRENT_VERSION_Entertainer) ldr.version_mismatch(version, CURRENT_VERSION_Entertainer);
	this->StaffMember::Load(ldr);
	/* No specific data to load currently. */
	ldr.ClosePattern();
}

void Entertainer::Save(Saver &svr)
{
	svr.StartPattern("etai", CURRENT_VERSION_Entertainer);
	this->StaffMember::Save(svr);
	/* No specific data to save currently. */
	svr.EndPattern();
}

/* Constructor. */
Handyman::Handyman()
{
	this->activity = HandymanActivity::WANDER;
}

/* Destructor. */
Handyman::~Handyman()
{
	/* Nothing to do currently. */
}

void Handyman::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("hndy");
	if (version < 1 || version > CURRENT_VERSION_Handyman) ldr.version_mismatch(version, CURRENT_VERSION_Handyman);
	this->StaffMember::Load(ldr);
	this->activity = static_cast<HandymanActivity>(ldr.GetByte());
	ldr.ClosePattern();
}

void Handyman::Save(Saver &svr)
{
	svr.StartPattern("hndy", CURRENT_VERSION_Handyman);
	this->StaffMember::Save(svr);
	svr.PutByte(static_cast<uint8>(this->activity));
	svr.EndPattern();
}

bool Handyman::IsLeavingPath() const
{
	switch (this->activity) {
		case HandymanActivity::HEADING_TO_WATERING:
		case HandymanActivity::LOOKING_FOR_PATH:
			return true;
		default:
			return StaffMember::IsLeavingPath();
	}
}

void Handyman::DecideMoveDirection()
{
	const TileEdge start_edge = this->GetCurrentEdge();

	if (this->activity == HandymanActivity::HEADING_TO_WATERING) {
		/* The handyman previously decided to water flowers at the current location. */
		this->activity = HandymanActivity::WATER;
		this->StartAnimation(_handyman_water[(start_edge + 2) % 4]);
		return;
	}

	const Voxel *vx = _world.GetVoxel(this->vox_pos);
	const bool is_on_path = HasValidPath(vx);
	if (is_on_path && _scenery.CountLitterAndVomit(this->vox_pos) > 0) {
		bool found_other_handyman = false;
		for (VoxelObject *o = vx->voxel_objects; o != nullptr; o = o->next_object) {
			if (Handyman *h = dynamic_cast<Handyman*>(o)) {
				if (h->activity == HandymanActivity::SWEEP) {
					found_other_handyman = true;
					break;
				}
			}
		}
		if (!found_other_handyman) {
			this->SetStatus(GUI_PERSON_STATUS_SWEEPING);
			this->activity = HandymanActivity::SWEEP;
			this->StartAnimation(_handyman_sweep[(start_edge + 2) % 4]);
			return;
		}
	}

	/* Consider emptying a bin if an overflowing one is nearby. */
	std::set<TileEdge> possible_edges;
	uint8 nr_possible_edges = 0;
	const PathObjectInstance *obj = _scenery.GetPathObject(this->vox_pos);
	if (obj != nullptr) {
		if (obj->type == &PathObjectType::LITTERBIN) {
			for (TileEdge e = EDGE_BEGIN; e != EDGE_COUNT; e++) {
				if (obj->GetExistsOnTileEdge(e) && !obj->GetDemolishedOnTileEdge(e) && obj->BinNeedsEmptying(e)) {
					bool found_other_handyman = false;
					for (VoxelObject *o = vx->voxel_objects; o != nullptr; o = o->next_object) {
						if (Handyman *h = dynamic_cast<Handyman*>(o)) {
							if (h->activity == static_cast<Handyman::HandymanActivity>(static_cast<int>(HandymanActivity::EMPTY_NE) + e)) {
								found_other_handyman = true;
								break;
							}
						}
					}
					if (!found_other_handyman) {
						possible_edges.insert(e);
						nr_possible_edges++;
					}
				}
			}
		}
	}
	if (nr_possible_edges > 0) {
		auto it = possible_edges.begin();
		if (nr_possible_edges > 1) std::advance(it, rnd.Uniform(nr_possible_edges - 1));
		this->StartAnimation(_center_path_tile[start_edge][*it]);
		return;
	}

	/* Check if a flowerbed in need of watering is nearby. */
	for (TileEdge edge = EDGE_BEGIN; edge != EDGE_COUNT; edge++) {
		XYZPoint16 pos = this->vox_pos;
		pos.x += _tile_dxy[edge].x;
		pos.y += _tile_dxy[edge].y;
		if (!IsVoxelstackInsideWorld(pos.x, pos.y)) continue;

		const Voxel *voxel = _world.GetVoxel(pos);
		if (voxel == nullptr) continue;

		if (voxel->instance != SRI_SCENERY || voxel->instance_data == INVALID_VOXEL_DATA) continue;  // No flowers here.
		if (_world.GetTileOwner(pos.x, pos.y) != OWN_PARK) continue;                                 // Not our responsibility.

		const SceneryType *type = _scenery.GetType(voxel->instance_data);
		if (type->watering_interval <= 0) continue;  // Some item that never needs watering.

		SceneryInstance *item = _scenery.GetItem(pos);
		if (item->ShouldBeWatered()) {
			bool found_other_handyman = false;
			for (VoxelObject *o = voxel->voxel_objects; o != nullptr; o = o->next_object) {
				if (Handyman *h = dynamic_cast<Handyman*>(o)) {
					if (h->activity == HandymanActivity::WATER) {
						found_other_handyman = true;
						break;
					}
				}
			}
			if (!found_other_handyman) {
				possible_edges.insert(edge);
				nr_possible_edges++;
			}
		}
	}
	if (nr_possible_edges > 0) {
		auto it = possible_edges.begin();
		if (nr_possible_edges > 1) std::advance(it, rnd.Uniform(nr_possible_edges - 1));

		this->activity = HandymanActivity::HEADING_TO_WATERING;
		this->SetStatus(GUI_PERSON_STATUS_WATERING);
		this->StartAnimation(_center_path_tile[start_edge][*it]);
		return;
	}

	return StaffMember::DecideMoveDirection();
}

AnimateResult Handyman::InteractWithPathObject(PathObjectInstance *obj)
{
	const TileEdge edge = this->GetCurrentEdge();
	if (obj->type == &PathObjectType::LITTERBIN && !obj->GetDemolishedOnTileEdge(edge) && obj->BinNeedsEmptying(edge)) {
		this->activity = static_cast<Handyman::HandymanActivity>(static_cast<int>(HandymanActivity::EMPTY_NE) + edge);
		this->SetStatus(GUI_PERSON_STATUS_EMPTYING);
		this->StartAnimation(_handyman_empty[edge]);
		return OAR_OK;
	}
	return OAR_CONTINUE;
}

AnimateResult Handyman::ActionAnimationCallback()
{
	switch (this->activity) {
		case HandymanActivity::WATER: {
			SceneryInstance *item = _scenery.GetItem(this->vox_pos);
			if (item != nullptr) item->time_since_watered = 0;
			break;
		}

		case HandymanActivity::SWEEP:
			_scenery.RemoveLitterAndVomit(this->vox_pos);
			break;

		case HandymanActivity::EMPTY_NE:
		case HandymanActivity::EMPTY_SE:
		case HandymanActivity::EMPTY_SW:
		case HandymanActivity::EMPTY_NW: {
			const TileEdge edge = this->GetCurrentEdge();
			PathObjectInstance *obj = _scenery.GetPathObject(this->vox_pos);
			if (obj != nullptr && obj->type == &PathObjectType::LITTERBIN &&
					obj->GetExistsOnTileEdge(edge) && !obj->GetDemolishedOnTileEdge(edge)) {
				obj->EmptyBin(edge);
			}
			break;
		}

		default: NOT_REACHED();
	}
	this->activity = HandymanActivity::WANDER;
	return OAR_CONTINUE;
}

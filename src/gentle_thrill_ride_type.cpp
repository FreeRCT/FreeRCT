/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gentle_thrill_type.cpp Implementation of gentle rides and thrill rides. */

#include "stdafx.h"
#include "map.h"
#include "shop_type.h"
#include "person.h"
#include "people.h"
#include "fileio.h"
#include "gentle_thrill_ride_type.h"
#include "generated/gentle_thrill_rides_strings.cpp"

GentleThrillRideType::GentleThrillRideType() : FixedRideType(RTK_GENTLE /* Kind will be set later in Load(). */)
{
	capacity.number_of_batches = 0;
	capacity.guests_per_batch = 0;
}

GentleThrillRideType::~GentleThrillRideType()
{
	/* Images and texts are handled by the sprite collector, no need to release its memory here. */
}

RideInstance *GentleThrillRideType::CreateInstance() const
{
	return new GentleThrillRideInstance(this);
}

/**
 * Load a type of gentle or thrill ride from the RCD file.
 * @param rcd_file Rcd file being loaded.
 * @param sprites Already loaded sprites.
 * @param texts Already loaded texts.
 * @todo #GentleThrillRideType::Load and #ShopType::Load share a lot of similar code. Pull it out into a common function in #FixedRideType.
 */
void GentleThrillRideType::Load(RcdFileReader *rcd_file, const ImageMap &sprites, const TextMap &texts)
{
	rcd_file->CheckVersion(5);
	int length = rcd_file->size;
	if (length < 3) rcd_file->Error("Length too short for header");

	this->kind = rcd_file->GetUInt8() ? RTK_THRILL : RTK_GENTLE;
	this->width_x = rcd_file->GetUInt8();
	this->width_y = rcd_file->GetUInt8();
	if (this->width_x < 1 || this->width_y < 1) rcd_file->Error("Dimension is zero");
	length -= 111 + (this->width_x * this->width_y);
	if (length <= 0) rcd_file->Error("Length too short for extended header");

	this->heights.reset(new int8[this->width_x * this->width_y]);
	for (int8 x = 0; x < this->width_x; ++x) {
		for (int8 y = 0; y < this->width_y; ++y) {
			this->heights[x * this->width_y + y] = rcd_file->GetUInt8();
		}
	}

	animation_idle = _sprite_manager.GetFrameSet(ImageSetKey(rcd_file->filename, rcd_file->GetUInt32()));
	animation_starting = _sprite_manager.GetTimedAnimation(ImageSetKey(rcd_file->filename, rcd_file->GetUInt32()));
	animation_working = _sprite_manager.GetTimedAnimation(ImageSetKey(rcd_file->filename, rcd_file->GetUInt32()));
	animation_stopping = _sprite_manager.GetTimedAnimation(ImageSetKey(rcd_file->filename, rcd_file->GetUInt32()));
	for (int i = 0; i < 4; i++) {
		ImageData *view;
		LoadSpriteFromFile(rcd_file, sprites, &view);
		this->previews[i] = view;
	}

	for (uint i = 0; i < 3; i++) {
		uint32 recolour = rcd_file->GetUInt32();
		this->recolours.Set(i, RecolourEntry(recolour));
	}
	this->item_type[0] = ITP_RIDE;
	this->item_cost[0] = rcd_file->GetInt32(); // Entrance fee.
	this->item_cost[1] = 0;                    // Unused.
	this->monthly_cost = rcd_file->GetInt32();
	this->monthly_open_cost = rcd_file->GetInt32();
	this->capacity.number_of_batches = rcd_file->GetUInt32();
	this->capacity.guests_per_batch = rcd_file->GetUInt32();
	this->default_idle_duration = rcd_file->GetUInt32();
	this->working_duration = rcd_file->GetUInt32();

	/* Check that all animations fit to the ride. */
	if (animation_idle == nullptr || animation_starting == nullptr ||
			animation_working == nullptr || animation_stopping == nullptr ||
			animation_idle->width_x != this->width_x || animation_idle->width_y != this->width_y) {
		rcd_file->Error("Idle animation does not fit");
	}
	int working_animation_min_length = 0;
	for (int i = 0; i < animation_starting->frames; ++i) {
		if (animation_starting->views[i]->width_x != this->width_x || animation_starting->views[i]->width_y != this->width_y) {
			rcd_file->Error("Starting animation does not fit");
		}
		working_animation_min_length += animation_starting->durations[i];
	}
	for (int i = 0; i < animation_working->frames; ++i) {
		if (animation_working->views[i]->width_x != this->width_x || animation_working->views[i]->width_y != this->width_y) {
			rcd_file->Error("Working animation does not fit");
		}
		working_animation_min_length += animation_working->durations[i];
	}
	for (int i = 0; i < animation_stopping->frames; ++i) {
		if (animation_stopping->views[i]->width_x != this->width_x || animation_stopping->views[i]->width_y != this->width_y) {
			rcd_file->Error("Stopping animation does not fit");
		}
		working_animation_min_length += animation_stopping->durations[i];
	}
	if (working_animation_min_length > this->working_duration) rcd_file->Error("Too long working animation");
	if (capacity.number_of_batches < 1 || capacity.guests_per_batch < 1) rcd_file->Error("Too low guest capacity");
	if (capacity.number_of_batches > 1 && working_animation_min_length != 0) {
		rcd_file->Error("Fixed rides with multiple guest batches can not have a working animation");
	}

	this->working_cycles_min = rcd_file->GetUInt16();
	this->working_cycles_max = rcd_file->GetUInt16();
	this->working_cycles_default = rcd_file->GetUInt16();
	this->reliability_max = rcd_file->GetUInt16();
	this->reliability_decrease_daily = rcd_file->GetUInt16();
	this->reliability_decrease_monthly = rcd_file->GetUInt16();
	this->intensity_base = rcd_file->GetUInt32();
	this->nausea_base = rcd_file->GetUInt32();
	this->excitement_base = rcd_file->GetUInt32();
	this->excitement_increase_cycle = rcd_file->GetUInt32();
	this->excitement_increase_scenery = rcd_file->GetUInt32();

	if (this->working_cycles_min < 1) rcd_file->Error("Zero working cycles");
	if (this->working_cycles_max < this->working_cycles_min) rcd_file->Error("Impossible working cycle limits");
	if (this->working_cycles_default < this->working_cycles_min) rcd_file->Error("Too few default working cycles");
	if (this->working_cycles_default > this->working_cycles_max) rcd_file->Error("Too many default working cycles");
	if (this->reliability_max < 0 || this->reliability_max > RELIABILITY_RANGE) rcd_file->Error("Reliability out of range");
	if (this->reliability_decrease_daily < 0 || this->reliability_decrease_daily > RELIABILITY_RANGE) rcd_file->Error("Daily reliability decrease out of range");
	if (this->reliability_decrease_monthly < 0 || this->reliability_decrease_monthly > RELIABILITY_RANGE) rcd_file->Error("Monthly reliability decrease out of range");

	TextData *text_data;
	LoadTextFromFile(rcd_file, texts, &text_data);
	StringID base = _language.RegisterStrings(*text_data, _gentle_thrill_rides_strings_table);
	this->SetupStrings(text_data, base,
			STR_GENERIC_GENTLE_THRILL_RIDES_START, GENTLE_THRILL_RIDES_STRING_TABLE_END,
			GENTLE_THRILL_RIDES_NAME_TYPE, GENTLE_THRILL_RIDES_DESCRIPTION_TYPE);

	this->internal_name = rcd_file->GetText();
	if (length != static_cast<int>(this->internal_name.size() + 1)) rcd_file->Error("Trailing bytes at end of block");
}

FixedRideType::RideCapacity GentleThrillRideType::GetRideCapacity() const
{
	return capacity;
}

const StringID *GentleThrillRideType::GetInstanceNames() const
{
	static const StringID names[] = {GENTLE_THRILL_RIDES_NAME_INSTANCE1, GENTLE_THRILL_RIDES_NAME_INSTANCE2, STR_INVALID};
	return names;
}

/**
 * Constructor of a gentle or thrill ride.
 * @param type Kind of ride.
 */
GentleThrillRideInstance::GentleThrillRideInstance(const GentleThrillRideType *type) : FixedRideInstance(type),
	entrance_pos(XYZPoint16::invalid()),
	exit_pos(XYZPoint16::invalid()),
	temp_entrance_pos(XYZPoint16::invalid()),
	temp_exit_pos(XYZPoint16::invalid())
{
	this->working_cycles = type->working_cycles_default;
}

GentleThrillRideInstance::~GentleThrillRideInstance()
{
	/* Nothing to do currently. */
}

/**
 * Get the ride type of the ride.
 * @return The gentle or thrill ride type of the ride.
 */
const GentleThrillRideType *GentleThrillRideInstance::GetGentleThrillRideType() const
{
	assert(this->type->kind == RTK_GENTLE || this->type->kind == RTK_THRILL);
	return static_cast<const GentleThrillRideType *>(this->type);
}

void GentleThrillRideInstance::InitializeItemPricesAndStatistics()
{
	FixedRideInstance::InitializeItemPricesAndStatistics();
	for (int i = 0; i < NUMBER_ITEM_TYPES_SOLD; i++) {
		this->item_price[i] = GetRideType()->item_cost[i];
	}
}

const Recolouring *GentleThrillRideInstance::GetRecolours(const XYZPoint16 &pos) const
{
	if (pos == entrance_pos || pos == temp_entrance_pos) return &this->entrance_recolours;
	if (pos == exit_pos || pos == temp_exit_pos) return &this->exit_recolours;
	return FixedRideInstance::GetRecolours(pos);
}

uint8 GentleThrillRideInstance::GetEntranceDirections(const XYZPoint16 &vox) const
{
	return (vox == this->entrance_pos || vox == this->exit_pos) ? 1 << EntranceExitRotation(vox) : SHF_ENTRANCE_NONE;
}

RideEntryResult GentleThrillRideInstance::EnterRide(int guest, [[maybe_unused]] const XYZPoint16 &vox, TileEdge entry)
{
	assert(vox == this->entrance_pos);
	if (_guests.GetExisting(guest)->cash < GetSaleItemPrice(0)) return RER_REFUSED;
	const int b = onride_guests.GetLoadingBatch();
	return (b >= 0 && this->onride_guests.batches[b].AddGuest(guest, entry)) ? RER_ENTERED : RER_WAIT;
}

EdgeCoordinate GentleThrillRideInstance::GetMechanicEntrance() const
{
	return EdgeCoordinate {this->exit_pos, static_cast<TileEdge>(this->EntranceExitRotation(this->exit_pos))};
}

XYZPoint32 GentleThrillRideInstance::GetExit([[maybe_unused]] int guest, [[maybe_unused]] TileEdge entry_edge)
{
	const int direction = this->EntranceExitRotation(this->exit_pos);
	XYZPoint32 p(this->exit_pos.x * 256, this->exit_pos.y * 256, this->vox_pos.z * 256);
	Random r;  // Don't put all guests on exactly the same spot.
	const int d = 128 + r.Uniform(128) - 64;
	switch (direction) {
		case VOR_WEST:  p.x += d;       p.y -= 32;      break;
		case VOR_EAST:  p.x += d;       p.y += 256+32;  break;
		case VOR_NORTH: p.x -= 32;      p.y += d;       break;
		case VOR_SOUTH: p.x += 256+32;  p.y += d;       break;
		default: NOT_REACHED();
	}
	return p;
}

bool GentleThrillRideInstance::IsEntranceLocation(const XYZPoint16& pos) const
{
	return pos == this->entrance_pos || pos == this->temp_entrance_pos;
}

bool GentleThrillRideInstance::IsExitLocation(const XYZPoint16& pos) const
{
	return pos == this->exit_pos || pos == this->temp_exit_pos;
}

bool GentleThrillRideInstance::CanOpenRide() const
{
	return this->entrance_pos != XYZPoint16::invalid() && this->exit_pos != XYZPoint16::invalid() && FixedRideInstance::CanOpenRide();
}

/**
 * Checks whether the ride's entrance or exit could be moved to the given location.
 * @param pos Absolute voxel in the world.
 * @param entrance Whether we're placing an entrance or exit.
 * @return The entrance or exit can be placed there.
 */
bool GentleThrillRideInstance::CanPlaceEntranceOrExit(const XYZPoint16& pos, const bool entrance) const
{
	if (pos.z != this->vox_pos.z || !IsVoxelstackInsideWorld(pos.x, pos.y) || _world.GetTileOwner(pos.x, pos.y) != OWN_PARK) return false;

	/* Is the position directly adjacent to the ride? */
	const GentleThrillRideType *t = this->GetGentleThrillRideType();
	const XYZPoint16 corner = this->vox_pos + OrientatedOffset(this->orientation, t->width_x - 1, t->width_y - 1);
	const int nw_line_y = std::min(this->vox_pos.y, corner.y) - 1;
	const int se_line_y = std::max(this->vox_pos.y, corner.y) + 1;
	const int ne_line_x = std::min(this->vox_pos.x, corner.x) - 1;
	const int sw_line_x = std::max(this->vox_pos.x, corner.x) + 1;
	if (pos.y == nw_line_y || pos.y == se_line_y) {
		if (pos.x <= ne_line_x || pos.x >= sw_line_x) return false;
	} else if (pos.x == ne_line_x || pos.x == sw_line_x) {
		if (pos.y <= nw_line_y || pos.y >= se_line_y) return false;
	} else {
		return false;
	}

	/* Is there enough vertical space available? */
	const int8 height = entrance ? RideEntranceExitType::entrance_height : RideEntranceExitType::exit_height;
	for (int16 h = 0; h < height; ++h) {
		Voxel *v = _world.GetCreateVoxel(pos + XYZPoint16(0, 0, h), false);
		if (v == nullptr) continue;

		if (h > 0 && v->GetGroundType() != GTP_INVALID) return false;
		if (!v->CanPlaceInstance() || v->GetGroundSlope() != SL_FLAT) return false;
	}
	return true;
}

/**
 * Move the ride's entrance to the given location.
 * @param pos Absolute voxel in the world.
 */
void GentleThrillRideInstance::SetEntrancePos(const XYZPoint16& pos)
{
	const int8 height = RideEntranceExitType::entrance_height;
	const SmallRideInstance index = static_cast<SmallRideInstance>(this->GetIndex());
	if (this->entrance_pos != XYZPoint16::invalid()) {
		for (int16 h = 0; h < height; ++h) {
			Voxel *voxel = _world.GetCreateVoxel(this->entrance_pos + XYZPoint16(0, 0, h), false);
			if (voxel != nullptr && voxel->instance != SRI_FREE) {
				assert(voxel->instance == index);
				voxel->ClearInstances();
			}
		}
		AddRemovePathEdges(this->entrance_pos, PATH_EMPTY, EDGE_ALL, PAS_UNUSED);
	}

	this->entrance_pos = pos;
	if (this->entrance_pos != XYZPoint16::invalid()) {
		int edges = 0;
		for (int16 h = 0; h < height; ++h) {
			const XYZPoint16 p = this->entrance_pos + XYZPoint16(0, 0, h);
			Voxel *voxel = _world.GetCreateVoxel(p, true);
			assert(voxel != nullptr && voxel->instance == SRI_FREE);
			voxel->SetInstance(index);
			if (h == 0) {
				edges = GetEntranceDirections(p);
				voxel->SetInstanceData(edges);
			} else {
				voxel->SetInstanceData(SHF_ENTRANCE_NONE);
			}
		}
		AddRemovePathEdges(this->entrance_pos, PATH_EMPTY, edges, PAS_QUEUE_PATH);
	}
}

/**
 * Move the ride's exit to the given location.
 * @param pos Absolute voxel in the world.
 */
void GentleThrillRideInstance::SetExitPos(const XYZPoint16& pos)
{
	const int8 height = RideEntranceExitType::exit_height;
	const SmallRideInstance index = static_cast<SmallRideInstance>(this->GetIndex());
	if (this->exit_pos != XYZPoint16::invalid()) {
		for (int16 h = 0; h < height; ++h) {
			Voxel *voxel = _world.GetCreateVoxel(this->exit_pos + XYZPoint16(0, 0, h), false);
			if (voxel != nullptr && voxel->instance != SRI_FREE) {
				assert(voxel->instance == index);
				voxel->ClearInstances();
			}
		}
		AddRemovePathEdges(this->exit_pos, PATH_EMPTY, EDGE_ALL, PAS_UNUSED);
	}

	this->exit_pos = pos;
		if (this->exit_pos != XYZPoint16::invalid()) {
		int edges = 0;
		for (int16 h = 0; h < height; ++h) {
			const XYZPoint16 p = this->exit_pos + XYZPoint16(0, 0, h);
			Voxel *voxel = _world.GetCreateVoxel(p, true);
			assert(voxel != nullptr && voxel->instance == SRI_FREE);
			voxel->SetInstance(index);
			if (h == 0) {
				edges = GetEntranceDirections(p);
				voxel->SetInstanceData(edges);
			} else {
				voxel->SetInstanceData(SHF_ENTRANCE_NONE);
			}
		}
		AddRemovePathEdges(this->exit_pos, PATH_EMPTY, edges, PAS_NORMAL_PATH);
	}
}

void GentleThrillRideInstance::RemoveFromWorld()
{
	SetEntrancePos(XYZPoint16::invalid());
	SetExitPos(XYZPoint16::invalid());
	FixedRideInstance::RemoveFromWorld();
}

bool GentleThrillRideInstance::CanBeVisited(const XYZPoint16 &vox, const TileEdge edge) const
{
	return RideInstance::CanBeVisited(vox, edge) && vox == this->entrance_pos && (edge + 2) % 4 == EntranceExitRotation(this->entrance_pos);
}

void GentleThrillRideInstance::RecalculateRatings()
{
	const GentleThrillRideType *t = this->GetGentleThrillRideType();
	this->intensity_rating = t->intensity_base;
	this->nausea_rating = t->nausea_base;
	this->excitement_rating = t->excitement_base + this->working_cycles * t->excitement_increase_cycle;

	if (t->excitement_increase_scenery == 0) return;
	int scenery = 0;
	for (int x = -t->width_x; x < 2 * t->width_x; x++) {
		for (int y = -t->width_y; y < 2 * t->width_y; y++) {
			const XYZPoint16 location = OrientatedOffset(this->orientation, x, y);
			if (!IsVoxelstackInsideWorld(this->vox_pos.x + location.x, this->vox_pos.y + location.y)) continue;
			const int8 height = t->GetHeight((t->width_x + x) / 3, (t->width_y + y) / 3);
			for (int h = -height; h < 2 * height; h++) {
				Voxel *voxel = _world.GetCreateVoxel(this->vox_pos + XYZPoint16(location.x, location.y, h), false);
				if (voxel == nullptr) continue;

				if (voxel->instance == SRI_SCENERY) scenery++;                 // Bonus for building among flower beds or forests.
				if (IsImplodedSteepSlope(voxel->GetGroundSlope())) scenery++;  // Bonus for building among hills.
				/* \todo Add boni for accurately mowed lawns and building near water. */
			}
		}
	}
	this->excitement_rating += scenery * t->excitement_increase_scenery;
}

static const uint32 CURRENT_VERSION_GentleThrillRideInstance = 1;   ///< Currently supported version of %GentleThrillRideInstance.

void GentleThrillRideInstance::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("gtri");
	if (version != CURRENT_VERSION_GentleThrillRideInstance) ldr.VersionMismatch(version, CURRENT_VERSION_GentleThrillRideInstance);
	this->FixedRideInstance::Load(ldr);

	XYZPoint16 pos;
	pos.x = ldr.GetWord();
	pos.y = ldr.GetWord();
	pos.z = ldr.GetWord();
	SetEntrancePos(pos);
	pos.x = ldr.GetWord();
	pos.y = ldr.GetWord();
	pos.z = ldr.GetWord();
	SetExitPos(pos);
	ldr.ClosePattern();
}

void GentleThrillRideInstance::Save(Saver &svr)
{
	svr.StartPattern("gtri", CURRENT_VERSION_GentleThrillRideInstance);
	this->FixedRideInstance::Save(svr);
	svr.PutWord(this->entrance_pos.x);
	svr.PutWord(this->entrance_pos.y);
	svr.PutWord(this->entrance_pos.z);
	svr.PutWord(this->exit_pos.x);
	svr.PutWord(this->exit_pos.y);
	svr.PutWord(this->exit_pos.z);
	svr.EndPattern();
}

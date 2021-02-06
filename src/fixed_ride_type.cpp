/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fixed_ride_type.cpp Implementation of the fixed rides. */

#include <SDL_timer.h>
#include "stdafx.h"
#include "map.h"
#include "fixed_ride_type.h"
#include "person.h"
#include "people.h"
#include "fileio.h"
#include "math_func.h"
#include "viewport.h"

FixedRideType::FixedRideType(const RideTypeKind k) : RideType(k)
{
	this->width_x = this->width_y = this->idle_duration = this->working_duration = 0;
	animation_idle = nullptr;
	animation_starting = nullptr;
	animation_working = nullptr;
	animation_stopping = nullptr;
}

FixedRideType::~FixedRideType()
{
	/* Images and texts are handled by the sprite collector, no need to release its memory here. */
}

const ImageData *FixedRideType::GetView(uint8 orientation) const
{
	return (orientation < 4) ? this->previews[orientation] : nullptr;
}

/**
 * \fn FixedRideType::RideCapacity FixedRideType::GetRideCapacity() const
 * Compute the capacity of onride guests in the ride.
 * @return Number of batches and number of guests per batch. If either is \c 0, guests don't stay in the ride.
 */

/**
 * Constructor of a fixed ride.
 * @param type Kind of fixed ride.
 */
FixedRideInstance::FixedRideInstance(const FixedRideType *type) : RideInstance(type)
{
	this->orientation = 0;

	/* \todo Implement loading of guests into a batch. */
	const FixedRideType::RideCapacity capacity = type->GetRideCapacity();
	this->onride_guests.Configure(capacity.guests_per_batch, capacity.number_of_batches);

	this->is_working = false;
	this->time_left_in_phase = 0;
}

FixedRideInstance::~FixedRideInstance()
{
}

/**
 * Whether the ride's entrance should be rendered at the given location.
 * @param pos Absolute voxel in the world.
 * @return An entrance is located at the given location.
 */
bool FixedRideInstance::IsEntranceLocation(const XYZPoint16& pos) const
{
	return false;
}

/**
 * Whether the ride's exit should be rendered at the given location.
 * @param pos Absolute voxel in the world.
 * @return An exit is located at the given location.
 */
bool FixedRideInstance::IsExitLocation(const XYZPoint16& pos) const
{
	return false;
}

/**
 * Determine at which voxel in the world a ride piece should be located.
 * @param orientation Orientation of the fixed ride.
 * @param x Unrotated x coordinate of the ride piece, relative to the ride's base voxel.
 * @param y Unrotated y coordinate of the ride piece, relative to the ride's base voxel.
 * @return Rotated location of the ride piece, relative to the ride's base voxel.
 */
XYZPoint16 FixedRideType::OrientatedOffset(const uint8 orientation, const int x, const int y)
{
	switch (orientation % VOR_NUM_ORIENT) {
		case VOR_EAST: return XYZPoint16(x, y, 0);
		case VOR_WEST: return XYZPoint16(-x, -y, 0);
		case VOR_NORTH: return XYZPoint16(-y, x, 0);
		case VOR_SOUTH: return XYZPoint16(y, -x, 0);
	}
	NOT_REACHED();
}
/**
 * Determine at which voxel in the world a ride piece should be located.
 * @param orientation Orientation of the fixed ride.
 * @param x Rotated x coordinate of the ride piece, relative to the ride's base voxel.
 * @param x Rotated y coordinate of the ride piece, relative to the ride's base voxel.
 * @return Unrotated location of the ride piece, relative to the ride's base voxel.
 */
XYZPoint16 FixedRideType::UnorientatedOffset(const uint8 orientation, const int x, const int y)
{
	switch (orientation % VOR_NUM_ORIENT) {
		case VOR_EAST: return XYZPoint16(x, y, 0);
		case VOR_WEST: return XYZPoint16(-x, -y, 0);
		case VOR_SOUTH: return XYZPoint16(-y, x, 0);
		case VOR_NORTH: return XYZPoint16(y, -x, 0);
	}
	NOT_REACHED();
}

/**
 * Get the fixed ride type of the ride.
 * @return The fixed ride type of the ride.
 */
const FixedRideType *FixedRideInstance::GetFixedRideType() const
{
	return static_cast<const FixedRideType *>(this->type);
}

void FixedRideInstance::CloseRide() {
	RideInstance::CloseRide();
	is_working = false;
	this->time_left_in_phase = 0;
}

void FixedRideInstance::OpenRide() {
	RideInstance::OpenRide();
	is_working = false;
	this->time_left_in_phase = GetFixedRideType()->idle_duration;
}

void FixedRideInstance::GetSprites(const XYZPoint16 &vox, uint16 voxel_number, uint8 orient, const ImageData *sprites[4]) const
{
	sprites[0] = nullptr;
	sprites[2] = nullptr;
	sprites[3] = nullptr;

	const FixedRideType* t = GetFixedRideType();
	auto entrance_exit_rotation = [this, t, &vox]() {
		const XYZPoint16 corner = this->vox_pos + FixedRideType::OrientatedOffset(this->orientation, t->width_x - 1, t->width_y - 1);
		if (vox.y == std::min(this->vox_pos.y, corner.y) - 1) return VOR_WEST;
		if (vox.y == std::max(this->vox_pos.y, corner.y) + 1) return VOR_EAST;
		if (vox.x == std::min(this->vox_pos.x, corner.x) - 1) return VOR_NORTH;
		if (vox.x == std::max(this->vox_pos.x, corner.x) + 1) return VOR_SOUTH;
		NOT_REACHED();  // Invalid entrance/exit location.
	};
	auto orientation_index = [orient](const int o) {
		return (4 + o - orient) & 3;
	};
	if (IsEntranceLocation(vox)) {
		sprites[1] = _rides_manager.entrances[this->entrance_type]->images[orientation_index(entrance_exit_rotation())];
	} else if (IsExitLocation(vox)) {
		sprites[1] = _rides_manager.exits[this->exit_type]->images[orientation_index(entrance_exit_rotation())];
	} else if (voxel_number == SHF_ENTRANCE_NONE) {
		sprites[1] = nullptr;
	} else {
		const FrameSet *set_to_use;
		if (is_working) {
			/* Check whether we are starting up, slowing down, or in the middle of the phase. */
			const int relative_time = Clamp(t->working_duration - time_left_in_phase, 0, t->working_duration);
			const int start_duration = t->animation_starting->GetTotalDuration();
			if (relative_time < start_duration) {
				/* Starting up. */
				const int index = t->animation_starting->GetFrame(relative_time, false);
				assert(index >= 0 && index < t->animation_starting->frames);
				set_to_use = t->animation_starting->views[index];
			} else {
				const int stop_duration = t->animation_stopping->GetTotalDuration();
				if (relative_time > t->working_duration - stop_duration) {
					/* Slowing down. */
					const int index = t->animation_stopping->GetFrame(stop_duration + relative_time - t->working_duration, false);
					assert(index >= 0 && index < t->animation_stopping->frames);
					set_to_use = t->animation_stopping->views[index];
				} else if (t->animation_working->GetTotalDuration() > 0) {
					/* Main part of the working animation. */
					const int index = t->animation_working->GetFrame(relative_time - start_duration, true);
					assert(index >= 0 && index < t->animation_working->frames);
					set_to_use = t->animation_working->views[index];
				} else {
					/* The artist did not find time to create a working animation. */
					set_to_use = t->animation_idle;
				}
			}
		} else {
			/* The ride is idle. */
			set_to_use = t->animation_idle;
		}
		const XYZPoint16 unrotated_pos = t->UnorientatedOffset(this->orientation, vox.x - vox_pos.x, vox.y - vox_pos.y);
		sprites[1] = set_to_use->sprites[orientation_index(this->orientation)][unrotated_pos.x * t->width_y + unrotated_pos.y];
	}
}

/**
 * Update a ride instance with its position in the world.
 * @param orientation Orientation of the fixed ride.
 * @param pos Position of the fixed ride.
 */
void FixedRideInstance::SetRide(uint8 orientation, const XYZPoint16 &pos)
{
	assert(this->state == RIS_ALLOCATED);
	const int8 wx = this->GetFixedRideType()->width_x;
	const int8 wy = this->GetFixedRideType()->width_y;
	for (int8 x = 0; x < wx; ++x) {
		for (int8 y = 0; y < wy; ++y) {
			const XYZPoint16 location = FixedRideType::OrientatedOffset(orientation, x, y);
			assert(_world.GetTileOwner(pos.x + location.x, pos.y + location.y) == OWN_PARK); // May only place it in your own park.
		}
	}
	this->orientation = orientation;
	this->vox_pos = pos;
}

void FixedRideInstance::RemoveAllPeople()
{
	for (GuestBatch &gb : this->onride_guests.batches) {
		if (gb.state == BST_EMPTY) continue;

		for (GuestData &gd : gb.guests) {
			if (!gd.IsEmpty()) {
				Guest *g = _guests.Get(gd.guest);
				g->ExitRide(this, gd.entry);

				gd.Clear();
			}
		}
		gb.state = BST_EMPTY;
	}
}

void FixedRideInstance::InsertIntoWorld()
{
	const SmallRideInstance index = static_cast<SmallRideInstance>(this->GetIndex());
	const int8 wx = this->GetFixedRideType()->width_x;
	const int8 wy = this->GetFixedRideType()->width_y;
	for (int8 x = 0; x < wx; ++x) {
		for (int8 y = 0; y < wy; ++y) {
			const int8 height = this->GetFixedRideType()->GetHeight(x, y);
			const XYZPoint16 location = FixedRideType::OrientatedOffset(this->orientation, x, y);
			for (int16 h = 0; h < height; ++h) {
				Voxel *voxel = _world.GetCreateVoxel(this->vox_pos + XYZPoint16(location.x, location.y, h), true);
				assert(voxel && voxel->GetInstance() == SRI_FREE);
				voxel->SetInstance(index);
				voxel->SetInstanceData(h == 0 ? SHF_ENTRANCE_BITS : SHF_ENTRANCE_NONE);
			}
		}
	}
}

void FixedRideInstance::RemoveFromWorld()
{
	const uint16 index = this->GetIndex();
	const int8 wx = this->GetFixedRideType()->width_x;
	const int8 wy = this->GetFixedRideType()->width_y;
	for (int8 x = 0; x < wx; ++x) {
		for (int8 y = 0; y < wy; ++y) {
			const XYZPoint16 unrotated_pos = FixedRideType::OrientatedOffset(this->orientation, x, y);
			const int8 height = this->GetFixedRideType()->GetHeight(x, y);
			if (!IsVoxelstackInsideWorld(this->vox_pos.x + unrotated_pos.x, this->vox_pos.y + unrotated_pos.y)) continue;
			for (int16 h = 0; h < height; ++h) {
				Voxel *voxel = _world.GetCreateVoxel(this->vox_pos + XYZPoint16(unrotated_pos.x, unrotated_pos.y, h), false);
				if (voxel && voxel->instance != SRI_FREE) {
					assert(voxel->instance == index);
					voxel->ClearInstances();
				}
			}
		}
	}
}

void FixedRideInstance::OnAnimate(int delay)
{
	bool kickout_guests = true;
	const FixedRideType* t = GetFixedRideType();
	if (this->state == RIS_OPEN && t->working_duration > 0) {
		this->time_left_in_phase -= delay;
		bool needs_update = this->is_working;
		if (this->time_left_in_phase < 0) {
			this->is_working = !this->is_working;
			this->time_left_in_phase += (this->is_working ? t->working_duration : t->idle_duration);
			needs_update = true;
		} else {
			kickout_guests = false;
		}

		/* Update the view during the working phase to ensure smooth animations, as well as on phase switches. */
		if (needs_update) {
			for (int8 x = 0; x < t->width_x; ++x) {
				for (int8 y = 0; y < t->width_y; ++y) {
					MarkVoxelDirty(this->vox_pos + t->OrientatedOffset(this->orientation, x, y));
				}
			}
		}
	}

	this->onride_guests.OnAnimate(delay); // Update remaining time of onride guests.

	/* Kick out guests that are done. */
	if (!kickout_guests) return;
	int start = -1;
	for (;;) {
		start = this->onride_guests.GetFinishedBatch(start);
		if (start < 0) break;

		GuestBatch &gb = this->onride_guests.batches.at(start);
		GuestData & gd = gb.guests[0];
		if (!gd.IsEmpty()) {
			Guest *g = _guests.Get(gd.guest);
			g->ExitRide(this, gd.entry);

			gd.Clear();
		}
		gb.state = BST_EMPTY;
	}
}

void FixedRideInstance::Load(Loader &ldr)
{
	this->RideInstance::Load(ldr);

	this->orientation = ldr.GetByte();
	uint16 x = ldr.GetWord();
	uint16 y = ldr.GetWord();
	uint16 z = ldr.GetWord();
	this->vox_pos = XYZPoint16(x, y, z);
	this->time_left_in_phase = ldr.GetLong();
	this->is_working = ldr.GetByte() == 1;

	InsertIntoWorld();
}

void FixedRideInstance::Save(Saver &svr)
{
	this->RideInstance::Save(svr);

	svr.PutByte(this->orientation);
	svr.PutWord(this->vox_pos.x);
	svr.PutWord(this->vox_pos.y);
	svr.PutWord(this->vox_pos.z);
	svr.PutLong(this->time_left_in_phase);
	svr.PutByte(this->is_working ? 1 : 0);
}

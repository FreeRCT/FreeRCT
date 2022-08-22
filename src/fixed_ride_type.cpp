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

FixedRideType::FixedRideType(const RideTypeKind k) : RideType(k),
	width_x(0),
	width_y(0),
	default_idle_duration(0),
	working_duration(0),
	animation_idle(nullptr),
	animation_starting(nullptr),
	animation_working(nullptr),
	animation_stopping(nullptr)
{
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
FixedRideInstance::FixedRideInstance(const FixedRideType *type) : RideInstance(type),
	orientation(0),
	working_cycles(1),
	max_idle_duration(type->default_idle_duration),
	min_idle_duration(0),
	is_working(false),
	time_left_in_phase(0)
{
	const FixedRideType::RideCapacity capacity = type->GetRideCapacity();
	this->onride_guests.Configure(capacity.guests_per_batch, capacity.number_of_batches);
}

FixedRideInstance::~FixedRideInstance()
{
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
	this->time_left_in_phase = this->max_idle_duration;
}

/**
 * Get the rotation of an entrance or exit placed at the given location.
 * @param vox The absolute coordinates.
 * @return The rotation.
 */
int FixedRideInstance::EntranceExitRotation(const XYZPoint16& vox) const
{
	const FixedRideType* t = GetFixedRideType();
	const XYZPoint16 corner = this->vox_pos + OrientatedOffset(this->orientation, t->width_x - 1, t->width_y - 1);
	if (vox.y == std::min(this->vox_pos.y, corner.y) - 1) return VOR_WEST;
	if (vox.y == std::max(this->vox_pos.y, corner.y) + 1) return VOR_EAST;
	if (vox.x == std::min(this->vox_pos.x, corner.x) - 1) return VOR_NORTH;
	if (vox.x == std::max(this->vox_pos.x, corner.x) + 1) return VOR_SOUTH;
	NOT_REACHED();  // Invalid entrance/exit location.
}

void FixedRideInstance::GetSprites(const XYZPoint16 &vox, [[maybe_unused]] uint16 voxel_number, uint8 orient, const ImageData *sprites[4], uint8 *platform) const
{
	sprites[0] = nullptr;
	sprites[2] = nullptr;
	sprites[3] = nullptr;
	if (platform != nullptr && vox.z == this->vox_pos.z) *platform = PATH_NE_NW_SE_SW;

	auto orientation_index = [orient](const int o) {
		return (4 + o - orient) & 3;
	};
	const FixedRideType* t = GetFixedRideType();
	if (IsEntranceLocation(vox)) {
		const ImageData *const *array = _rides_manager.entrances[this->entrance_type]->images[orientation_index(EntranceExitRotation(vox))];
		sprites[1] = array[0];
		sprites[2] = array[1];
	} else if (IsExitLocation(vox)) {
		const ImageData *const *array = _rides_manager.exits[this->exit_type]->images[orientation_index(EntranceExitRotation(vox))];
		sprites[1] = array[0];
		sprites[2] = array[1];
	} else if (vox.z != this->vox_pos.z) {
		sprites[1] = nullptr;
	} else {
		const FrameSet *set_to_use;
		if (is_working) {
			/* Check whether we are starting up, slowing down, or in the middle of the phase. */
			const int total_duration = this->working_cycles * t->working_duration;
			const int relative_time = Clamp(total_duration - time_left_in_phase, 0, total_duration);
			const int start_duration = t->animation_starting == nullptr ? 0 : t->animation_starting->GetTotalDuration();
			if (relative_time < start_duration) {
				/* Starting up. */
				const int index = t->animation_starting->GetFrame(relative_time, false);
				assert(index >= 0 && index < t->animation_starting->frames);
				set_to_use = t->animation_starting->views[index];
			} else {
				const int stop_duration = t->animation_stopping == nullptr ? 0 : t->animation_stopping->GetTotalDuration();
				if (relative_time > total_duration - stop_duration) {
					/* Slowing down. */
					const int index = t->animation_stopping->GetFrame(stop_duration + relative_time - total_duration, false);
					assert(index >= 0 && index < t->animation_stopping->frames);
					set_to_use = t->animation_stopping->views[index];
				} else if (t->animation_working != nullptr && t->animation_working->GetTotalDuration() > 0) {
					/* Main part of the working animation. */
					const int index = t->animation_working->GetFrame(relative_time - start_duration, true);
					assert(index >= 0 && index < t->animation_working->frames);
					set_to_use = t->animation_working->views[index];
				} else {
					/* The ride does not have a working animation. */
					set_to_use = t->animation_idle;
				}
			}
		} else {
			/* The ride is idle. */
			set_to_use = t->animation_idle;
		}
		const XYZPoint16 unrotated_pos = UnorientatedOffset(this->orientation, vox.x - vox_pos.x, vox.y - vox_pos.y);
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
#ifndef NDEBUG
	assert(this->state == RIS_ALLOCATED);
	const int8 wx = this->GetFixedRideType()->width_x;
	const int8 wy = this->GetFixedRideType()->width_y;
	for (int8 x = 0; x < wx; ++x) {
		for (int8 y = 0; y < wy; ++y) {
			const XYZPoint16 location = OrientatedOffset(orientation, x, y);
			assert(_world.GetTileOwner(pos.x + location.x, pos.y + location.y) == OWN_PARK); // May only place it in your own park.
		}
	}
#endif
	this->orientation = orientation;
	this->vox_pos = pos;
}

void FixedRideInstance::RemoveAllPeople()
{
	for (GuestBatch &gb : this->onride_guests.batches) {
		if (gb.state == BST_EMPTY) continue;

		for (GuestData &gd : gb.guests) {
			if (!gd.IsEmpty()) {
				_guests.GetExisting(gd.guest)->ExitRide(this, gd.entry);

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
			const XYZPoint16 location = OrientatedOffset(this->orientation, x, y);
			for (int16 h = 0; h < height; ++h) {
				const XYZPoint16 p = this->vox_pos + XYZPoint16(location.x, location.y, h);
				Voxel *voxel = _world.GetCreateVoxel(p, true);
				assert(voxel && voxel->GetInstance() == SRI_FREE);
				voxel->SetInstance(index);
				voxel->SetInstanceData(h == 0 ? GetEntranceDirections(p) : static_cast<uint16>(SHF_ENTRANCE_NONE));
			}
		}
	}
}

void FixedRideInstance::RemoveFromWorld()
{
#ifndef NDEBUG
	const uint16 index = this->GetIndex();
#endif
	const int8 wx = this->GetFixedRideType()->width_x;
	const int8 wy = this->GetFixedRideType()->width_y;
	for (int8 x = 0; x < wx; ++x) {
		for (int8 y = 0; y < wy; ++y) {
			const XYZPoint16 unrotated_pos = OrientatedOffset(this->orientation, x, y);
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

void FixedRideInstance::OnNewDay()
{
	RideInstance::OnNewDay();
	this->RecalculateRatings();
}

void FixedRideInstance::OnAnimate(const int delay)
{
	RideInstance::OnAnimate(delay);
	if (this->broken) return;

	this->onride_guests.OnAnimate(delay); // Update remaining time of onride guests.

	const FixedRideType* t = GetFixedRideType();
	bool needs_update = this->is_working;
	bool force_start = false;
	if (this->state == RIS_OPEN && this->GetKind() != RTK_SHOP) {
		this->time_left_in_phase -= delay;
		if (this->time_left_in_phase < 0) {
			this->is_working = !this->is_working;
			this->time_left_in_phase += (this->is_working ? (this->working_cycles * t->working_duration) : this->max_idle_duration);
			force_start = this->is_working;
			needs_update = true;
		}
	} else {
		this->is_working = false;
	}

	/* Kick out guests that are done. */
	int start = -1;
	for (;;) {
		start = this->onride_guests.GetFinishedBatch(start);
		if (start < 0) break;

		GuestBatch &gb = this->onride_guests.batches.at(start);
		bool is_empty = true;
		for (GuestData &gd : gb.guests) {
			if (!gd.IsEmpty()) {
				_guests.GetExisting(gd.guest)->ExitRide(this, gd.entry);
				gd.Clear();
				/* Kick out only one guest at a time so they appear to be walking out in a nice, ordered line. */
				is_empty = false;
				break;
			}
		}
		if (is_empty) gb.state = BST_EMPTY;
	}

	/* Ensure there is always a Loading batch, except when all batches are Running. */
	start = this->onride_guests.GetLoadingBatch();
	if (start < 0) {
		start = this->onride_guests.GetFreeBatch();
	}
	if (start >= 0) {
		GuestBatch &batch = this->onride_guests.GetBatch(start);
		batch.state = BST_LOADING;
		/* Start the batch when it is full or when the timeout has elapsed. */
		bool is_full = true;
		if (!force_start) {
			for (GuestData &gd : batch.guests) {
				if (gd.IsEmpty()) {
					is_full = false;
					break;
				}
			}
		}
		/* \todo Only start if the waiting time exceeded #min_idle_duration. */
		if (is_full) {
			this->is_working = true;
			this->time_left_in_phase = this->working_cycles * t->working_duration;
			batch.Start(this->time_left_in_phase);
			needs_update = true;
		}
	}

	/* Update the view during the working phase to ensure smooth animations, as well as on phase switches. */
	if (needs_update) {
		for (int8 x = 0; x < t->width_x; ++x) {
			for (int8 y = 0; y < t->width_y; ++y) {
				MarkVoxelDirty(this->vox_pos + OrientatedOffset(this->orientation, x, y));
			}
		}
	}
}

static const uint32 CURRENT_VERSION_FixedRideInstance = 1;   ///< Currently supported version of %FixedRideInstance.

void FixedRideInstance::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("fxri");
	if (version != CURRENT_VERSION_FixedRideInstance) ldr.version_mismatch(version, CURRENT_VERSION_FixedRideInstance);
	this->RideInstance::Load(ldr);

	this->orientation = ldr.GetByte();
	uint16 x = ldr.GetWord();
	uint16 y = ldr.GetWord();
	uint16 z = ldr.GetWord();
	this->vox_pos = XYZPoint16(x, y, z);
	this->working_cycles = ldr.GetWord();
	this->max_idle_duration = ldr.GetLong();
	this->min_idle_duration = ldr.GetLong();
	this->time_left_in_phase = ldr.GetLong();
	this->is_working = ldr.GetByte() == 1;
	this->onride_guests.Load(ldr);

	InsertIntoWorld();
	ldr.ClosePattern();
}

void FixedRideInstance::Save(Saver &svr)
{
	svr.StartPattern("fxri", CURRENT_VERSION_FixedRideInstance);
	this->RideInstance::Save(svr);

	svr.PutByte(this->orientation);
	svr.PutWord(this->vox_pos.x);
	svr.PutWord(this->vox_pos.y);
	svr.PutWord(this->vox_pos.z);
	svr.PutWord(this->working_cycles);
	svr.PutLong(this->max_idle_duration);
	svr.PutLong(this->min_idle_duration);
	svr.PutLong(this->time_left_in_phase);
	svr.PutByte(this->is_working ? 1 : 0);
	this->onride_guests.Save(svr);
	svr.EndPattern();
}

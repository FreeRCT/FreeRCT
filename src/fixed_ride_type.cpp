/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fixed_ride_type.cpp Implementation of the fixed rides. */

#include "stdafx.h"
#include "map.h"
#include "fixed_ride_type.h"
#include "person.h"
#include "people.h"
#include "fileio.h"

constexpr int8 FIXED_RIDE_VOXEL_NUMBER_MAIN_POSITION = 1;  ///< Voxel instance data to indicate that the voxel is the ride's main position.
constexpr int8 FIXED_RIDE_VOXEL_NUMBER_OTHER_POSITION = 0; ///< Voxel instance data to indicate that the voxel is NOT the ride's main position.

FixedRideType::FixedRideType(const RideTypeKind k) : RideType(k)
{
	std::fill_n(this->views, lengthof(this->views), nullptr);
}

FixedRideType::~FixedRideType()
{
	/* Images and texts are handled by the sprite collector, no need to release its memory here. */
}

const ImageData *FixedRideType::GetView(uint8 orientation) const
{
	return (orientation < 4) ? this->views[orientation] : nullptr;
}

/**
 * \fn int GetRideCapacity() const
 * Compute the capacity of onride guests in the ride.
 * @return Size of a batch in bits 0..7, and number of batches in bits 8..14. \c 0 means guests don't stay in the ride.
 */

/**
 * Load a type of fixed ride from an RCD file. This function is meant to be called only from within the #Load() function of derived classes.
 * @param rcd_file Rcd file being loaded.
 * @param sprites Already loaded sprites.
 * @param texts Already loaded texts.
 * @return Loading was successful.
 */
bool FixedRideType::LoadCommon(RcdFileReader *rcd_file, const ImageMap &sprites, const TextMap &texts)
{
	this->width_x = rcd_file->GetUInt8();
	this->width_y = rcd_file->GetUInt8();
	this->heights.reset(new int8[this->width_x * this->width_y]);
	for (int8 x = 0; x < this->width_x; ++x) {
		for (int8 y = 0; y < this->width_y; ++y) {
			this->heights[x * this->width_y + y] = rcd_file->GetUInt8();
		}
	}
	return true;
}

/**
 * Constructor of a fixed ride.
 * @param type Kind of fixed ride.
 */
FixedRideInstance::FixedRideInstance(const FixedRideType *type) : RideInstance(type)
{
	this->orientation = 0;

	int capacity = type->GetRideCapacity();
	assert(capacity == 0 || (capacity & 0xFF) == 1); ///< \todo Implement loading of guests into a batch.
	this->onride_guests.Configure(capacity & 0xFF, capacity >> 8);
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

void FixedRideInstance::GetSprites(uint16 voxel_number, uint8 orient, const ImageData *sprites[4]) const
{
	sprites[0] = nullptr;
	sprites[1] = voxel_number == SHF_ENTRANCE_NONE ? nullptr : this->type->GetView((4 + this->orientation - orient) & 3);
	sprites[2] = nullptr;
	sprites[3] = nullptr;
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
			assert(_world.GetTileOwner(pos.x + x, pos.y + y) == OWN_PARK); // May only place it in your own park.
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
			for (int16 h = 0; h < height; ++h) {
				Voxel *voxel = _world.GetCreateVoxel(this->vox_pos + XYZPoint16(x, y, h), true);
				assert(voxel && voxel->GetInstance() == SRI_FREE);
				voxel->SetInstance(index);
				voxel->SetInstanceData(x == 0 && y == 0 && h == 0 ? SHF_ENTRANCE_BITS : SHF_ENTRANCE_NONE);
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
			const int8 height = this->GetFixedRideType()->GetHeight(x, y);
			for (int16 h = 0; h < height; ++h) {
				Voxel *voxel = _world.GetCreateVoxel(this->vox_pos + XYZPoint16(x, y, h), false);
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
	this->onride_guests.OnAnimate(delay); // Update remaining time of onride guests.

	/* Kick out guests that are done. */
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

	InsertIntoWorld();
}

void FixedRideInstance::Save(Saver &svr)
{
	this->RideInstance::Save(svr);

	svr.PutByte(this->orientation);
	svr.PutWord(this->vox_pos.x);
	svr.PutWord(this->vox_pos.y);
	svr.PutWord(this->vox_pos.z);
}

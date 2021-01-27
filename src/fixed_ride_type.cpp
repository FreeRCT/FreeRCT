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

FixedRideType::FixedRideType(const RideTypeKind k) : RideType(k)
{
	this->width_x = this->width_y = 0;
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
 * \fn int GetRideCapacity() const
 * Compute the capacity of onride guests in the ride.
 * @return Size of a batch in bits 0..7, and number of batches in bits 8..14. \c 0 means guests don't stay in the ride.
 */

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
 * Determine at which voxel in the world a ride piece should be located.
 * @param orientation Orientation of the fixed ride.
 * @param x Unrotated x coordinate of the ride piece, relative to the ride's base voxel.
 * @param x Unrotated y coordinate of the ride piece, relative to the ride's base voxel.
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

void FixedRideInstance::GetSprites(const XYZPoint16 &vox, uint16 voxel_number, uint8 orient, const ImageData *sprites[4]) const
{
	sprites[0] = nullptr;
	if (voxel_number == SHF_ENTRANCE_NONE) sprites[1] = nullptr;
	else {
		const FixedRideType* t = GetFixedRideType();
		const XYZPoint16 unrotated_pos = t->UnorientatedOffset(this->orientation, vox.x - vox_pos.x, vox.y - vox_pos.y);
		sprites[1] = t->views[(4 + this->orientation - orient) & 3][unrotated_pos.x * t->width_y + unrotated_pos.y];
	}
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

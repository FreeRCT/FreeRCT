/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fixed_ride_type.h Fixes rides. */

#ifndef FIXED_RIDE_TYPE_H
#define FIXED_RIDE_TYPE_H

#include "ride_type.h"
#include "guest_batches.h"

/**
 * A 'ride' where you can buy food, drinks, and other stuff you need for a visit.
 */
class FixedRideType : public RideType {
public:
	explicit FixedRideType(RideTypeKind kind);
	~FixedRideType();

	virtual int GetRideCapacity() const = 0;

	const ImageData *GetView(uint8 orientation) const override;

	int8 width_x; ///< This ride's width in x direction.
	int8 width_y; ///< This ride's width in y direction.
	std::unique_ptr<int8[]> heights;

	/**
	 * The height of this ride at the given position.
	 * @param x X coordinate, relative to the base position.
	 * @param y Y coordinate, relative to the base position.
	 * @return The height.
	 */
	inline int8 GetHeight(const int8 x, const int8 y) const
	{
		return heights[x * width_y + y];
	}

	static XYZPoint16 OrientatedOffset(const uint8 orientation, const int x, const int y);
	static XYZPoint16 UnorientatedOffset(const uint8 orientation, const int x, const int y);

	/*
	 * The sprite for the a-th animation phase at the voxel (x,y) is located at index
	 * [(a * ride_width_x * ride_width_y) + (x * ride_width_y) + y].
	 * The values for the ride's animation phases range from 1 to `animation_phases`.
	 * Animation phase 0 refers to the images used for the ride when it is not animating.
	 */
	unsigned animation_phases;               ///< Number of phases in the ride's animation (0 if the ride has no animation).
	std::unique_ptr<ImageData *[]> views[4]; ///< Ride graphics with a width and height of 64 px each, one for each voxel occupied by the ride.
	ImageData* previews[4];                  ///< Previews for the ride construction window.
};

/** Fixed rides. */
class FixedRideInstance : public RideInstance {
public:
	FixedRideInstance(const FixedRideType *type);
	~FixedRideInstance();

	const FixedRideType *GetFixedRideType() const;
	void GetSprites(const XYZPoint16 &vox, uint16 voxel_number, uint8 orient, const ImageData *sprites[4]) const override;

	virtual void SetRide(uint8 orientation, const XYZPoint16 &pos);
	void RemoveAllPeople() override;
	void OnAnimate(int delay) override;

	void Load(Loader &ldr) override;
	void Save(Saver &svr) override;

	void InsertIntoWorld() override;
	void RemoveFromWorld() override;

	uint8 orientation;  ///< Orientation of the shop.
	XYZPoint16 vox_pos; ///< Position of the shop base voxel.

protected:
	OnRideGuests onride_guests; ///< Guests in the ride.
};

#endif

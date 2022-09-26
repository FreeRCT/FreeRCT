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

	/* Information about how many guests can use the ride at the same time. */
	struct RideCapacity {
		int number_of_batches; ///< How many batches of guests fit into the ride.
		int guests_per_batch;  ///< How many guests may be in each batch.
	};
	virtual RideCapacity GetRideCapacity() const = 0;

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

	Money build_cost;                         ///< Cost to build this ride.
	int default_idle_duration;                ///< Default duration of the idle phase in milliseconds.
	int working_duration;                     ///< Duration of the working phase in milliseconds.
	const FrameSet *animation_idle;           ///< Ride graphics when the ride is not working
	const TimedAnimation *animation_starting; ///< Ride graphics when the ride is starting to work
	const TimedAnimation *animation_working;  ///< Ride graphics when the ride is working
	const TimedAnimation *animation_stopping; ///< Ride graphics when the ride is stopping to work
	const ImageData* previews[4];             ///< Previews for the ride construction window.
};

/** Fixed rides. */
class FixedRideInstance : public RideInstance {
public:
	FixedRideInstance(const FixedRideType *type);
	~FixedRideInstance();

	const FixedRideType *GetFixedRideType() const;
	void GetSprites(const XYZPoint16 &vox, uint16 voxel_number, uint8 orient, int zoom, const ImageData *sprites[4], uint8 *platform) const override;

	virtual void SetRide(uint8 orientation, const XYZPoint16 &pos);
	void RemoveAllPeople() override;
	void OnAnimate(int delay) override;
	void OpenRide() override;
	void CloseRide() override;
	void OnNewDay() override;
	void OnNewMonth() override;

	void Load(Loader &ldr) override;
	void Save(Saver &svr) override;

	void InsertIntoWorld() override;
	void RemoveFromWorld() override;
	Money ComputeBuildCost() const;
	inline Money ComputeReturnCost() const override
	{
		return this->return_cost;
	}
	inline XYZPoint16 RepresentativeLocation() const override
	{
		return this->vox_pos;
	}

	int EntranceExitRotation(const XYZPoint16& vox) const;

	Money return_cost;      ///< Money returned by removing this ride.
	uint8 orientation;      ///< Orientation of the shop.
	XYZPoint16 vox_pos;     ///< Position of the shop base voxel.
	int16 working_cycles;   ///< Number of working cycles.
	int max_idle_duration;  ///< Maximum duration of the idle phase in milliseconds.
	int min_idle_duration;  ///< Minimum duration of the idle phase in milliseconds.

protected:
	OnRideGuests onride_guests; ///< Guests in the ride.

	bool is_working;        /// Whether the ride is currently in the working phase.
	int time_left_in_phase; /// Number of milliseconds left in the current phase.
};

#endif

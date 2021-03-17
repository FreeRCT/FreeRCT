/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gentle_thrill_type.h Gentle and thrill rides. */

#ifndef GENTLE_THRILL_RIDE_TYPE_H
#define GENTLE_THRILL_RIDE_TYPE_H

#include "fixed_ride_type.h"

/**
 * A gentle ride or a thrilling ride.
 */
class GentleThrillRideType : public FixedRideType {
public:
	GentleThrillRideType();
	~GentleThrillRideType();

	bool Load(RcdFileReader *rcf_file, const ImageMap &sprites, const TextMap &texts);
	FixedRideType::RideCapacity GetRideCapacity() const override;

	const StringID *GetInstanceNames() const override;
	RideInstance *CreateInstance() const override;

	int16 working_cycles_min;            ///< Minimum number of working cycles.
	int16 working_cycles_max;            ///< Maximum number of working cycles.
	int16 working_cycles_default;        ///< Default number of working cycles.

private:
	FixedRideType::RideCapacity capacity;
};

/** A gentle or thrill ride. */
class GentleThrillRideInstance : public FixedRideInstance {
public:
	GentleThrillRideInstance(const GentleThrillRideType *type);
	~GentleThrillRideInstance();

	const GentleThrillRideType *GetGentleThrillRideType() const;

	const Recolouring *GetRecolours(const XYZPoint16 &pos) const override;
	bool IsEntranceLocation(const XYZPoint16& pos) const override;
	bool IsExitLocation(const XYZPoint16& pos) const override;
	bool CanOpenRide() const override;
	void RecalculateRatings() override;

	uint8 GetEntranceDirections(const XYZPoint16 &vox) const override;
	RideEntryResult EnterRide(int guest, const XYZPoint16 &vox, TileEdge entry) override;
	XYZPoint32 GetExit(int guest, TileEdge entry_edge) override;
	void InitializeItemPricesAndStatistics() override;

	bool CanPlaceEntranceOrExit(const XYZPoint16& pos, bool entrance) const;
	void SetEntrancePos(const XYZPoint16& pos);
	void SetExitPos(const XYZPoint16& pos);
	void RemoveFromWorld() override;
	bool CanBeVisited(const XYZPoint16 &vox, TileEdge edge) const override;
	std::pair<XYZPoint16, TileEdge> GetMechanicEntrance() const override;

	void Load(Loader &ldr) override;
	void Save(Saver &svr) override;

	XYZPoint16 entrance_pos;      ///< Location of the ride's entrance.
	XYZPoint16 exit_pos;          ///< Location of the ride's exit.
	XYZPoint16 temp_entrance_pos; ///< Temporary location of the ride's entrance while the user is moving the entrance.
	XYZPoint16 temp_exit_pos;     ///< Temporary location of the ride's exit while the user is moving the exit.
};

#endif

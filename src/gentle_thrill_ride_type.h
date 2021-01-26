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
	int GetRideCapacity() const override;

	const StringID *GetInstanceNames() const override;
	RideInstance *CreateInstance() const override;
};

/** A gentle or thrill ride. */
class GentleThrillRideInstance : public FixedRideInstance {
public:
	GentleThrillRideInstance(const GentleThrillRideType *type);
	~GentleThrillRideInstance();

	const GentleThrillRideType *GetGentleThrillRideType() const;

	uint8 GetEntranceDirections(const XYZPoint16 &vox) const override;
	RideEntryResult EnterRide(int guest, TileEdge entry) override;
	XYZPoint32 GetExit(int guest, TileEdge entry_edge) override;

	void Load(Loader &ldr) override;
	void Save(Saver &svr) override;
};

#endif

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file shop_type.h Shops. */

#ifndef SHOP_TYPE_H
#define SHOP_TYPE_H

#include "fixed_ride_type.h"

/**
 * A 'ride' where you can buy food, drinks, and other stuff you need for a visit.
 * @todo Allow for other sized sprites + different recolours.
 */
class ShopType : public FixedRideType {
public:
	ShopType();
	~ShopType();

	bool Load(RcdFileReader *rcf_file, const ImageMap &sprites, const TextMap &texts);
	FixedRideType::RideCapacity GetRideCapacity() const override;

	const StringID *GetInstanceNames() const override;
	RideInstance *CreateInstance() const override;

	uint8 flags; ///< Shop flags. @see ShopFlags
};

/** Shop 'ride'. */
class ShopInstance : public FixedRideInstance {
public:
	ShopInstance(const ShopType *type);
	~ShopInstance();

	const ShopType *GetShopType() const;
	bool CanBeVisited(const XYZPoint16 &vox, TileEdge edge) const override;

	void SetRide(uint8 orientation, const XYZPoint16 &pos) override;
	uint8 GetEntranceDirections(const XYZPoint16 &vox) const override;
	RideEntryResult EnterRide(int guest, TileEdge entry) override;
	XYZPoint32 GetExit(int guest, TileEdge entry_edge) override;

	void Load(Loader &ldr) override;
	void Save(Saver &svr) override;
};

#endif

/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file shop_type.h Shops. */

#ifndef SHOP_TYPE_H
#define SHOP_TYPE_H

#include "ride_type.h"
#include "guest_batches.h"

/**
 * A 'ride' where you can buy food, drinks, and other stuff you need for a visit.
 * @todo Allow for other sized sprites + different recolours.
 */
class ShopType : public RideType {
public:
	ShopType();
	~ShopType();

	bool Load(RcdFileReader *rcf_file, const ImageMap &sprites, const TextMap &texts);
	int GetRideCapacity() const;

	const ImageData *GetView(uint8 orientation) const override;
	const StringID *GetInstanceNames() const override;
	RideInstance *CreateInstance() const override;

	int8 height; ///< Number of voxels used by this shop.
	uint8 flags; ///< Shop flags. @see ShopFlags

protected:
	ImageData *views[4]; ///< 64 pixel wide shop graphics.
};

/** Shop 'ride'. */
class ShopInstance : public RideInstance {
public:
	ShopInstance(const ShopType *type);
	~ShopInstance();

	const ShopType *GetShopType() const;
	void GetSprites(uint16 voxel_number, uint8 orient, const ImageData *sprites[4]) const override;

	void SetRide(uint8 orientation, const XYZPoint16 &pos);
	uint8 GetEntranceDirections(const XYZPoint16 &vox) const override;
	RideEntryResult EnterRide(int guest, TileEdge entry) override;
	XYZPoint32 GetExit(int guest, TileEdge entry_edge) override;
	void RemoveAllPeople() override;
	void OnAnimate(int delay) override;

	uint8 orientation;  ///< Orientation of the shop.
	XYZPoint16 vox_pos; ///< Position of the shop base voxel.

private:
	OnRideGuests onride_guests; ///< Guests in the ride.
};

#endif

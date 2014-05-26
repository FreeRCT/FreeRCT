/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file shop_placement.h Definition of coordination code for shop placement. */

#ifndef SHOP_PLACEMENT_H
#define SHOP_PLACEMENT_H

#include "viewport.h"

class ShopType;

/** States in the shop placement. */
enum ShopPlacementState {
	SPS_OFF,      ///< Shop placement is off.
	SPS_OPENED,   ///< A shop placement window is open.
	SPS_BAD_POS,  ///< A shop has been selected, but the mouse is at a bad spot for building a shop.
	SPS_GOOD_POS, ///< A shop has been selected, and the mouse if at a good spot (is displayed in the world additions).
};

/** Result codes in trying to place a shop in the world. */
enum RidePlacementResult {
	RPR_FAIL,    ///< Ride could not be placed in the world.
	RPR_SAMEPOS, ///< Ride got placed at the same spot as previously.
	RPR_CHANGED, ///< Ride got placed at a different spot in the world.
};

/**
 * Class interacting between #RideSelectGui, and the #Viewport mouse mode #MM_SHOP_PLACEMENT.
 * @todo [low] Other rides will handle construction from their own instance window, perhaps the shop should do that too?
 */
class ShopPlacementManager : public MouseMode {
public:
	ShopPlacementManager();

	bool MayActivateMode() override;
	void ActivateMode(const Point16 &pos) override;
	void LeaveMode() override;
	bool EnableCursors() override;

	void OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos) override;
	void OnMouseButtonEvent(Viewport *vp, uint8 state) override;

	void OpenWindow();
	void CloseWindow();
	bool SetSelection(int ride_type);
	void Rotated();

	ShopPlacementState state; ///< Current state of the shop placement manager.
	TileEdge orientation;     ///< Orientation of the shop that will be placed.
	int selected_ride;        ///< Selected type of ride (negative means nothing selected).
	uint16 instance;          ///< Allocated ride instance, is #INVALID_RIDE_INSTANCE if not active.
	Point16 mouse_pos;        ///< Stored mouse position.

private:
	bool CanPlaceShop(const ShopType *selected_shop, int xpos, int ypos, int zpos);
	RidePlacementResult ComputeShopVoxel(int32 xworld, int32 yworld, int32 zworld);
	void PlaceShop(const Point16 &pos);
};

extern ShopPlacementManager _shop_placer;

#endif

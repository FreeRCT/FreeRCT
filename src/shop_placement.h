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

/** States in the shop placement. */
enum ShopPlacementState {
	SPS_OFF,           ///< Shop placement is off.
	SPS_HAS_SELECTION, ///< A shop placement selection has been set, waiting for the user to place it.
};

/** Class interacting between #RideSelectGui, and the #Viewport mouse mode #MM_SHOP_PLACEMENT. */
class ShopPlacementManager {
public:
	ShopPlacementManager();

	void SetState(ShopPlacementState new_state);

	inline bool IsActive() const;
	void Enable(Viewport *vp);

	ShopPlacementState state;      ///< Current state of the shop placement manager.
	uint8 orientation;             ///< Orientation of the shop that will be placed.
};

/**
 * Should the shop placement mouse mode be activated?
 * @return The shop placement mode should be activated.
 */
inline bool ShopPlacementManager::IsActive() const
{
	return this->state == SPS_HAS_SELECTION;
}

extern ShopPlacementManager _shop_placer;

#endif


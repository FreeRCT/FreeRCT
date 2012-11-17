/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ride_gui.cpp Gui for selecting a ride to build. */

#include "stdafx.h"
#include "window.h"
#include "language.h"
#include "sprite_store.h"
#include "ride_type.h"
#include "palette.h"
#include "shop_placement.h"
#include "table/gui_sprites.h"
#include "viewport.h"
#include "map.h"

ShopPlacementManager _shop_placer; ///< Coordination object for placing a shop.

/**
 * Gui for selecting a ride to build.
 * @ingroup gui_group
 * XXX Unify widget number parameters.
 */
class RideSelectGui : public GuiWindow {
public:
	RideSelectGui();
	~RideSelectGui();

	virtual void UpdateWidgetSize(int wid_num, BaseWidget *wid);
	virtual void DrawWidget(int wid_num, const BaseWidget *wid) const;
	virtual void OnClick(WidgetNumber wid_num);
	virtual void OnChange(ChangeCode code, uint32 parameter);

protected:
	int16 ride_types[RTK_RIDE_KIND_COUNT]; ///< Number of ride types for each kind.
	int16 current_kind; ///< Current selected kind of ride type. @see RideTypeKind
	int16 current_ride; ///< Current selected ride type (index in #RidesManager::ride_types, or \c -1).

	bool SetNewRide(int16 newKind, bool force = false);
};

/**
 * Widgets of the #RideSelectGui.
 * @ingroup gui_group
 */
enum RideSelectWidgets {
	RSEL_SHOPS,       ///< Button to select 'shops' type.
	RSEL_GENTLE,      ///< Button to select 'gentle rides' type.
	RSEL_WET,         ///< Button to select 'wet rides' type.
	RSEL_COASTER,     ///< Button to select 'coaster rides' type.
	RSEL_LIST,        ///< Ride selection list.
	RSEL_SCROLL_LIST, ///< scrollbar of the list.
	RSEL_DESC,        ///< Description of the selected ride type.
	RSEL_DISPLAY,     ///< Display the ride.
	RSEL_SELECT,      ///< 'select ride' button.
	RSEL_ROT_POS,     ///< Rotate ride in positive direction (counter-clockwise).
	RSEL_ROT_NEG,     ///< Rotate ride in negative direction (clockwise).
};

/** Widgets of the select bar. */
static const WidgetNumber _ride_type_select_bar[] = {RSEL_SHOPS, RSEL_GENTLE, RSEL_WET, RSEL_COASTER, INVALID_WIDGET_INDEX};
assert_compile(lengthof(_ride_type_select_bar) == RTK_RIDE_KIND_COUNT + 1); ///< Select bar should have all ride types.

/**
 * Widget description of the ride selection gui.
 * @ingroup gui_group
 */
static const WidgetPart _ride_select_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN), SetData(GUI_RIDE_SELECT_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
		EndContainer(),
		/* Ride types bar. */
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
			Intermediate(1, 0),
				Widget(WT_TEXT_BUTTON, RSEL_SHOPS, COL_RANGE_DARK_GREEN),
						SetData(GUI_RIDE_SELECT_SHOPS, GUI_RIDE_SELECT_SHOPS_TOOLTIP),
				Widget(WT_TEXT_BUTTON, RSEL_GENTLE, COL_RANGE_DARK_GREEN),
						SetData(GUI_RIDE_SELECT_GENTLE, GUI_RIDE_SELECT_GENTLE_TOOLTIP),
				Widget(WT_TEXT_BUTTON, RSEL_WET, COL_RANGE_DARK_GREEN),
						SetData(GUI_RIDE_SELECT_WET, GUI_RIDE_SELECT_WET_TOOLTIP),
				Widget(WT_TEXT_BUTTON, RSEL_COASTER, COL_RANGE_DARK_GREEN),
						SetData(GUI_RIDE_SELECT_COASTER, GUI_RIDE_SELECT_COASTER_TOOLTIP),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN), SetFill(1,0), SetResize(1, 0),
			EndContainer(),
		/* Available rides. */
		Intermediate(1, 3),
			Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
				Widget(WT_EMPTY, RSEL_LIST, COL_RANGE_DARK_GREEN), SetFill(0, 1), SetResize(0, 1), SetMinimalSize(100, 100),
			Widget(WT_VERT_SCROLLBAR, RSEL_SCROLL_LIST, COL_RANGE_DARK_GREEN),
			Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
					Intermediate(3, 1),
						Widget(WT_EMPTY, RSEL_DESC, COL_RANGE_DARK_GREEN), SetFill(1, 1), SetResize(1, 1),
								SetMinimalSize(200, 200),
						Intermediate(1, 2),
							Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN), SetFill(1, 1), SetResize(1, 0),
							Widget(WT_EMPTY, RSEL_DISPLAY, COL_RANGE_DARK_GREEN), SetMinimalSize(70, 70),
						Intermediate(1, 4),
							Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN), SetFill(1, 1), SetResize(1, 0),
							Widget(WT_TEXT_BUTTON, RSEL_SELECT, COL_RANGE_DARK_GREEN), SetPadding(0, 3, 3, 0),
									SetFill(1, 1), SetData(GUI_RIDE_SELECT_RIDE, GUI_RIDE_SELECT_RIDE_TOOLTIP),
							Widget(WT_IMAGE_PUSHBUTTON, RSEL_ROT_POS, COL_RANGE_DARK_GREEN), SetPadding(0, 3, 3, 0),
									SetData(SPR_GUI_ROT3D_POS, GUI_RIDE_SELECT_ROT_POS_TOOLTIP),
							Widget(WT_IMAGE_PUSHBUTTON, RSEL_ROT_NEG, COL_RANGE_DARK_GREEN), SetPadding(0, 3, 3, 0),
									SetData(SPR_GUI_ROT3D_NEG, GUI_RIDE_SELECT_ROT_NEG_TOOLTIP),
	EndContainer(),
};

RideSelectGui::RideSelectGui() : GuiWindow(WC_RIDE_SELECT)
{
	this->SetupWidgetTree(_ride_select_gui_parts, lengthof(_ride_select_gui_parts));

	/* Initialize counts of ride kinds */
	for (uint i = 0; i < lengthof(this->ride_types); i++) this->ride_types[i] = 0;
	for (uint i = 0; i < lengthof(_rides_manager.ride_types); i++) {
		const ShopType *ride = _rides_manager.ride_types[i];
		if (ride == NULL) continue;

		assert(ride->kind >= 0 && ride->kind < lengthof(this->ride_types));
		this->ride_types[ride->kind]++;
	}

	this->current_kind = 0;
	/* Select first non-0 kind (or the first kind). */
	for (uint i = 0; i < lengthof(this->ride_types); i++) {
		if (this->ride_types[i] != 0) {
			this->current_kind = i;
			break;
		}
	}
	_shop_placer.OpenWindow();
	this->SetNewRide(this->current_kind, true);
}

RideSelectGui::~RideSelectGui()
{
	_shop_placer.CloseWindow();

	/* Verify that the ride instances are all cleaned up. */
	for (uint i = 0; i < lengthof(_rides_manager.instances); i++) {
		assert(_rides_manager.instances[i].state != RIS_ALLOCATED);
	}
}

/* virtual */ void RideSelectGui::UpdateWidgetSize(int wid_num, BaseWidget *wid)
{
	switch (wid_num) {
		case RSEL_LIST:
			wid->resize_y = GetTextHeight();
			wid->min_y = 5 * wid->resize_y;

			for (uint i = 0; i < lengthof(_rides_manager.ride_types); i++) {
				const ShopType *ride = _rides_manager.ride_types[i];
				if (ride == NULL) continue;

				int width, height;
				GetTextSize(ride->GetString(SHOPS_NAME_TYPE), &width, &height);
				if (width > wid->min_x) wid->min_x = width;
			}
			break;

		case RSEL_DESC: {
			int max_height = 0;
			for (uint i = 0; i < lengthof(_rides_manager.ride_types); i++) {
				const ShopType *ride = _rides_manager.ride_types[i];
				if (ride == NULL) continue;

				int width, height;
				GetMultilineTextSize(ride->GetString(SHOPS_DESCRIPTION_TYPE), wid->min_x, &width, &height);
				if (height > max_height) max_height = height;
			}
			wid->min_y = max_height;
			break;
		}
	}
}

/* virtual */ void RideSelectGui::DrawWidget(int wid_num, const BaseWidget *wid) const
{
	switch (wid_num) {
		case RSEL_LIST: {
			Point32 rect;
			int lines = wid->pos.height / GetTextHeight(); // TODO Implement scrollbar position & size.
			rect.x = this->GetWidgetScreenX(wid);
			rect.y = this->GetWidgetScreenY(wid);
			for (int i = 0; i < (int)lengthof(_rides_manager.ride_types); i++) {
				const ShopType *ride = _rides_manager.ride_types[i];
				if (ride == NULL || ride->kind != this->current_kind) continue;
				if (lines <= 0) break;
				lines--;

				DrawString(ride->GetString(SHOPS_NAME_TYPE), (this->current_ride == i) ? TEXT_WHITE : TEXT_BLACK,
						rect.x, rect.y, wid->pos.width);
				rect.y += GetTextHeight();
			}
			break;
		}

		case RSEL_DESC:
			if (this->current_ride != -1) {
				const ShopType *ride = _rides_manager.GetRideType(this->current_ride);
				if (ride != NULL) {
					DrawMultilineString(ride->GetString(SHOPS_DESCRIPTION_TYPE),
							this->GetWidgetScreenX(wid), this->GetWidgetScreenY(wid), wid->pos.width, wid->pos.height,
							TEXT_WHITE);
				}
			}
			break;

		case RSEL_DISPLAY:
			if (this->current_ride != -1) {
				const ShopType *ride = _rides_manager.GetRideType(this->current_ride);
				static const Recolouring recolour; // Never modified, display 'original' image in the gui.
				if (ride != NULL) {
					_video->BlitImage(this->GetWidgetScreenX(wid) + wid->pos.width / 2,
							this->GetWidgetScreenY(wid) + 40, ride->views[_shop_placer.orientation], recolour, 0);
				}
			}
			break;
	}
}

/* virtual */ void RideSelectGui::OnClick(WidgetNumber wid_num)
{
	switch (wid_num) {
		case RSEL_SHOPS:
		case RSEL_GENTLE:
		case RSEL_WET:
		case RSEL_COASTER:
			if (SetNewRide(wid_num - RSEL_SHOPS)) this->MarkDirty();
			break;

		case RSEL_LIST:
		case RSEL_SCROLL_LIST:
			break;

		case RSEL_SELECT: {
			bool pressed = this->IsWidgetPressed(wid_num);
			if (pressed) {
				_shop_placer.SetSelection(-1);
				pressed = false;
			} else {
				pressed = _shop_placer.SetSelection(this->current_ride);
			}
			this->SetWidgetPressed(wid_num, pressed);
			break;
		}
		case RSEL_ROT_POS:
			_shop_placer.orientation = (_shop_placer.orientation + 3) & 3;
			this->MarkWidgetDirty(RSEL_DISPLAY);
			_shop_placer.Rotated();
			break;

		case RSEL_ROT_NEG:
			_shop_placer.orientation = (_shop_placer.orientation + 1) & 3;
			this->MarkWidgetDirty(RSEL_DISPLAY);
			_shop_placer.Rotated();
			break;
	}
}

/* virtual */ void RideSelectGui::OnChange(ChangeCode code, uint32 parameter)
{
	if (code != CHG_MOUSE_MODE_LOST) return;

	this->SetWidgetPressed(RSEL_SELECT, false);
}

/**
 * Select a kind of ride, update the #current_kind and #current_ride variables.
 * @param newKind Newly selected kind of ride.
 * @param force Update the variables even if nothing appears to have changed.
 * @return Whether selection was changed.
 */
bool RideSelectGui::SetNewRide(int16 newKind, bool force)
{
	assert(newKind >= 0 && newKind < RTK_RIDE_KIND_COUNT);
	if (!force && (newKind == this->current_kind || this->ride_types[newKind] == 0)) return false;
	this->current_kind = newKind;
	this->SetRadioButtonsSelected(_ride_type_select_bar, _ride_type_select_bar[this->current_kind]);
	int i;
	for (i = 0; i < (int)lengthof(_rides_manager.ride_types); i++) {
		const ShopType *ride = _rides_manager.ride_types[i];
		if (ride == NULL) continue;

		if (this->ride_types[ride->kind] == newKind) break;
	}
	if (i >= (int)lengthof(_rides_manager.ride_types)) {
		if (!force) return false;
		i = 0;
	}
	this->current_ride = i;
	_shop_placer.SetSelection(this->IsWidgetPressed(RSEL_SELECT) ? this->current_ride : -1);
	return true;
}


/**
 * Open the ride selection gui.
 * @ingroup gui_group
 */
void ShowRideSelectGui()
{
	if (HighlightWindowByType(WC_RIDE_SELECT)) return;
	new RideSelectGui;
}

/**
 * Does a path run at/to the bottom the given voxel in the neighbouring voxel?
 * @param xpos X coordinate of the voxel.
 * @param ypos Y coordinate of the voxel.
 * @param zpos Z coordinate of the voxel.
 * @param edge Direction to move to get the neihgbouring voxel.
 * @pre voxel coordinate must be valid in the world.
 * @todo Merge with path computations in the path placement.
 * @return Whether a path exists at the bottom of the neighbouring voxel.
 */
static bool PathExistsAtBottomEdge(int xpos, int ypos, int zpos, TileEdge edge)
{
	xpos += _tile_dxy[edge].x;
	ypos += _tile_dxy[edge].y;
	if (xpos < 0 || xpos >= _world.GetXSize() || ypos < 0 || ypos >= _world.GetYSize()) return false;

	const Voxel *vx = _world.GetVoxel(xpos, ypos, zpos);
	if (vx == NULL) return false;
	uint8 type = vx->GetType();
	if (type == VT_EMPTY || type == VT_REFERENCE) {
		/* No path here, check the voxel below. */
		if (zpos == 0) return false;
		vx = _world.GetVoxel(xpos, ypos, zpos - 1);
		if (vx == NULL || vx->GetType() != VT_SURFACE || !HasValidPath(vx)) return false;
		/* Path must end at the top of the voxel. */
		return vx->GetPathRideFlags() == _path_up_from_edge[edge];
	}
	if (type != VT_SURFACE || !HasValidPath(vx)) return false;
	/* Path must end at the bottom of the voxel. */
	return vx->GetPathRideFlags() < PATH_FLAT_COUNT || vx->GetPathRideFlags() == _path_up_from_edge[edge];
}

/**
 * Can a shop be placed at the given voxel?
 * @param selected_shop Shop to place.
 * @param xpos X coordinate of the voxel.
 * @param ypos Y coordinate of the voxel.
 * @param zpos Z coordinate of the voxel.
 * @pre voxel coordinate must be valid in the world.
 * @pre \a selected_shop may not be \c NULL.
 * @return Shop can be placed at the given position.
 */
bool ShopPlacementManager::CanPlaceShop(const ShopType *selected_shop, int xpos, int ypos, int zpos)
{
	/* 1. Can the position itself be used to build a shop? */
	const Voxel *vx = _world.GetVoxel(xpos, ypos, zpos);
	if (vx != NULL) {
		uint8 type = vx->GetType();
		if (type == VT_RIDE || type == VT_REFERENCE) return false;
		if (type == VT_SURFACE) {
			if (vx->GetPathRideNumber() != PT_INVALID) return false; // Cannot build on a path or other ride.
			return vx->GetGroundType() != GTP_INVALID; // Can always build at the surface.
		}
	}

	/* 2. Is there a path at the right place? */
	Viewport *vp = GetViewport();
	for (TileEdge entrance = EDGE_BEGIN; entrance < EDGE_COUNT; entrance++) { // Loop over the 4 unrotated directions.
		if ((selected_shop->flags & (1 << entrance)) == 0) continue; // No entrance here.
		TileEdge entr = (TileEdge)((entrance + vp->orientation + this->orientation) & 3); // Perform rotation specified by the user in the GUI.
		if (PathExistsAtBottomEdge(xpos, ypos, zpos, entr)) return true;
	}
	return false;
}

/**
 * Decide at which voxel to place a shop. It should be placed at a voxel intersecting with the view line
 * through the given point in the world.
 * @param xworld X coordinate of the point.
 * @param yworld Y coordinate of the point.
 * @param zworld Z coordinate of the point.
 * @return Result of the placement process.
 */
RidePlacementResult ShopPlacementManager::ComputeShopVoxel(int32 xworld, int32 yworld, int32 zworld)
{
	RideInstance *ri = _rides_manager.GetRideInstance(this->instance);
	assert(ri != NULL && ri->type != NULL); // It should be possible to set the position of a shop.

	Viewport *vp = GetViewport();
	if (vp == NULL) return RPR_FAIL;

	int dx, dy; // Change of xworld and yworld for every (zworld / 2) change.
	switch (vp->orientation) {
		case VOR_NORTH: dx =  1; dy =  1; break;
		case VOR_WEST:  dx = -1; dy =  1; break;
		case VOR_SOUTH: dx = -1; dy = -1; break;
		case VOR_EAST:  dx =  1; dy = -1; break;
		default: NOT_REACHED();
	}

	/* Move to the top voxel of the world. */
	int zpos = WORLD_Z_SIZE - 1;
	int dz = zpos * 256 - zworld;
	xworld += dx * dz / 2;
	yworld += dy * dz / 2;

	while (zpos >= 0) {
		int xpos = xworld / 256;
		int ypos = yworld / 256;
		if (xpos >= 0 && xpos < _world.GetXSize() && ypos >= 0 && ypos < _world.GetYSize() &&
				this->CanPlaceShop(ri->type, xpos, ypos, zpos)) {
			/* Position of the shop the same as previously? */
			if (ri->xpos != (uint16)xpos || ri->ypos != (uint16)ypos || ri->zpos != (uint8)zpos ||
					ri->orientation != this->orientation) {
				ri->xpos = xpos;
				ri->ypos = ypos;
				ri->zpos = zpos;
				ri->orientation = (this->orientation + vp->orientation) & 3;
				return RPR_CHANGED;
			}
			return RPR_SAMEPOS;
		} else {
			/* Since z gets smaller, we subtract dx and dy, thus the checks reverse. */
			if (xpos < 0 && dx > 0) break;
			if (xpos >= _world.GetXSize() && dx < 0) break;
			if (ypos < 0 && dy > 0) break;
			if (ypos >= _world.GetYSize() && dy < 0) break;
		}
		xworld -= 128 * dx;
		yworld -= 128 * dy;
		zpos--;
	}
	return RPR_FAIL;
}

/**
 * Place the shop at the indicated mouse position (if possible), and update the state to #SPS_BAD_POS or #SPS_GOOD_POS.
 * @param pos Mouse position.
 * @pre World additions should be ours to use, and have been enabled.
 */
void ShopPlacementManager::PlaceShop(const Point16 &pos)
{
	Viewport *vp = GetViewport();
	Point32 wxy = vp->ComputeHorizontalTranslation(vp->rect.width / 2 - pos.x, vp->rect.height / 2 - pos.y);

	/* Clean current display if needed. */
	switch (this->ComputeShopVoxel(wxy.x,wxy.y, vp->zview)) {
		case RPR_FAIL:
			if (this->state == SPS_BAD_POS) return; // Nothing to do.
			_additions.MarkDirty(vp);
			_additions.Clear();
			this->state = SPS_BAD_POS;
			return;

		case RPR_SAMEPOS:
			if (this->state == SPS_GOOD_POS) return; // Already placed and shown.
			/* FALL-THROUGH */

		case RPR_CHANGED: {
			_additions.MarkDirty(vp);
			_additions.Clear();

			RideInstance *ri = _rides_manager.GetRideInstance(this->instance);
			Voxel *vx = _additions.GetCreateVoxel(ri->xpos, ri->ypos, ri->zpos, true);
			RideVoxelData rvd;
			rvd.ride_number = this->instance;
			rvd.flags = 0; // XXX ride entrance bits (although it is silly to assume entrance is part of it?)
			vx->SetRide(rvd);
			_additions.MarkDirty(vp);
			vp->EnsureAdditionsAreVisible();
			this->state = SPS_GOOD_POS;
			return;
		}

		default: NOT_REACHED();
	}
}

/** Constructor of the shop placement coordinator. */
ShopPlacementManager::ShopPlacementManager() : MouseMode(WC_RIDE_SELECT, MM_SHOP_PLACEMENT)
{
	this->state = SPS_OFF;
	this->selected_ride = -1;
	this->instance = INVALID_RIDE_INSTANCE;
}

/* virtual */ bool ShopPlacementManager::EnableCursors()
{
	return true;
}

/* virtual */ bool ShopPlacementManager::MayActivateMode()
{
	return this->state == SPS_BAD_POS || this->state == SPS_GOOD_POS
			|| (this->state == SPS_OPENED && this->selected_ride >= 0);
}


/* virtual */ void ShopPlacementManager::ActivateMode(const Point16 &pos)
{
	this->mouse_pos = pos;

	assert(this->state != SPS_OFF);

	if (this->state == SPS_OPENED) {
		if (this->selected_ride < 0) return;
		EnableWorldAdditions();
	}

	/* (this->state == SPS_OPENED && this->selected_ride >= 0) || this->state == SPS_BAD_POS || this->state == SPS_GOOD_POS */
	this->PlaceShop(pos);
}

/* virtual */ void ShopPlacementManager::LeaveMode()
{
	DisableWorldAdditions();

	if (this->state == SPS_BAD_POS || this->state == SPS_GOOD_POS) {
		/* Deselect the ride selection, notify the window, so it can raise the select button and change the selected ride. */
		NotifyChange(WC_RIDE_SELECT, CHG_MOUSE_MODE_LOST, 0);
		this->selected_ride = -1;

		/* Free the created ride instance. */
		RideInstance *ri = _rides_manager.GetRideInstance(this->instance);
		ri->FreeRide();
		this->instance = INVALID_RIDE_INSTANCE;

		this->state = SPS_OPENED;
	}
}

/* virtual */ void ShopPlacementManager::OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos)
{
	this->mouse_pos = pos;

	if (this->state == SPS_OFF || this->state == SPS_OPENED) return;
	this->PlaceShop(pos);
}

/* virtual */ void ShopPlacementManager::OnMouseButtonEvent(Viewport *vp, uint8 mstate)
{
	if (this->state != SPS_GOOD_POS || mstate != MB_LEFT) return;

	/* Buy (mark ride instance as closed) */
	RideInstance *ri = _rides_manager.GetRideInstance(this->instance);
	ri->CloseRide();
	this->instance = INVALID_RIDE_INSTANCE;

	/* Disable additions (has been moved to the real world) */
	_additions.Commit();
	DisableWorldAdditions();

	/* Deselect ride selection. */
	NotifyChange(WC_RIDE_SELECT, CHG_MOUSE_MODE_LOST, 0);
	this->selected_ride = -1;

	/* Switch to another mouse mode. */
	this->state = SPS_OPENED;
	_mouse_modes.SetViewportMousemode();
}

/** The user opened the window. */
void ShopPlacementManager::OpenWindow()
{
	if (this->state == SPS_OFF) {
		this->state = SPS_OPENED;
		this->orientation = 0;
		this->selected_ride = -1;
		this->instance = INVALID_RIDE_INSTANCE;
	}
}

/** The user closed the window. */
void ShopPlacementManager::CloseWindow()
{
	if (this->state == SPS_OFF) return;

	if (this->state == SPS_BAD_POS || this->state == SPS_GOOD_POS ||
			(this->state == SPS_OPENED && this->selected_ride >= 0)) {
		/* Free the created ride instance. */
		RideInstance *ri = _rides_manager.GetRideInstance(this->instance);
		ri->FreeRide();
		this->instance = INVALID_RIDE_INSTANCE;

		this->selected_ride = -1;
		this->state = SPS_OFF;
		_mouse_modes.SetViewportMousemode();
		return;
	}
	this->state = SPS_OFF; // Was in opened without selection.
}

/**
 * User has selected a ride (or \c -1 for selecting no ride).
 * @param ride_type Selected type of ride (\c -1 means selection is disabled).
 * @return New ride type has been set successfully.
 */
bool ShopPlacementManager::SetSelection(int ride_type)
{
	this->selected_ride = ride_type;

	if (ride_type >= 0) {
		/* New ride type selected. */
		if (this->state == SPS_OFF) return true;

		if (this->state == SPS_OPENED) {
			/* Create/allocate new ride instance. */
			assert(this->instance == INVALID_RIDE_INSTANCE);
			const ShopType *st = _rides_manager.GetRideType(this->selected_ride);
			if (st == NULL) {
				this->selected_ride = -1;
				return false;
			}
			this->instance = _rides_manager.GetFreeInstance();
			if (this->instance == INVALID_RIDE_INSTANCE) {
				this->selected_ride = -1;
				return false;
			}
			RideInstance *ri = _rides_manager.GetRideInstance(this->instance);
			ri->state = RIS_ALLOCATED;
			ri->ClaimRide(st, (uint8 *)"");

			/* Depress button (Already done) */

			_mouse_modes.SetMouseMode(this->mode);
			/* Fall through to the shop placement. */
		}

		/* States SPS_BAD_POS, SPS_GOOD_POS. */
		this->PlaceShop(this->mouse_pos);
		return true;
	} else {
		/* No ride type selected. */
		if (this->state == SPS_OFF || this->state == SPS_OPENED) return true;

		/* States SPS_BAD_POS, SPS_GOOD_POS. */
		DisableWorldAdditions();
		this->selected_ride = -1;

		/* Free the created ride instance. */
		RideInstance *ri = _rides_manager.GetRideInstance(this->instance);
		ri->FreeRide();
		this->instance = INVALID_RIDE_INSTANCE;

		this->state = SPS_OPENED;
		_mouse_modes.SetViewportMousemode();
		return true;
	}
}

/** The shop was rotated; recompute position if needed. */
void ShopPlacementManager::Rotated()
{
	if (this->state == SPS_OFF || this->state == SPS_OPENED) return;
	this->PlaceShop(this->mouse_pos);
}

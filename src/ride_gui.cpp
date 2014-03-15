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
#include "shop_type.h"
#include "palette.h"
#include "shop_placement.h"
#include "viewport.h"
#include "map.h"

#include "gui_sprites.h"

ShopPlacementManager _shop_placer; ///< Coordination object for placing a shop.

/**
 * GUI for selecting a ride to build.
 * @ingroup gui_group
 * @todo Unify widget number parameters.
 */
class RideSelectGui : public GuiWindow {
public:
	RideSelectGui();
	~RideSelectGui();

	void UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid) override;
	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
	void OnClick(WidgetNumber wid_num) override;
	void OnChange(ChangeCode code, uint32 parameter) override;

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
	RSEL_DISPLAY,     ///< Display the ride type.
	RSEL_SELECT,      ///< 'select ride type' button.
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

RideSelectGui::RideSelectGui() : GuiWindow(WC_RIDE_SELECT, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_ride_select_gui_parts, lengthof(_ride_select_gui_parts));

	/* Initialize counts of ride kinds */
	for (uint i = 0; i < lengthof(this->ride_types); i++) this->ride_types[i] = 0;
	for (uint i = 0; i < lengthof(_rides_manager.ride_types); i++) {
		const RideType *ride_type = _rides_manager.ride_types[i];
		if (ride_type == nullptr) continue;

		assert(ride_type->kind >= 0 && ride_type->kind < lengthof(this->ride_types));
		this->ride_types[ride_type->kind]++;
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
	_rides_manager.CheckNoAllocatedRides(); // Check that no rides are being constructed.
}

void RideSelectGui::UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid)
{
	switch (wid_num) {
		case RSEL_LIST:
			wid->resize_y = GetTextHeight();
			wid->min_y = 5 * wid->resize_y;

			for (uint i = 0; i < lengthof(_rides_manager.ride_types); i++) {
				const RideType *ride_type = _rides_manager.ride_types[i];
				if (ride_type == nullptr) continue;

				int width, height;
				GetTextSize(ride_type->GetString(ride_type->GetTypeName()), &width, &height);
				if (width > wid->min_x) wid->min_x = width;
			}
			break;

		case RSEL_DESC: {
			int max_height = 0;
			for (uint i = 0; i < lengthof(_rides_manager.ride_types); i++) {
				const RideType *ride_type = _rides_manager.ride_types[i];
				if (ride_type == nullptr) continue;

				int width, height;
				GetMultilineTextSize(ride_type->GetString(ride_type->GetTypeDescription()), wid->min_x, &width, &height);
				if (height > max_height) max_height = height;
			}
			wid->min_y = max_height;
			break;
		}
	}
}

void RideSelectGui::DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const
{
	switch (wid_num) {
		case RSEL_LIST: {
			Point32 rect;
			int lines = wid->pos.height / GetTextHeight(); /// \todo Implement scrollbar position & size.
			rect.x = this->GetWidgetScreenX(wid);
			rect.y = this->GetWidgetScreenY(wid);
			for (int i = 0; i < (int)lengthof(_rides_manager.ride_types); i++) {
				const RideType *ride_type = _rides_manager.ride_types[i];
				if (ride_type == nullptr || ride_type->kind != this->current_kind) continue;
				if (lines <= 0) break;
				lines--;

				DrawString(ride_type->GetString(ride_type->GetTypeName()), (this->current_ride == i) ? TEXT_WHITE : TEXT_BLACK,
						rect.x, rect.y, wid->pos.width);
				rect.y += GetTextHeight();
			}
			break;
		}

		case RSEL_DESC:
			if (this->current_ride != -1) {
				const RideType *ride_type = _rides_manager.GetRideType(this->current_ride);
				if (ride_type != nullptr) {
					DrawMultilineString(ride_type->GetString(ride_type->GetTypeDescription()),
							this->GetWidgetScreenX(wid), this->GetWidgetScreenY(wid), wid->pos.width, wid->pos.height,
							TEXT_WHITE);
				}
			}
			break;

		case RSEL_DISPLAY:
			if (this->current_ride != -1) {
				const RideType *ride_type = _rides_manager.GetRideType(this->current_ride);
				/* Display the selected shop in the ride select window. */
				if (ride_type != nullptr && ride_type->kind == RTK_SHOP) {
					static const Recolouring recolour; // Never modified, display 'original' image in the GUI.
					_video->BlitImage(this->GetWidgetScreenX(wid) + wid->pos.width / 2,
							this->GetWidgetScreenY(wid) + 40, ride_type->GetView(_shop_placer.orientation),
							recolour, 0);
				}
			}
			break;
	}
}

void RideSelectGui::OnClick(WidgetNumber wid_num)
{
	switch (wid_num) {
		case RSEL_SHOPS:
		case RSEL_GENTLE:
		case RSEL_WET:
		case RSEL_COASTER:
			if (this->SetNewRide(wid_num - RSEL_SHOPS)) this->MarkDirty();
			break;

		case RSEL_LIST:
		case RSEL_SCROLL_LIST:
			break;

		case RSEL_SELECT: {
			if (this->ride_types[this->current_kind] == 0) return;

			if (this->current_kind == RTK_SHOP) {
				bool pressed = this->IsWidgetPressed(wid_num);
				if (pressed) {
					_shop_placer.SetSelection(-1);
					pressed = false;
				} else {
					pressed = _shop_placer.SetSelection(this->current_ride);
				}
				this->SetWidgetPressed(wid_num, pressed);
			} else {
				if (this->current_ride != -1) {
					const RideType *ride_type = _rides_manager.GetRideType(this->current_ride);
					assert(ride_type != nullptr && ride_type->kind == RTK_COASTER);
					uint16 instance = _rides_manager.GetFreeInstance(ride_type);
					if (instance != INVALID_RIDE_INSTANCE) {
						RideInstance *ri = _rides_manager.CreateInstance(ride_type, instance);
						_rides_manager.NewInstanceAdded(instance);
						ShowCoasterManagementGui(ri);
					}
				}
			}
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

void RideSelectGui::OnChange(ChangeCode code, uint32 parameter)
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
	if (!force && newKind == this->current_kind) return false;
	this->current_kind = newKind;
	this->SetRadioButtonsSelected(_ride_type_select_bar, _ride_type_select_bar[this->current_kind]);

	this->current_ride = -1;
	if (this->ride_types[newKind] > 0) {
		for (int i = 0; i < (int)lengthof(_rides_manager.ride_types); i++) {
			const RideType *ride_type = _rides_manager.ride_types[i];
			if (ride_type == nullptr || ride_type->kind != this->current_kind) continue;

			this->current_ride = i;
			break;
		}
	}

	/* Update selection with the new ride. */
	switch (this->current_kind) {
		case RTK_SHOP:
			_shop_placer.SetSelection(this->IsWidgetPressed(RSEL_SELECT) ? this->current_ride : -1);
			break;

		case RTK_GENTLE:
		case RTK_WET:
		case RTK_COASTER:
			_shop_placer.SetSelection(-1); // De-select a shop.
			/* Widget should behave as a mono-stable button if not selecting a shop. */
			this->SetWidgetPressed(RSEL_SELECT, false);
			break;

		default:
			NOT_REACHED();
	}
	return true;
}


/**
 * Open the ride selection GUI.
 * @ingroup gui_group
 */
void ShowRideSelectGui()
{
	if (HighlightWindowByType(WC_RIDE_SELECT, ALL_WINDOWS_OF_TYPE)) return;
	new RideSelectGui;
}

/**
 * Can a shop be placed at the given voxel?
 * @param selected_shop Shop to place.
 * @param xpos X coordinate of the voxel.
 * @param ypos Y coordinate of the voxel.
 * @param zpos Z coordinate of the voxel.
 * @pre voxel coordinate must be valid in the world.
 * @pre \a selected_shop may not be \c nullptr.
 * @return Shop can be placed at the given position.
 */
bool ShopPlacementManager::CanPlaceShop(const ShopType *selected_shop, int xpos, int ypos, int zpos)
{
	/* 1. Can the position itself be used to build a shop? */
	if (_world.GetTileOwner(xpos, ypos) != OWN_PARK) return false;
	const Voxel *vx = _world.GetVoxel(xpos, ypos, zpos);
	if (vx != nullptr) {
		if (!vx->CanPlaceInstance()) return false; // Cannot build on a path or other ride.
		return vx->GetGroundType() != GTP_INVALID && vx->GetGroundSlope() == SL_FLAT; // Can build at a flat surface.
	}

	/* 2. Is the shop just above non-flat ground? */
	if (zpos > 0) {
		vx = _world.GetVoxel(xpos, ypos, zpos - 1);
		if (vx != nullptr && vx->GetInstance() == SRI_FREE &&
				vx->GetGroundType() != GTP_INVALID && vx->GetGroundSlope() != SL_FLAT) return true;
	}

	/* 3. Is there a path at the right place? */
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
	ShopInstance *si = static_cast<ShopInstance *>(_rides_manager.GetRideInstance(this->instance));
	assert(si != nullptr && si->GetKind() == RTK_SHOP); // It should be possible to set the position of a shop.
	const ShopType *st = si->GetShopType();
	assert(st != nullptr);

	Viewport *vp = GetViewport();
	if (vp == nullptr) return RPR_FAIL;

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
		if (IsVoxelstackInsideWorld(xpos, ypos) && this->CanPlaceShop(st, xpos, ypos, zpos)) {
			/* Position of the shop the same as previously? */
			if (si->xpos != (uint16)xpos || si->ypos != (uint16)ypos || si->zpos != (uint8)zpos ||
					si->orientation != this->orientation) {
				si->SetRide((this->orientation + vp->orientation) & 3, xpos, ypos, zpos);
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
	switch (this->ComputeShopVoxel(wxy.x, wxy.y, vp->zview)) {
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

			/// \todo Let the shop do this.
			ShopInstance *si = static_cast<ShopInstance *>(_rides_manager.GetRideInstance(this->instance));
			assert(si != nullptr && si->GetKind() == RTK_SHOP);
			Voxel *vx = _additions.GetCreateVoxel(si->xpos, si->ypos, si->zpos, true);
			assert(this->instance >= SRI_FULL_RIDES && this->instance <= SRI_LAST);
			vx->SetInstance((SmallRideInstance)this->instance);
			uint8 entrances = si->GetEntranceDirections(si->xpos, si->ypos, si->zpos);
			vx->SetInstanceData(entrances);
			AddRemovePathEdges(si->xpos, si->ypos, si->zpos, PATH_EMPTY, entrances, true, true);
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

bool ShopPlacementManager::EnableCursors()
{
	return true;
}

bool ShopPlacementManager::MayActivateMode()
{
	return this->state == SPS_BAD_POS || this->state == SPS_GOOD_POS
			|| (this->state == SPS_OPENED && this->selected_ride >= 0);
}


void ShopPlacementManager::ActivateMode(const Point16 &pos)
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

void ShopPlacementManager::LeaveMode()
{
	DisableWorldAdditions();

	if (this->state == SPS_BAD_POS || this->state == SPS_GOOD_POS) {
		/* Deselect the ride selection, notify the window, so it can raise the select button and change the selected ride. */
		NotifyChange(WC_RIDE_SELECT, ALL_WINDOWS_OF_TYPE, CHG_MOUSE_MODE_LOST, 0);
		this->selected_ride = -1;

		/* Free the created ride instance. */
		_rides_manager.DeleteInstance(this->instance);
		this->instance = INVALID_RIDE_INSTANCE;

		this->state = SPS_OPENED;
	}
}

void ShopPlacementManager::OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos)
{
	this->mouse_pos = pos;

	if (this->state == SPS_OFF || this->state == SPS_OPENED) return;
	this->PlaceShop(pos);
}

void ShopPlacementManager::OnMouseButtonEvent(Viewport *vp, uint8 mstate)
{
	if (this->state != SPS_GOOD_POS || mstate != MB_LEFT) return;

	/* Buy (mark ride instance as closed) */
	_rides_manager.NewInstanceAdded(this->instance);
	this->instance = INVALID_RIDE_INSTANCE;

	/* Disable additions (has been moved to the real world) */
	_additions.Commit();
	DisableWorldAdditions();

	/* Deselect ride selection. */
	NotifyChange(WC_RIDE_SELECT, ALL_WINDOWS_OF_TYPE, CHG_MOUSE_MODE_LOST, 0);
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
		_rides_manager.DeleteInstance(this->instance);
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
			const RideType *rt = _rides_manager.GetRideType(this->selected_ride);
			if (rt == nullptr) {
				this->selected_ride = -1;
				return false;
			}
			this->instance = _rides_manager.GetFreeInstance(rt);
			if (this->instance == INVALID_RIDE_INSTANCE) {
				this->selected_ride = -1;
				return false;
			}
			_rides_manager.CreateInstance(rt, this->instance);

			/* Depress button (Already done). */

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
		_rides_manager.DeleteInstance(this->instance);
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

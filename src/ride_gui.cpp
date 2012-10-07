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
					Intermediate(2, 1),
						Widget(WT_EMPTY, RSEL_DESC, COL_RANGE_DARK_GREEN), SetFill(1, 1), SetResize(1, 1),
								SetMinimalSize(200, 200),
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
	this->SetNewRide(this->current_kind, true);
}

RideSelectGui::~RideSelectGui()
{
	_shop_placer.SetState(SPS_OFF);
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
				const ShopType *ride = _rides_manager.ride_types[this->current_ride];
				if (ride != NULL) {
					DrawMultilineString(ride->GetString(SHOPS_DESCRIPTION_TYPE),
							this->GetWidgetScreenX(wid), this->GetWidgetScreenY(wid), wid->pos.width, wid->pos.height,
							TEXT_WHITE);
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
			this->SetWidgetPressed(wid_num, !pressed);
			if (!pressed) {
				_shop_placer.SetState(SPS_HAS_SELECTION);
				_shop_placer.orientation = VOR_NORTH;
			} else {
				_shop_placer.SetState(SPS_OFF);
			}
			break;
		}
		case RSEL_ROT_POS:
			_shop_placer.orientation = (_shop_placer.orientation + 1) & 3;
			// XXX Mark things as dirty.
			break;

		case RSEL_ROT_NEG:
			_shop_placer.orientation = (_shop_placer.orientation + 3) & 3;
			// XXX Mark things as dirty.
			break;
	}
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
	this->current_ride = (i < (int)lengthof(_rides_manager.ride_types)) ? i : 0;
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

/** Constructor of the shop placement coordinator. */
ShopPlacementManager::ShopPlacementManager()
{
	this->state = SPS_OFF;
}

/**
 * Set the state of the shop placer.
 * @param new_state New state of the shop placer object.
 */
void ShopPlacementManager::SetState(ShopPlacementState new_state)
{
	if (this->state == new_state) return;
	this->state = new_state;

	/* Will select this mode when the window is high enough in the stack, and enable us. */
	SetViewportMousemode();
}

/**
 * Viewport requests enabling the mode, make it work again.
 * @param vp %Viewport performing the request.
 */
void ShopPlacementManager::Enable(Viewport *vp)
{
	if (this->state == SPS_OFF) return;

	vp->SetMouseModeState(MM_SHOP_PLACEMENT);
}


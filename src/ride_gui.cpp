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
#include "viewport.h"
#include "map.h"

#include "gui_sprites.h"

/**
 * GUI for selecting a ride to build.
 * @ingroup gui_group
 * @todo Unify widget number parameters.
 */
class RideSelectGui : public GuiWindow {
public:
	RideSelectGui();

	void UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid) override;
	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
	void OnClick(WidgetNumber wid_num, const Point16 &pos) override;

protected:
	int16 ride_types[RTK_RIDE_KIND_COUNT]; ///< Number of ride types for each kind.
	int16 current_kind; ///< Current selected kind of ride type. @see RideTypeKind
	int16 current_ride; ///< Current selected ride type (index in #RidesManager::ride_types, or \c -1).

	bool SetNewRideKind(int16 new_kind, bool force = false);
	void SetNewRide(int new_number);
};

/**
 * Widgets of the #RideSelectGui.
 * @ingroup gui_group
 */
enum RideSelectWidgets {
	RSEL_SHOPS,       ///< Button to select 'shops' type.
	RSEL_GENTLE,      ///< Button to select 'gentle rides' type.
	RSEL_THRILL,      ///< Button to select 'thrill rides' type.
	RSEL_WET,         ///< Button to select 'wet rides' type.
	RSEL_COASTER,     ///< Button to select 'coaster rides' type.
	RSEL_LIST,        ///< Ride selection list.
	RSEL_SCROLL_LIST, ///< scrollbar of the list.
	RSEL_DESC,        ///< Description of the selected ride type.
	RSEL_DISPLAY,     ///< Display the ride type.
	RSEL_SELECT,      ///< 'select ride type' button.
};

/** Widgets of the select bar. */
static const WidgetNumber _ride_type_select_bar[] = {RSEL_SHOPS, RSEL_GENTLE, RSEL_THRILL, RSEL_WET, RSEL_COASTER, INVALID_WIDGET_INDEX};
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
			Intermediate(2, 1),
				Intermediate(1, 0),
					Widget(WT_LEFT_FILLER_TAB, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
					Widget(WT_TEXT_TAB, RSEL_SHOPS, COL_RANGE_DARK_GREEN),
							SetData(GUI_RIDE_SELECT_SHOPS, GUI_RIDE_SELECT_SHOPS_TOOLTIP),
					Widget(WT_TEXT_TAB, RSEL_GENTLE, COL_RANGE_DARK_GREEN),
							SetData(GUI_RIDE_SELECT_GENTLE, GUI_RIDE_SELECT_GENTLE_TOOLTIP),
					Widget(WT_TEXT_TAB, RSEL_THRILL, COL_RANGE_DARK_GREEN),
							SetData(GUI_RIDE_SELECT_THRILL, GUI_RIDE_SELECT_THRILL_TOOLTIP),
					Widget(WT_TEXT_TAB, RSEL_WET, COL_RANGE_DARK_GREEN),
							SetData(GUI_RIDE_SELECT_WET, GUI_RIDE_SELECT_WET_TOOLTIP),
					Widget(WT_TEXT_TAB, RSEL_COASTER, COL_RANGE_DARK_GREEN),
							SetData(GUI_RIDE_SELECT_COASTER, GUI_RIDE_SELECT_COASTER_TOOLTIP),
					Widget(WT_RIGHT_FILLER_TAB, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN), SetFill(1,0), SetResize(1, 0),
				EndContainer(),
				/* Available rides. */
				Widget(WT_TAB_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
					Intermediate(1, 3),
						Widget(WT_EMPTY, RSEL_LIST, COL_RANGE_DARK_GREEN), SetFill(0, 1), SetResize(0, 1), SetMinimalSize(100, 100),
						Widget(WT_VERT_SCROLLBAR, RSEL_SCROLL_LIST, COL_RANGE_DARK_GREEN),
						Intermediate(3, 1),
							Widget(WT_EMPTY, RSEL_DESC, COL_RANGE_DARK_GREEN), SetFill(1, 1), SetResize(1, 1), SetMinimalSize(200, 200),
							Intermediate(1, 2),
								Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN), SetFill(1, 1), SetResize(1, 0),
								Widget(WT_EMPTY, RSEL_DISPLAY, COL_RANGE_DARK_GREEN), SetMinimalSize(70, 70),
							Intermediate(1, 2),
								Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN), SetFill(1, 1), SetResize(1, 0),
								Widget(WT_TEXT_BUTTON, RSEL_SELECT, COL_RANGE_DARK_GREEN), SetPadding(0, 3, 3, 0),
										SetFill(1, 1), SetData(GUI_RIDE_SELECT_RIDE, GUI_RIDE_SELECT_RIDE_TOOLTIP),
	EndContainer(),
};

RideSelectGui::RideSelectGui() : GuiWindow(WC_RIDE_SELECT, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_ride_select_gui_parts, lengthof(_ride_select_gui_parts));
	this->SetScrolledWidget(RSEL_LIST, RSEL_SCROLL_LIST);

	/* Initialize counts of ride kinds */
	for (uint i = 0; i < lengthof(this->ride_types); i++) this->ride_types[i] = 0;
	for (int i = 0; i < MAX_NUMBER_OF_RIDE_TYPES; i++) {
		const RideType *ride_type = _rides_manager.GetRideType(i);
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
	this->SetNewRideKind(this->current_kind, true);
}

void RideSelectGui::UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid)
{
	switch (wid_num) {
		case RSEL_LIST:
			wid->resize_y = GetTextHeight();
			wid->min_y = 5 * wid->resize_y;

			for (int i = 0; i < MAX_NUMBER_OF_RIDE_TYPES; i++) {
				const RideType *ride_type = _rides_manager.GetRideType(i);
				if (ride_type == nullptr) continue;

				int width, height;
				GetTextSize(ride_type->GetString(ride_type->GetTypeName()), &width, &height);
				if (width > wid->min_x) wid->min_x = width;
			}
			break;

		case RSEL_DESC: {
			int max_height = 0;
			for (int i = 0; i < MAX_NUMBER_OF_RIDE_TYPES; i++) {
				const RideType *ride_type = _rides_manager.GetRideType(i);
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
			Point32 rect(this->GetWidgetScreenX(wid), this->GetWidgetScreenY(wid));
			const ScrollbarWidget *sb = this->GetWidget<ScrollbarWidget>(RSEL_SCROLL_LIST);
			int lines = sb->GetVisibleCount();
			int start = sb->GetStart();
			int counter = 0;
			for (int i = 0; i < MAX_NUMBER_OF_RIDE_TYPES && lines > 0; i++) {
				const RideType *ride_type = _rides_manager.GetRideType(i);
				if (ride_type == nullptr || ride_type->kind != this->current_kind) continue;
				if (counter >= start) {
					lines--;
					DrawString(ride_type->GetString(ride_type->GetTypeName()), (this->current_ride == i) ? TEXT_WHITE : TEXT_BLACK,
							rect.x, rect.y, wid->pos.width);
					rect.y += GetTextHeight();
				}
				counter++;
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
			/// \todo Add picture of rides in RCD.
			break;
	}
}

void RideSelectGui::OnClick(WidgetNumber wid_num, const Point16 &pos)
{
	switch (wid_num) {
		case RSEL_SHOPS:
		case RSEL_GENTLE:
		case RSEL_THRILL:
		case RSEL_WET:
		case RSEL_COASTER:
			if (this->SetNewRideKind(wid_num - RSEL_SHOPS)) this->MarkDirty();
			break;

		case RSEL_LIST: {
			const ScrollbarWidget *sb = this->GetWidget<ScrollbarWidget>(RSEL_SCROLL_LIST);
 			this->SetNewRide(sb->GetClickedRow(pos));
			this->MarkDirty();
			break;
		}

		case RSEL_SELECT: {
			if (this->ride_types[this->current_kind] == 0) return;

			if (this->current_ride != -1) {
				const RideType *ride_type = _rides_manager.GetRideType(this->current_ride);
				if (ride_type == nullptr) return;

				uint16 instance = _rides_manager.GetFreeInstance(ride_type);
				if (instance == INVALID_RIDE_INSTANCE) return;

				RideInstance *ri = _rides_manager.CreateInstance(ride_type, instance);
				assert(this->current_kind == ride_type->kind);
				switch (ride_type->kind) {
					case RTK_SHOP:
					case RTK_GENTLE:
					case RTK_THRILL:
						ShowRideBuildGui(ri);
						break;
					case RTK_COASTER:
						_rides_manager.NewInstanceAdded(instance);
						ShowCoasterManagementGui(ri);
						break;
					default: NOT_REACHED(); // Add cases for more ride types here when they get implemented.
				}
				delete this;
			}
			break;
		}
	}
}

/**
 * Select a kind of ride, update the #current_kind and #current_ride variables.
 * @param new_kind Newly selected kind of ride.
 * @param force Update the variables even if nothing appears to have changed.
 * @return Whether selection was changed.
 */
bool RideSelectGui::SetNewRideKind(int16 new_kind, bool force)
{
	assert(new_kind >= 0 && new_kind < RTK_RIDE_KIND_COUNT);
	if (!force && new_kind == this->current_kind) return false;
	this->current_kind = new_kind;
	this->SetRadioButtonsSelected(_ride_type_select_bar, _ride_type_select_bar[this->current_kind]);
	/* Update the scroll bar with number of items of the ride kind. */
	ScrollbarWidget *sb = this->GetWidget<ScrollbarWidget>(RSEL_SCROLL_LIST);
	sb->SetItemCount(this->ride_types[this->current_kind]);
	this->SetNewRide(0);
	return true;
}

/**
 * Set a new ride in the currently selected kind of rides.
 * @param number Index in the current kind of rides to select.
 */
void RideSelectGui::SetNewRide(int number)
{
	this->current_ride = -1;
	number = std::min(number, this->ride_types[this->current_kind] - 1);
	if (this->ride_types[this->current_kind] > 0) {
		for (int i = 0; i < MAX_NUMBER_OF_RIDE_TYPES; i++) {
			const RideType *ride_type = _rides_manager.GetRideType(i);
			if (ride_type == nullptr || ride_type->kind != this->current_kind) continue;
			if (number-- > 0) continue;

			this->current_ride = i;
			break;
		}
	}
	this->SetWidgetPressed(RSEL_SELECT, false);
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

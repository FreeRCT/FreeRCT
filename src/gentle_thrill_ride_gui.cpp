/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gentle_thrill_ride_gui.cpp %Window for interacting with gentle and thrill rides. */

#include "stdafx.h"
#include "math_func.h"
#include "window.h"
#include "money.h"
#include "language.h"
#include "sprite_store.h"
#include "gentle_thrill_ride_type.h"
#include "entity_gui.h"

/** Window to prompt for removing a gentle/thrill ride. */
class GentleThrillRideRemoveWindow : public EntityRemoveWindow  {
public:
	GentleThrillRideRemoveWindow(GentleThrillRideInstance *si);

	void OnClick(WidgetNumber number, const Point16 &pos) override;
	void SetWidgetStringParameters(WidgetNumber wid_num) const override;

private:
	GentleThrillRideInstance *si; ///< Gentle/Thrill ride instance to remove.
};

/**
 * Constructor of the gentle/thrill ride remove window.
 * @param si Gentle/Thrill ride instance to remove.
 */
GentleThrillRideRemoveWindow::GentleThrillRideRemoveWindow(GentleThrillRideInstance *si) : EntityRemoveWindow(WC_GENTLE_THRILL_RIDE_REMOVE, si->GetIndex())
{
	this->si = si;
}

void GentleThrillRideRemoveWindow::OnClick(WidgetNumber number, const Point16 &pos)
{
	if (number == ERW_YES) {
		delete GetWindowByType(WC_GENTLE_THRILL_RIDE_MANAGER, this->si->GetIndex());
		_rides_manager.DeleteInstance(this->si->GetIndex());
	}
	delete this;
}

void GentleThrillRideRemoveWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	if (wid_num == ERW_MESSAGE) _str_params.SetUint8(1, (uint8 *)this->si->name.get());
}

/**
 * Open a gentle/thrill ride remove remove window for the given ride.
 * @param si ride instance to remove.
 */
void ShowGentleThrillRideRemove(GentleThrillRideInstance *si)
{
	if (HighlightWindowByType(WC_GENTLE_THRILL_RIDE_REMOVE, si->GetIndex())) return;

	new GentleThrillRideRemoveWindow(si);
}

/** Widgets of the gentle/thrill ride management window. */
enum GentleThrillRideManagerWidgets {
	GTRMW_TITLEBAR,
	GTRMW_MONTHLY_COST,
	GTRMW_ENTRANCE_FEE,
	GTRMW_RIDE_OPENED,
	GTRMW_RIDE_OPENED_TEXT,
	GTRMW_RIDE_CLOSED,
	GTRMW_RIDE_CLOSED_TEXT,
	GTRMW_RECOLOUR1,
	GTRMW_RECOLOUR2,
	GTRMW_RECOLOUR3,
	GTRMW_REMOVE,
};

/** Widget parts of the #GentleThrillRideManagerWindow. */
static const WidgetPart _gentle_thrill_ride_manager_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, GTRMW_TITLEBAR, COL_RANGE_DARK_RED), SetData(GUI_GENTLE_THRILL_RIDES_MANAGER_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Intermediate(2, 2),
				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GENTLE_THRILL_RIDES_MANAGER_MONTHLY_COST_TEXT, STR_NULL),
				Widget(WT_RIGHT_TEXT, GTRMW_MONTHLY_COST, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GENTLE_THRILL_RIDES_MANAGER_ENTRANCE_FEE_TEXT, STR_NULL),
				Widget(WT_RIGHT_TEXT, GTRMW_ENTRANCE_FEE, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Intermediate(2, 1),
				Intermediate(2, 2),
					Widget(WT_RADIOBUTTON, GTRMW_RIDE_OPENED, COL_RANGE_DARK_RED), SetPadding(0, 2, 0, 0),
					Widget(WT_LEFT_TEXT, GTRMW_RIDE_OPENED_TEXT, COL_RANGE_DARK_RED), SetData(GUI_GENTLE_THRILL_RIDES_MANAGER_OPENED_TEXT, STR_NULL),
					Widget(WT_RADIOBUTTON, GTRMW_RIDE_CLOSED, COL_RANGE_DARK_RED), SetPadding(0, 2, 0, 0),
					Widget(WT_LEFT_TEXT, GTRMW_RIDE_CLOSED_TEXT, COL_RANGE_DARK_RED), SetData(GUI_GENTLE_THRILL_RIDES_MANAGER_CLOSED_TEXT, STR_NULL),
				Intermediate(1, 3),
					Widget(WT_DROPDOWN_BUTTON, GTRMW_RECOLOUR1, COL_RANGE_DARK_RED), SetData(GENTLE_THRILL_RIDES_DESCRIPTION_RECOLOUR1, STR_NULL), SetPadding(2, 2, 2, 2),
					Widget(WT_DROPDOWN_BUTTON, GTRMW_RECOLOUR2, COL_RANGE_DARK_RED), SetData(GENTLE_THRILL_RIDES_DESCRIPTION_RECOLOUR2, STR_NULL), SetPadding(2, 2, 2, 2),
					Widget(WT_DROPDOWN_BUTTON, GTRMW_RECOLOUR3, COL_RANGE_DARK_RED), SetData(GENTLE_THRILL_RIDES_DESCRIPTION_RECOLOUR3, STR_NULL), SetPadding(2, 2, 2, 2),
					SetResize(1, 0),
			Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
				Widget(WT_TEXT_PUSHBUTTON, GTRMW_REMOVE, COL_RANGE_DARK_RED),
						SetData(GUI_ENTITY_REMOVE, GUI_ENTITY_REMOVE_TOOLTIP),
		EndContainer(),
};

/** GUI window for interacting with a gentle/thrill ride instance. */
class GentleThrillRideManagerWindow : public GuiWindow {
public:
	GentleThrillRideManagerWindow(GentleThrillRideInstance *ri);

	void UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid) override;
	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void OnClick(WidgetNumber wid_num, const Point16 &pos) override;
	void OnChange(ChangeCode code, uint32 parameter) override;

private:
	GentleThrillRideInstance *ride; ///< Gentle/Thrill ride instance getting managed by this window.

	void SetGentleThrillRideToggleButtons();
};

/**
 * Constructor of the gentle/thrill ride management window.
 * @param ri Ride to manage.
 */
GentleThrillRideManagerWindow::GentleThrillRideManagerWindow(GentleThrillRideInstance *ri) : GuiWindow(WC_GENTLE_THRILL_RIDE_MANAGER, ri->GetIndex())
{
	this->ride = ri;
	this->SetRideType(this->ride->GetGentleThrillRideType());
	this->SetupWidgetTree(_gentle_thrill_ride_manager_gui_parts, lengthof(_gentle_thrill_ride_manager_gui_parts));
	this->SetGentleThrillRideToggleButtons();

	for (int i = 0; i < 3; i++) {
		const RecolourEntry &re = this->ride->recolours.entries[i];
		if (!re.IsValid()) this->GetWidget<LeafWidget>(GTRMW_RECOLOUR1 + i)->SetShaded(true);
	}
}

assert_compile(MAX_RECOLOUR >= 3); ///< Check that the 3 recolourings of a gentle/thrill ride fit in the Recolouring::entries array.

/** Update the radio buttons of the window. */
void GentleThrillRideManagerWindow::SetGentleThrillRideToggleButtons()
{
	this->SetWidgetChecked(GTRMW_RIDE_OPENED, this->ride->state == RIS_OPEN);
	this->SetWidgetChecked(GTRMW_RIDE_CLOSED, this->ride->state == RIS_CLOSED);
}

void GentleThrillRideManagerWindow::UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid)
{
}

void GentleThrillRideManagerWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case GTRMW_TITLEBAR:
			_str_params.SetUint8(1, _language.GetText(this->ride->GetKind() == RTK_GENTLE ? GUI_GENTLE_RIDES_MANAGER_TITLE : GUI_THRILL_RIDES_MANAGER_TITLE));
			_str_params.SetUint8(2, this->ride->name.get());
			break;

		case GTRMW_MONTHLY_COST: {
			const FixedRideType *type = this->ride->GetFixedRideType();
			_str_params.SetMoney(1, type->monthly_cost + (this->ride->state == RIS_CLOSED ? Money(0) : type->monthly_open_cost));
		} break;

		case GTRMW_ENTRANCE_FEE:
			_str_params.SetMoney(1, this->ride->GetFixedRideType()->item_cost[0]);
			break;
	}
}

void GentleThrillRideManagerWindow::OnClick(WidgetNumber wid_num, const Point16 &pos)
{
	switch (wid_num) {
		case GTRMW_RIDE_OPENED_TEXT:
		case GTRMW_RIDE_OPENED:
			if (this->ride->state != RIS_OPEN) {
				this->ride->OpenRide();
				this->SetGentleThrillRideToggleButtons();
			}
			break;

		case GTRMW_RIDE_CLOSED_TEXT:
		case GTRMW_RIDE_CLOSED:
			if (this->ride->state != RIS_CLOSED) {
				this->ride->CloseRide();
				this->SetGentleThrillRideToggleButtons();
			}
			break;

		case GTRMW_RECOLOUR1:
		case GTRMW_RECOLOUR2:
		case GTRMW_RECOLOUR3: {
			RecolourEntry *re = &this->ride->recolours.entries[wid_num - GTRMW_RECOLOUR1];
			if (re->IsValid()) {
				this->ShowRecolourDropdown(wid_num, re, COL_RANGE_DARK_RED);
			}
			break;
		}
		case GTRMW_REMOVE:
			ShowGentleThrillRideRemove(this->ride);
			break;
	}
}

void GentleThrillRideManagerWindow::OnChange(ChangeCode code, uint32 parameter)
{
	if (code == CHG_DISPLAY_OLD) this->MarkDirty();
}

/**
 * Open a window to manage a given gentle/thrill ride.
 * @param number Ride to manage.
 */
void ShowGentleThrillRideManagementGui(uint16 number)
{
	if (HighlightWindowByType(WC_GENTLE_THRILL_RIDE_MANAGER, number)) return;

	RideInstance *ri = _rides_manager.GetRideInstance(number);
	if (ri == nullptr || (ri->GetKind() != RTK_GENTLE && ri->GetKind() != RTK_THRILL)) return;

	new GentleThrillRideManagerWindow(static_cast<GentleThrillRideInstance *>(ri));
}

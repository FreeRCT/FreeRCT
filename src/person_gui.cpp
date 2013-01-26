/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file person_gui.cpp %Window for interacting with persons. */

#include "stdafx.h"
#include "math_func.h"
#include "window.h"
#include "person_type.h"
#include "sprite_store.h"
#include "ride_type.h"
#include "person.h"

/** Widgets of the guest info window. */
enum GuestInfoWidgets {
	GIW_TITLEBAR,      ///< Title bar widget.
	GIW_MONEY,         ///< Current amount of money.
	GIW_HAPPINESS,     ///< Happiness level.
	GIW_ITEMS,         ///< List of carried items.
	GIW_HUNGER_LEVEL,  ///< Amount of hunger.
	GIW_THIRST_LEVEL,  ///< Amount of thirst.
	GIW_WASTE_LEVEL,   ///< Amount of food/drink waste.
};

/** Widget parts of the #GuestInfoWindow. */
static const WidgetPart _guest_info_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, GIW_TITLEBAR, COL_RANGE_DARK_RED), SetData(STR_ARG1, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Intermediate(6, 2), SetPadding(2, 2, 2, 2),
				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GUEST_INFO_MONEY, STR_NULL),
				Widget(WT_RIGHT_TEXT, GIW_MONEY, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GUEST_INFO_HAPPINESS, STR_NULL),
				Widget(WT_RIGHT_TEXT, GIW_HAPPINESS, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GUEST_INFO_HUNGER, STR_NULL),
				Widget(WT_RIGHT_TEXT, GIW_HUNGER_LEVEL, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GUEST_INFO_THIRST, STR_NULL),
				Widget(WT_RIGHT_TEXT, GIW_THIRST_LEVEL, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GUEST_INFO_WASTE, STR_NULL),
				Widget(WT_RIGHT_TEXT, GIW_WASTE_LEVEL, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GUEST_INFO_ITEMS, STR_NULL),
				Widget(WT_RIGHT_TEXT, GIW_ITEMS, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
			EndContainer(),
	EndContainer(),
};

/** Gui window for interacting with a guest. */
class GuestInfoWindow : public GuiWindow {
public:
	GuestInfoWindow(const Guest *guest);

	/* virtual */ void SetWidgetStringParameters(WidgetNumber wid_num) const;
	/* virtual */ void OnChange(ChangeCode code, uint32 parameter);

private:
	const Guest *guest; ///< The guest getting looked at by this window.
};

/**
 * Constructor of the guest info window.
 * @param guest #Guest to view.
 */
GuestInfoWindow::GuestInfoWindow(const Guest *guest) : GuiWindow(WC_GUEST_INFO, guest->id)
{
	this->guest = guest;
	this->SetupWidgetTree(_guest_info_gui_parts, lengthof(_guest_info_gui_parts));
}

/* virtual */ void GuestInfoWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case GIW_TITLEBAR:
			_str_params.SetUint8(1, (uint8 *)this->guest->GetName());
			break;

		case GIW_MONEY:
			_str_params.SetMoney(1, this->guest->cash);
			break;

		case GIW_HAPPINESS:
			_str_params.SetNumber(1, this->guest->happiness);
			break;

		case GIW_HUNGER_LEVEL:
			_str_params.SetNumber(1, this->guest->hunger_level);
			break;

		case GIW_THIRST_LEVEL:
			_str_params.SetNumber(1, this->guest->thirst_level);
			break;
		case GIW_WASTE_LEVEL:
			_str_params.SetNumber(1, this->guest->waste);
			break;

		case GIW_ITEMS:
			_str_params.SetStrID(1, (this->guest->has_wrapper ? GUI_ITEM_WRAPPER : GUI_ITEM_NONE));
			break;

		default: break;
	}
}

/* virtual */ void GuestInfoWindow::OnChange(ChangeCode code, uint32 parameter)
{
	if (code == CHG_DISPLAY_OLD) this->MarkDirty();
}

/**
 * Open a window to view a given guest's info.
 * @param person #Person to view.
 */
void ShowGuestInfoGui(const Person *person)
{
	if (HighlightWindowByType(WC_GUEST_INFO, person->id)) return;

	const Guest *guest = dynamic_cast<const Guest *>(person);

	if (guest != NULL) {
		new GuestInfoWindow(guest);
	}
}

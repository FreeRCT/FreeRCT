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
#include "people.h"

/** Widgets of the guest info window. */
enum GuestInfoWidgets {
	GIW_TITLEBAR,     ///< Title bar widget.
	GIW_STATUS,       ///< Status text.
	GIW_MONEY,        ///< Current amount of money.
	GIW_MONEY_SPENT,  ///< Total amount of money spent.
	GIW_HAPPINESS,    ///< Happiness level.
	GIW_ITEMS,        ///< List of carried items.
	GIW_HUNGER_LEVEL, ///< Amount of hunger.
	GIW_THIRST_LEVEL, ///< Amount of thirst.
	GIW_WASTE_LEVEL,  ///< Amount of food/drink waste.
	GIW_NAUSEA,       ///< Nausea level.
};

/** Widget parts of the #GuestInfoWindow. */
static const WidgetPart _guest_info_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, GIW_TITLEBAR, COL_RANGE_DARK_RED), SetData(STR_ARG1, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
		Intermediate(2, 1),
			Widget(WT_CENTERED_TEXT, GIW_STATUS, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
			Intermediate(8, 2), SetPadding(2, 2, 2, 2),
				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GUEST_INFO_MONEY, STR_NULL),
				Widget(WT_RIGHT_TEXT, GIW_MONEY, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GUEST_INFO_MONEY_SPENT, STR_NULL),
				Widget(WT_RIGHT_TEXT, GIW_MONEY_SPENT, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GUEST_INFO_HAPPINESS, STR_NULL),
				Widget(WT_RIGHT_TEXT, GIW_HAPPINESS, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GUEST_INFO_HUNGER, STR_NULL),
				Widget(WT_RIGHT_TEXT, GIW_HUNGER_LEVEL, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GUEST_INFO_THIRST, STR_NULL),
				Widget(WT_RIGHT_TEXT, GIW_THIRST_LEVEL, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GUEST_INFO_WASTE, STR_NULL),
				Widget(WT_RIGHT_TEXT, GIW_WASTE_LEVEL, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GUEST_INFO_NAUSEA, STR_NULL),
				Widget(WT_RIGHT_TEXT, GIW_NAUSEA, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_GUEST_INFO_ITEMS, STR_NULL),
				Widget(WT_RIGHT_TEXT, GIW_ITEMS, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
			EndContainer(),
	EndContainer(),
};

/** GUI window for interacting with a guest. */
class GuestInfoWindow : public GuiWindow {
public:
	GuestInfoWindow(const Guest *guest);

	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void OnChange(ChangeCode code, uint32 parameter) override;

private:
	const Guest *guest; ///< The guest getting looked at by this window.
};

/**
 * Constructor of the guest info window.
 * @param guest #Guest to view.
 */
GuestInfoWindow::GuestInfoWindow(const Guest *guest) : GuiWindow(WC_PERSON_INFO, guest->id)
{
	this->guest = guest;
	this->SetupWidgetTree(_guest_info_gui_parts, lengthof(_guest_info_gui_parts));
}

void GuestInfoWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case GIW_TITLEBAR:
			_str_params.SetText(1, this->guest->GetName());
			break;

		case GIW_STATUS:
			_str_params.SetText(1, this->guest->GetStatus());
			break;

		case GIW_MONEY:
			_str_params.SetMoney(1, this->guest->cash);
			break;

		case GIW_MONEY_SPENT:
			_str_params.SetMoney(1, this->guest->cash_spent);
			break;

		case GIW_NAUSEA:
			_str_params.SetNumber(1, this->guest->nausea);
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

void GuestInfoWindow::OnChange(ChangeCode code, uint32 /* parameter */)
{
	if (code == CHG_DISPLAY_OLD) this->MarkDirty();
}

/** Widgets of the staff info window. */
enum StaffInfoWidgets {
	SIW_TITLEBAR,     ///< Title bar widget.
	SIW_STATUS,       ///< Status text.
	SIW_SALARY,       ///< Salary text.
	SIW_DISMISS,      ///< Dismiss button.
};

/** Widget parts of the #StaffInfoWindow. */
static const WidgetPart _staff_info_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, SIW_TITLEBAR, COL_RANGE_DARK_RED), SetData(STR_ARG1, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Intermediate(3, 1), SetPadding(2, 2, 2, 2),
			Widget(WT_CENTERED_TEXT, SIW_STATUS, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
			Widget(WT_LEFT_TEXT, SIW_SALARY, COL_RANGE_DARK_RED), SetData(GUI_STAFF_SALARY, STR_NULL),
			Widget(WT_TEXT_PUSHBUTTON, SIW_DISMISS, COL_RANGE_DARK_RED), SetData(GUI_STAFF_DISMISS, STR_NULL),
	EndContainer(),
};

/** GUI window for interacting with a staff member. */
class StaffInfoWindow : public GuiWindow {
public:
	StaffInfoWindow(const StaffMember *person);

	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void OnChange(ChangeCode code, uint32 parameter) override;
	void OnClick(WidgetNumber number, const Point16 &pos) override;

private:
	const StaffMember *person;  ///< The person getting looked at by this window.
};

/**
 * Constructor of the staff info window.
 * @param person #StaffMember to view.
 */
StaffInfoWindow::StaffInfoWindow(const StaffMember *person) : GuiWindow(WC_PERSON_INFO, person->id)
{
	this->person = person;
	this->SetupWidgetTree(_staff_info_gui_parts, lengthof(_guest_info_gui_parts));
}

void StaffInfoWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case SIW_TITLEBAR:
			_str_params.SetText(1, this->person->GetName());
			break;

		case SIW_SALARY:
			_str_params.SetMoney(1, StaffMember::SALARY.at(this->person->type));
			break;

		case SIW_STATUS: {
			_str_params.SetText(1, this->person->GetStatus());
			break;
		}

		default: break;
	}
}

void StaffInfoWindow::OnClick(WidgetNumber number, const Point16 & /* pos */)
{
	if (number == SIW_DISMISS) _staff.Dismiss(this->person);  // This also deletes this window.
}

void StaffInfoWindow::OnChange(ChangeCode code, uint32 /* parameter */)
{
	switch (code) {
		case CHG_DISPLAY_OLD:
			this->MarkDirty();
			break;
		case CHG_PERSON_DELETED:
			delete this;
			break;
		default:
			break;
	}
}

/**
 * Open a window to view a given person's info.
 * @param person #Person to view.
 */
void ShowPersonInfoGui(const Person *person)
{
	if (HighlightWindowByType(WC_PERSON_INFO, person->id) != nullptr) return;

	switch (person->type) {
		case PERSON_GUEST:
			new GuestInfoWindow(static_cast<const Guest*>(person));
			break;

		case PERSON_MECHANIC:
		case PERSON_HANDYMAN:
		case PERSON_GUARD:
		case PERSON_ENTERTAINER:
			new StaffInfoWindow(static_cast<const StaffMember*>(person));
			break;

		default: NOT_REACHED();
	}
}

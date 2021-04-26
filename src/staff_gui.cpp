/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file staff_gui.cpp %Window to manage staff. */

#include "stdafx.h"
#include "window.h"
#include "person.h"
#include "people.h"

/**
 * Widget numbers of the staff GUI.
 * @ingroup gui_group
 */
enum StaffWidgets {
	STAFF_GUI_LIST,               ///< List of staff members.
	STAFF_GUI_SCROLL_LIST,        ///< Scrollbar of the list.
	STAFF_CATEGORY_MECHANICS,     ///< Tab for the Mechanics category.
	STAFF_CATEGORY_HANDYMEN,      ///< Tab for the Handymen category.
	STAFF_CATEGORY_GUARDS,        ///< Tab for the Guards category.
	STAFF_CATEGORY_ENTERTAINERS,  ///< Tab for the Entertainers category.
	STAFF_SALARY,                 ///< Salary text.
	STAFF_HIRE,                   ///< Hire button.
};

/**
 * Widget description of the staff GUI.
 * @ingroup gui_group
 */
static const WidgetPart _staff_select_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, COL_RANGE_BLUE), SetData(GUI_STAFF_MANAGEMENT_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_BLUE),
		EndContainer(),
		/* Staff types bar. */
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_BLUE),
			Intermediate(3, 1),
				Intermediate(1, 0),
					Widget(WT_LEFT_FILLER_TAB, INVALID_WIDGET_INDEX,  COL_RANGE_BLUE),
					Widget(WT_TEXT_TAB, STAFF_CATEGORY_MECHANICS,     COL_RANGE_BLUE), SetData(GUI_STAFF_CATEGORY_MECHANICS,    STR_NULL),
					Widget(WT_TEXT_TAB, STAFF_CATEGORY_HANDYMEN,      COL_RANGE_BLUE), SetData(GUI_STAFF_CATEGORY_HANDYMEN,     STR_NULL),
					Widget(WT_TEXT_TAB, STAFF_CATEGORY_GUARDS,        COL_RANGE_BLUE), SetData(GUI_STAFF_CATEGORY_GUARDS,       STR_NULL),
					Widget(WT_TEXT_TAB, STAFF_CATEGORY_ENTERTAINERS,  COL_RANGE_BLUE), SetData(GUI_STAFF_CATEGORY_ENTERTAINERS, STR_NULL),
					Widget(WT_RIGHT_FILLER_TAB, INVALID_WIDGET_INDEX, COL_RANGE_BLUE), SetFill(1,0), SetResize(1, 0),
				EndContainer(),
				Intermediate(1, 3), SetPadding(2, 2, 2, 2),
					Widget(WT_RIGHT_TEXT,      INVALID_WIDGET_INDEX, COL_RANGE_BLUE), SetData(GUI_STAFF_SALARY, STR_NULL),
					Widget(WT_LEFT_TEXT,       STAFF_SALARY,         COL_RANGE_BLUE), SetData(STR_ARG1, STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, STAFF_HIRE,           COL_RANGE_BLUE), SetData(GUI_STAFF_HIRE, STR_NULL),
				/* List of staff. */
				Widget(WT_TAB_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_BLUE),
					Intermediate(1, 2),
						Widget(WT_EMPTY, STAFF_GUI_LIST, COL_RANGE_BLUE), SetFill(1, 1), SetResize(1, 1), SetMinimalSize(100, 200),
						Widget(WT_VERT_SCROLLBAR, STAFF_GUI_SCROLL_LIST, COL_RANGE_BLUE),
	EndContainer(),
};

/** GUI window for interacting with a guest. */
class StaffManagementGui : public GuiWindow {
public:
	StaffManagementGui();

	void SelectTab(PersonType t);
	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
	void OnClick(WidgetNumber wid, const Point16 &pos) override;
	void SetWidgetStringParameters(WidgetNumber wid_num) const override;

	PersonType selected;   ///< Currently selected tab.
};

/**
 * Constructor of the guest info window.
 * @param guest #Guest to view.
 */
StaffManagementGui::StaffManagementGui() : GuiWindow(WC_STAFF, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_staff_select_gui_parts, lengthof(_staff_select_gui_parts));
	this->SetScrolledWidget(STAFF_GUI_LIST, STAFF_GUI_SCROLL_LIST);
	this->SelectTab(PERSON_MECHANIC);
}

/**
 * Select a tab.
 * @param p Type of person to select.
 */
void StaffManagementGui::SelectTab(const PersonType p)
{
	this->selected = p;
	this->GetWidget<ScrollbarWidget>(STAFF_GUI_SCROLL_LIST)->SetItemCount(std::min<uint16>(1u, _staff.CountMechanics()));

	this->SetWidgetPressed(STAFF_CATEGORY_MECHANICS,    p == PERSON_MECHANIC);
	this->SetWidgetPressed(STAFF_CATEGORY_HANDYMEN,     p == PERSON_HANDYMAN);
	this->SetWidgetPressed(STAFF_CATEGORY_GUARDS,       p == PERSON_GUARD);
	this->SetWidgetPressed(STAFF_CATEGORY_ENTERTAINERS, p == PERSON_ENTERTAINER);
}

void StaffManagementGui::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case STAFF_SALARY:
			switch (this->selected) {
				case PERSON_MECHANIC:    _str_params.SetMoney(1, Mechanic::SALARY);    break;
				case PERSON_HANDYMAN:    _str_params.SetMoney(1, Handyman::SALARY);    break;
				case PERSON_GUARD:       _str_params.SetMoney(1, Guard::SALARY);       break;
				case PERSON_ENTERTAINER: _str_params.SetMoney(1, Entertainer::SALARY); break;
				default: break;
			}
			break;
	}
}

void StaffManagementGui::OnClick(const WidgetNumber number, const Point16 &pos)
{
	switch (number) {
		case STAFF_HIRE:
			switch (this->selected) {
				case PERSON_MECHANIC:
					_staff.HireMechanic();
					break;
				case PERSON_HANDYMAN:
					_staff.HireHandyman();
					break;
				case PERSON_GUARD:
					_staff.HireGuard();
					break;
				case PERSON_ENTERTAINER:
					_staff.HireEntertainer();
					break;
				default:
					NOT_REACHED();
			}
			break;

		case STAFF_CATEGORY_MECHANICS:    this->SelectTab(PERSON_MECHANIC);    break;
		case STAFF_CATEGORY_HANDYMEN:     this->SelectTab(PERSON_HANDYMAN);    break;
		case STAFF_CATEGORY_GUARDS:       this->SelectTab(PERSON_GUARD);       break;
		case STAFF_CATEGORY_ENTERTAINERS: this->SelectTab(PERSON_ENTERTAINER); break;

		default:
			break;
	}
}

void StaffManagementGui::DrawWidget(const WidgetNumber wid_num, const BaseWidget *wid) const
{
	if (wid_num != STAFF_GUI_LIST) return GuiWindow::DrawWidget(wid_num, wid);

	// NOCOM
}

/** Open a window to view and manage the park's staff. */
void ShowStaffManagementGui()
{
	if (HighlightWindowByType(WC_STAFF, ALL_WINDOWS_OF_TYPE) != nullptr) return;
	new StaffManagementGui;
}

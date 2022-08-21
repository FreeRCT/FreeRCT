/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file finances_gui.cpp Finance management gui code. */

#include "stdafx.h"
#include "window.h"
#include "viewport.h"
#include "terraform.h"
#include "sprite_store.h"
#include "finances.h"
#include "gui_sprites.h"

static const Money LOAN_STEP_SIZE = 100000;   ///< Amount of loan taken or paid back when clicking the loan buttons once.

/**
  * GUI for viewing and managing financial information.
  * @ingroup gui_group
  */
class FinancesGui : public GuiWindow {
public:
	FinancesGui();

	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void OnDraw(MouseModeSelector *selector) override;
	void OnClick(WidgetNumber widget, const Point16 &pos) override;
	void UpdateButtons();
};

/**
  * Widget numbers of the finances GUI.
  * @ingroup gui_group
  */
enum FinancesWidgets {
	FIN_RIDE_CONSTRUCTION_VALUE,
	FIN_RIDE_RUNNING_VALUE,
	FIN_LAND_PURCHASE_VALUE,
	FIN_LANDSCAPING_VALUE,
	FIN_PARK_TICKETS_VALUE,
	FIN_RIDE_TICKETS_VALUE,
	FIN_SHOP_SALES_VALUE,
	FIN_SHOP_STOCK_VALUE,
	FIN_FOOD_SALES_VALUE,
	FIN_FOOD_STOCK_VALUE,
	FIN_STAFF_WAGES_VALUE,
	FIN_MARKETING_VALUE,
	FIN_RESEARCH_VALUE,
	FIN_LOAN_INTEREST_VALUE,
	FIN_TOTAL_VALUE,
	FIN_CASH,            ///< Current cash text field.
	FIN_MAX_LOAN,        ///< Maximum loan text field.
	FIN_INTEREST,        ///< Annual loan interest text field.
	FIN_CURRENT_LOAN,    ///< Current loan text field.
	FIN_INCREASE_LOAN,   ///< Increase loan button.
	FIN_DECREASE_LOAN,   ///< Decrease loan button.
};

/**
 * Helper macro to easily generate each category row of the finances window.
 * Intended for use in #_finances_gui_parts.
 * @param id the name of the category row.
 */
#define FINANCES_ROW(id) \
	Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_GREY), \
		SetPadding(2, 10, 2, 2), \
		SetData(GUI_FINANCES_ ## id ## _TEXT, STR_NULL), \
	Widget(WT_RIGHT_TEXT, FIN_ ## id ## _VALUE, COL_RANGE_GREY), \
		SetMinimalSize(100, 10), \
		SetPadding(2, 10, 2, 10), \
		SetData(STR_ARG1, STR_NULL)

/** Widget parts of the #FinancesGui window. */
static const WidgetPart _finances_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, COL_RANGE_GREY), SetData(GUI_FINANCES_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_GREY),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_GREY),
			Intermediate(1, 2), SetPadding(2, 2, 2, 2),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_GREY),
			Intermediate(4, 2), SetPadding(2, 2, 2, 2),
				Widget(WT_LEFT_TEXT,  INVALID_WIDGET_INDEX, COL_RANGE_GREY), SetData(GUI_FINANCES_CASH,          STR_NULL),
				Widget(WT_RIGHT_TEXT, FIN_CASH,             COL_RANGE_GREY), SetData(STR_ARG1,                   STR_NULL),
				Widget(WT_LEFT_TEXT,  INVALID_WIDGET_INDEX, COL_RANGE_GREY), SetData(GUI_FINANCES_MAX_LOAN,      STR_NULL),
				Widget(WT_RIGHT_TEXT, FIN_MAX_LOAN,         COL_RANGE_GREY), SetData(STR_ARG1,                   STR_NULL),
				Widget(WT_LEFT_TEXT,  INVALID_WIDGET_INDEX, COL_RANGE_GREY), SetData(GUI_FINANCES_LOAN_INTEREST, STR_NULL),
				Widget(WT_RIGHT_TEXT, FIN_INTEREST,         COL_RANGE_GREY), SetData(STR_ARG1,                   STR_NULL),
				Widget(WT_LEFT_TEXT,  INVALID_WIDGET_INDEX, COL_RANGE_GREY), SetData(GUI_FINANCES_CURRENT_LOAN,  STR_NULL),
				Intermediate(1, 3), SetPadding(2, 2, 2, 2),
					Widget(WT_TEXT_PUSHBUTTON, FIN_DECREASE_LOAN, COL_RANGE_GREY), SetData(GUI_DECREASE_BUTTON, STR_NULL),
					Widget(WT_CENTERED_TEXT,   FIN_CURRENT_LOAN,  COL_RANGE_GREY), SetData(STR_ARG1,            STR_NULL),
					Widget(WT_TEXT_PUSHBUTTON, FIN_INCREASE_LOAN, COL_RANGE_GREY), SetData(GUI_INCREASE_BUTTON, STR_NULL),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_GREY),
			Intermediate(15, 2), SetPadding(2, 2, 2, 2),
				FINANCES_ROW(RIDE_CONSTRUCTION),
				FINANCES_ROW(RIDE_RUNNING),
				FINANCES_ROW(LAND_PURCHASE),
				FINANCES_ROW(LANDSCAPING),
				FINANCES_ROW(PARK_TICKETS),
				FINANCES_ROW(RIDE_TICKETS),
				FINANCES_ROW(SHOP_SALES),
				FINANCES_ROW(SHOP_STOCK),
				FINANCES_ROW(FOOD_SALES),
				FINANCES_ROW(FOOD_STOCK),
				FINANCES_ROW(STAFF_WAGES),
				FINANCES_ROW(MARKETING),
				FINANCES_ROW(RESEARCH),
				FINANCES_ROW(LOAN_INTEREST),
				FINANCES_ROW(TOTAL),
			EndContainer(),
	EndContainer(),
};
#undef FINANCES_ROW

FinancesGui::FinancesGui() : GuiWindow(WC_FINANCES, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_finances_gui_parts, lengthof(_finances_gui_parts));
}

void FinancesGui::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	const Finances &f = _finances_manager.GetFinances();
	switch (wid_num) {
		case FIN_RIDE_CONSTRUCTION_VALUE: _str_params.SetMoney(1, f.ride_construct); break;
		case FIN_RIDE_RUNNING_VALUE:      _str_params.SetMoney(1, f.ride_running);   break;
		case FIN_LAND_PURCHASE_VALUE:     _str_params.SetMoney(1, f.land_purchase);  break;
		case FIN_LANDSCAPING_VALUE:       _str_params.SetMoney(1, f.landscaping);    break;
		case FIN_PARK_TICKETS_VALUE:      _str_params.SetMoney(1, f.park_tickets);   break;
		case FIN_RIDE_TICKETS_VALUE:      _str_params.SetMoney(1, f.ride_tickets);   break;
		case FIN_SHOP_SALES_VALUE:        _str_params.SetMoney(1, f.shop_sales);     break;
		case FIN_SHOP_STOCK_VALUE:        _str_params.SetMoney(1, f.shop_stock);     break;
		case FIN_FOOD_SALES_VALUE:        _str_params.SetMoney(1, f.food_sales);     break;
		case FIN_FOOD_STOCK_VALUE:        _str_params.SetMoney(1, f.food_stock);     break;
		case FIN_STAFF_WAGES_VALUE:       _str_params.SetMoney(1, f.staff_wages);    break;
		case FIN_MARKETING_VALUE:         _str_params.SetMoney(1, f.marketing);      break;
		case FIN_RESEARCH_VALUE:          _str_params.SetMoney(1, f.research);       break;
		case FIN_LOAN_INTEREST_VALUE:     _str_params.SetMoney(1, f.loan_interest);  break;
		case FIN_TOTAL_VALUE:             _str_params.SetMoney(1, f.GetTotal());     break;

		case FIN_CASH:         _str_params.SetMoney(1, _finances_manager.GetCash()); break;
		case FIN_CURRENT_LOAN: _str_params.SetMoney(1, _finances_manager.GetLoan()); break;
		case FIN_MAX_LOAN:     _str_params.SetMoney(1, _scenario.max_loan         ); break;
		case FIN_INTEREST: {
			static char buffer[512];
			snprintf(buffer, lengthof(buffer), _language.GetSgText(GUI_FINANCES_LOAN_INTEREST_VALUE).c_str(), _scenario.interest * 0.1f);
			_str_params.SetText(1, buffer);
			break;
		}

		default: break;
	}
}

/** Recompute whether the loan buttons are enabled. */
void FinancesGui::UpdateButtons()
{
	this->GetWidget<LeafWidget>(FIN_INCREASE_LOAN)->SetShaded(_finances_manager.GetLoan() >= _scenario.max_loan);
	this->GetWidget<LeafWidget>(FIN_DECREASE_LOAN)->SetShaded(_finances_manager.GetLoan() <= 0 ||
			_finances_manager.GetCash() < std::min(LOAN_STEP_SIZE, _finances_manager.GetLoan()));
}

void FinancesGui::OnDraw(MouseModeSelector *selector)
{
	this->UpdateButtons();
	GuiWindow::OnDraw(selector);
}

void FinancesGui::OnClick(WidgetNumber widget, [[maybe_unused]] const Point16 &pos)
{
	switch (widget) {
		case FIN_INCREASE_LOAN:
			_finances_manager.TakeLoan(std::min(LOAN_STEP_SIZE, _scenario.max_loan - _finances_manager.GetLoan()));
			break;
		case FIN_DECREASE_LOAN:
			_finances_manager.RepayLoan(std::min(LOAN_STEP_SIZE, std::min(_finances_manager.GetLoan(), _finances_manager.GetCash())));
			break;

		default: break;
	}
}

/** Open the finances window (or if it is already opened, highlight and raise it). */
void ShowFinancesGui()
{
	if (HighlightWindowByType(WC_FINANCES, ALL_WINDOWS_OF_TYPE) != nullptr) return;
	new FinancesGui;
}

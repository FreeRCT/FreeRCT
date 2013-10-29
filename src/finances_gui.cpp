/* $Id$ */

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

/**
  * GUI for viewing and managing financial information.
  * @ingroup gui_group
  */
class FinancesGui : public GuiWindow {
public:
	FinancesGui();
	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
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
};

/**
 * Helper macro to easily generate each category row of the finances window.
 * Intended for use in #_finances_gui_parts.
 * @param id the name of the category row.
 */
#define FINANCES_ROW(id) \
	Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, 0), \
		SetPadding(2, 10, 2, 2), \
		SetData(GUI_FINANCES_ ## id ## _TEXT, STR_NULL), \
	Widget(WT_RIGHT_TEXT, FIN_ ## id ## _VALUE, 0), \
		SetMinimalSize(100, 10), \
		SetPadding(2, 10, 2, 10), \
		SetData(STR_ARG1, STR_NULL)

/** Widget parts of the #FinancesGui window. */
static const WidgetPart _finances_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, 0), SetData(GUI_FINANCES_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, 0),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, 0),
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
	}
}


/** Open the finances window (or if it is already opened, highlight and raise it). */
void ShowFinancesGui()
{
	if (HighlightWindowByType(WC_FINANCES, ALL_WINDOWS_OF_TYPE)) return;
	new FinancesGui;
}

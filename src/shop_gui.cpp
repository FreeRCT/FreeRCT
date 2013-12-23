/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file shop_gui.cpp %Window for inter-acting with shops. */

#include "stdafx.h"
#include "math_func.h"
#include "window.h"
#include "money.h"
#include "language.h"
#include "sprite_store.h"
#include "shop_type.h"

/** Widgets of the shop management window. */
enum ShopManagerWidgets {
	SMW_TITLEBAR,     ///< Title bar widget.
	SMW_ITEM1_HEAD,   ///< Name of the first item being sold.
	SMW_ITEM2_HEAD,   ///< Name of the second item being sold.
	SMW_ITEM1_COST,   ///< Cost of the first item.
	SMW_ITEM2_COST,   ///< Cost of the second item.
	SMW_ITEM1_SELL,   ///< Selling price of the first item.
	SMW_ITEM2_SELL,   ///< Selling price of the second item.
	SMW_ITEM1_PROFIT, ///< Profit of a first item.
	SMW_ITEM2_PROFIT, ///< Profit of a second item.
	SMW_ITEM1_COUNT,  ///< Number of first items sold in total.
	SMW_ITEM2_COUNT,  ///< Number of second items sold in total.
	SMW_SELL_PROFIT,  ///< Total selling profit.
	SMW_SHOP_COST,    ///< Shop maintenance/personnel costs.
	SMW_TOTAL_PROFIT, ///< Total shop profit.
	SMW_SHOP_OPENED,  ///< Radio button of 'shop is open'.
	SMW_SHOP_CLOSED,  ///< Radio button of 'shop is closed'.
};

/** Widget parts of the #ShopManagerWindow. */
static const WidgetPart _shop_manager_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, SMW_TITLEBAR, COL_RANGE_DARK_RED), SetData(GUI_SHOP_MANAGER_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Intermediate(8, 3), SetPadding(2, 2, 2, 2),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
				Widget(WT_CENTERED_TEXT, SMW_ITEM1_HEAD, COL_RANGE_DARK_RED), SetData(SHOPS_NAME_ITEM1, STR_NULL), SetMinimalSize(60, 10),
				Widget(WT_CENTERED_TEXT, SMW_ITEM2_HEAD, COL_RANGE_DARK_RED), SetData(SHOPS_NAME_ITEM2, STR_NULL), SetMinimalSize(60, 10),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_SHOP_MANAGER_COST_PRICE_TEXT, STR_NULL),
				Widget(WT_RIGHT_TEXT, SMW_ITEM1_COST, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
				Widget(WT_RIGHT_TEXT, SMW_ITEM2_COST, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_SHOP_MANAGER_SELLING_PRICE_TEXT, STR_NULL),
				Widget(WT_RIGHT_TEXT, SMW_ITEM1_SELL, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
				Widget(WT_RIGHT_TEXT, SMW_ITEM2_SELL, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_SHOP_MANAGER_ITEM_PROFIT_TEXT, STR_NULL),
				Widget(WT_RIGHT_TEXT, SMW_ITEM1_PROFIT, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
				Widget(WT_RIGHT_TEXT, SMW_ITEM2_PROFIT, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_SHOP_MANAGER_ITEMS_SOLD_TEXT, STR_NULL),
				Widget(WT_RIGHT_TEXT, SMW_ITEM1_COUNT, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
				Widget(WT_RIGHT_TEXT, SMW_ITEM2_COUNT, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_SHOP_MANAGER_SELL_PROFIT_TEXT, STR_NULL),
				Widget(WT_RIGHT_TEXT, SMW_SELL_PROFIT, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_SHOP_MANAGER_SHOP_COST_TEXT, STR_NULL),
				Widget(WT_RIGHT_TEXT, SMW_SHOP_COST, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),

				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_SHOP_MANAGER_TOTAL_PROFIT_TEXT, STR_NULL),
				Widget(WT_RIGHT_TEXT, SMW_TOTAL_PROFIT, COL_RANGE_DARK_RED), SetData(STR_ARG1, STR_NULL),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Intermediate(2,2),
				Widget(WT_RADIOBUTTON, SMW_SHOP_OPENED, COL_RANGE_DARK_RED), SetPadding(0, 2, 0, 0),
				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_SHOP_MANAGER_OPENED_TEXT, STR_NULL),
				Widget(WT_RADIOBUTTON, SMW_SHOP_CLOSED, COL_RANGE_DARK_RED), SetPadding(0, 2, 0, 0),
				Widget(WT_LEFT_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED), SetData(GUI_SHOP_MANAGER_CLOSED_TEXT, STR_NULL),
		EndContainer(),
};

/** GUI window for interacting with a shop instance. */
class ShopManagerWindow : public GuiWindow {
public:
	ShopManagerWindow(ShopInstance *ri);

	void UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid) override;
	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void OnClick(WidgetNumber wid_num) override;
	void OnChange(ChangeCode code, uint32 parameter) override;

private:
	ShopInstance *shop; ///< Shop instance getting managed by this window.

	void SetShopToggleButtons();
};

/**
 * Constructor of the shop management window.
 * @param ri Shop to manage.
 */
ShopManagerWindow::ShopManagerWindow(ShopInstance *ri) : GuiWindow(WC_SHOP_MANAGER, ri->GetIndex())
{
	this->shop = ri;
	this->SetShopType(this->shop->GetShopType());
	this->SetupWidgetTree(_shop_manager_gui_parts, lengthof(_shop_manager_gui_parts));
	this->SetShopToggleButtons();
}

/** Update the radio buttons of the window. */
void ShopManagerWindow::SetShopToggleButtons()
{
	this->SetWidgetChecked(SMW_SHOP_OPENED, this->shop->state == RIS_OPEN);
	this->SetWidgetChecked(SMW_SHOP_CLOSED, this->shop->state == RIS_CLOSED);
}

void ShopManagerWindow::UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid)
{
	if (wid_num != SMW_ITEM1_COUNT && wid_num != SMW_ITEM2_COUNT) return;

	int w, h;
	_video->GetNumberRangeSize(0, 100000, &w, &h);
	wid->min_x = std::max(wid->min_x, (uint16)w);
	wid->min_y = std::max(wid->min_y, (uint16)h);
}

void ShopManagerWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case SMW_TITLEBAR:
			_str_params.SetUint8(1, this->shop->name);
			break;

		case SMW_ITEM1_COST:
		case SMW_ITEM2_COST: {
			const ShopType *st = this->shop->GetShopType();
			_str_params.SetMoney(1, st->item_cost[wid_num - SMW_ITEM1_COST]);
			break;
		}

		case SMW_ITEM1_SELL:
		case SMW_ITEM2_SELL:
			_str_params.SetMoney(1, this->shop->GetSaleItemPrice(wid_num - SMW_ITEM1_SELL));
			break;

		case SMW_ITEM1_PROFIT:
		case SMW_ITEM2_PROFIT: {
			const ShopType *st = this->shop->GetShopType();
			const Money &cost = st->item_cost[wid_num - SMW_ITEM1_PROFIT];
			const Money &sell = this->shop->GetSaleItemPrice(wid_num - SMW_ITEM1_PROFIT);
			_str_params.SetMoney(1, sell - cost);
			break;
		}

		case SMW_ITEM1_COUNT:
		case SMW_ITEM2_COUNT:
			_str_params.SetNumber(1, this->shop->item_count[wid_num - SMW_ITEM1_COUNT]);
			break;

		case SMW_SELL_PROFIT:
			_str_params.SetMoney(1, this->shop->total_sell_profit);
			break;

		case SMW_SHOP_COST:
			_str_params.SetMoney(1, this->shop->total_profit - this->shop->total_sell_profit);
			break;

		case SMW_TOTAL_PROFIT:
			_str_params.SetMoney(1, this->shop->total_profit);
			break;
	}
}

void ShopManagerWindow::OnClick(WidgetNumber wid_num)
{
	switch (wid_num) {
		case SMW_SHOP_OPENED:
			if (this->shop->state != RIS_OPEN) {
				this->shop->OpenRide();
				this->SetShopToggleButtons();
			}
			break;

		case SMW_SHOP_CLOSED:
			if (this->shop->state != RIS_CLOSED) {
				this->shop->CloseRide();
				this->SetShopToggleButtons();
			}
			break;
	}
}

void ShopManagerWindow::OnChange(ChangeCode code, uint32 parameter)
{
	if (code == CHG_DISPLAY_OLD) this->MarkDirty();
}

/**
 * Open a window to manage a given shop.
 * @param number Shop to manage.
 */
void ShowShopManagementGui(uint16 number)
{
	if (HighlightWindowByType(WC_SHOP_MANAGER, number)) return;

	RideInstance *ri = _rides_manager.GetRideInstance(number);
	if (ri == nullptr || ri->GetKind() != RTK_SHOP) return;

	new ShopManagerWindow(static_cast<ShopInstance *>(ri));
}

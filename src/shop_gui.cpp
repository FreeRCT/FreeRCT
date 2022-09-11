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
#include "entity_gui.h"
#include "finances.h"
#include "viewport.h"

/** Window to prompt for removing a shop. */
class ShopRemoveWindow : public EntityRemoveWindow  {
public:
	ShopRemoveWindow(ShopInstance *si);

	void OnClick(WidgetNumber number, const Point16 &pos) override;
	void SetWidgetStringParameters(WidgetNumber wid_num) const override;

private:
	ShopInstance *si; ///< Shop instance to remove.
};

/**
 * Constructor of the shop remove window.
 * @param si Shop instance to remove.
 */
ShopRemoveWindow::ShopRemoveWindow(ShopInstance *instance) : EntityRemoveWindow(WC_SHOP_REMOVE, instance->GetIndex()), si(instance)
{
}

void ShopRemoveWindow::OnClick(WidgetNumber number, [[maybe_unused]] const Point16 &pos)
{
	if (number == ERW_YES) {
		const Money cost = this->si->ComputeReturnCost();
		_finances_manager.PayRideConstruct(cost);
		_window_manager.GetViewport()->AddFloatawayMoneyAmount(cost, this->si->RepresentativeLocation());

		delete GetWindowByType(WC_SHOP_MANAGER, this->si->GetIndex());

		_rides_manager.DeleteInstance(this->si->GetIndex());
	}
	delete this;
}

void ShopRemoveWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case ERW_MESSAGE:
			_str_params.SetText(1, this->si->name);
			break;
		case ERW_COST:
			_str_params.SetMoney(1, -this->si->ComputeReturnCost());
			break;
		default:
			break;
	}
}

/**
 * Open a shop remove window for the given shop.
 * @param si Shop instance to remove.
 */
void ShowShopRemove(ShopInstance *si)
{
	if (HighlightWindowByType(WC_SHOP_REMOVE, si->GetIndex()) != nullptr) return;

	new ShopRemoveWindow(si);
}

/** Widgets of the shop management window. */
enum ShopManagerWidgets {
	SMW_TITLEBAR,         ///< Title bar widget.
	SMW_ITEM1_HEAD,       ///< Name of the first item being sold.
	SMW_ITEM2_HEAD,       ///< Name of the second item being sold.
	SMW_ITEM1_COST,       ///< Cost of the first item.
	SMW_ITEM2_COST,       ///< Cost of the second item.
	SMW_ITEM1_SELL,       ///< Selling price of the first item.
	SMW_ITEM2_SELL,       ///< Selling price of the second item.
	SMW_ITEM1_PROFIT,     ///< Profit of a first item.
	SMW_ITEM2_PROFIT,     ///< Profit of a second item.
	SMW_ITEM1_COUNT,      ///< Number of first items sold in total.
	SMW_ITEM2_COUNT,      ///< Number of second items sold in total.
	SMW_SELL_PROFIT,      ///< Total selling profit.
	SMW_SHOP_COST,        ///< Shop maintenance/personnel costs.
	SMW_TOTAL_PROFIT,     ///< Total shop profit.
	SMW_OPEN_SHOP_PANEL,  ///< Open shop button.
	SMW_CLOSE_SHOP_PANEL, ///< Close shop button.
	SMW_OPEN_SHOP_LIGHT,  ///< Open shop light.
	SMW_CLOSE_SHOP_LIGHT, ///< Close shop light.
	SMW_RECOLOUR1,        ///< First recolour dropdown.
	SMW_RECOLOUR2,        ///< Second recolour dropdown.
	SMW_RECOLOUR3,        ///< Third recolour dropdown.
	SMW_REMOVE,           ///< Remove button widget.
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
			Intermediate(1, 3), SetEqualSize(false, true),
				Intermediate(1, 3),
					Widget(WT_DROPDOWN_BUTTON, SMW_RECOLOUR1, COL_RANGE_DARK_RED), SetData(SHOPS_DESCRIPTION_RECOLOUR1, STR_NULL), SetPadding(2, 2, 2, 2),
					Widget(WT_DROPDOWN_BUTTON, SMW_RECOLOUR2, COL_RANGE_DARK_RED), SetData(SHOPS_DESCRIPTION_RECOLOUR2, STR_NULL), SetPadding(2, 2, 2, 2),
					Widget(WT_DROPDOWN_BUTTON, SMW_RECOLOUR3, COL_RANGE_DARK_RED), SetData(SHOPS_DESCRIPTION_RECOLOUR3, STR_NULL), SetPadding(2, 2, 2, 2),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0), SetMinimalSize(1, 40),
				Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
					Intermediate(0, 1),
						Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(0, 1),
						Widget(WT_PANEL, SMW_CLOSE_SHOP_PANEL, COL_RANGE_DARK_RED),
							Widget(WT_RADIOBUTTON, SMW_CLOSE_SHOP_LIGHT, COL_RANGE_RED  ), SetPadding(0, 2, 0, 0),
						Widget(WT_PANEL, SMW_OPEN_SHOP_PANEL, COL_RANGE_DARK_RED),
							Widget(WT_RADIOBUTTON, SMW_OPEN_SHOP_LIGHT , COL_RANGE_GREEN), SetPadding(0, 2, 0, 0),
						Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(0, 1),
				EndContainer(),

			Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
				Widget(WT_TEXT_PUSHBUTTON, SMW_REMOVE, COL_RANGE_DARK_RED),
						SetData(GUI_ENTITY_REMOVE, GUI_ENTITY_REMOVE_TOOLTIP),
		EndContainer(),
};

/** GUI window for interacting with a shop instance. */
class ShopManagerWindow : public GuiWindow {
public:
	ShopManagerWindow(ShopInstance *ri);

	void UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid) override;
	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void OnClick(WidgetNumber wid_num, const Point16 &pos) override;

private:
	ShopInstance *shop; ///< Shop instance getting managed by this window.

	void SetShopToggleButtons();
};

/**
 * Constructor of the shop management window.
 * @param ri Shop to manage.
 */
ShopManagerWindow::ShopManagerWindow(ShopInstance *ri) : GuiWindow(WC_SHOP_MANAGER, ri->GetIndex()), shop(ri)
{
	this->SetRideType(this->shop->GetShopType());
	this->SetupWidgetTree(_shop_manager_gui_parts, lengthof(_shop_manager_gui_parts));
	this->SetShopToggleButtons();

	for (int i = 0; i < MAX_RIDE_RECOLOURS; i++) {
		const RecolourEntry &re = this->shop->recolours.entries[i];
		if (!re.IsValid()) this->GetWidget<LeafWidget>(SMW_RECOLOUR1 + i)->SetShaded(true);
	}
}

assert_compile(MAX_RECOLOUR >= MAX_RIDE_RECOLOURS); ///< Check that the 3 recolourings of a shop fit in the Recolouring::entries array.

/** Update the radio buttons of the window. */
void ShopManagerWindow::SetShopToggleButtons()
{
	this->GetWidget<LeafWidget>(SMW_OPEN_SHOP_LIGHT )->shift = this->shop->state == RIS_OPEN   ? GS_LIGHT : GS_NIGHT;
	this->GetWidget<LeafWidget>(SMW_CLOSE_SHOP_LIGHT)->shift = this->shop->state == RIS_CLOSED ? GS_LIGHT : GS_NIGHT;
}

void ShopManagerWindow::UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid)
{
	if (wid_num != SMW_ITEM1_COUNT && wid_num != SMW_ITEM2_COUNT) return;

	int w, h;
	_video.GetNumberRangeSize(0, 100000, &w, &h);
	wid->min_x = std::max(wid->min_x, (uint16)w);
	wid->min_y = std::max(wid->min_y, (uint16)h);
}

void ShopManagerWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case SMW_TITLEBAR:
			_str_params.SetText(1, this->shop->name);
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

void ShopManagerWindow::OnClick(WidgetNumber wid_num, [[maybe_unused]] const Point16 &pos)
{
	switch (wid_num) {
		case SMW_OPEN_SHOP_LIGHT:
		case SMW_OPEN_SHOP_PANEL:
			if (this->shop->state != RIS_OPEN) {
				this->shop->OpenRide();
				this->SetShopToggleButtons();
			}
			break;

		case SMW_CLOSE_SHOP_LIGHT:
		case SMW_CLOSE_SHOP_PANEL:
			if (this->shop->state != RIS_CLOSED) {
				this->shop->CloseRide();
				this->SetShopToggleButtons();
			}
			break;

		case SMW_RECOLOUR1:
		case SMW_RECOLOUR2:
		case SMW_RECOLOUR3: {
			RecolourEntry *re = &this->shop->recolours.entries[wid_num - SMW_RECOLOUR1];
			if (re->IsValid()) {
				this->ShowRecolourDropdown(wid_num, re, COL_RANGE_DARK_RED);
			}
			break;
		}
		case SMW_REMOVE:
			ShowShopRemove(this->shop);
			break;
	}
}

/**
 * Open a window to manage a given shop.
 * @param number Shop to manage.
 */
void ShowShopManagementGui(uint16 number)
{
	if (HighlightWindowByType(WC_SHOP_MANAGER, number) != nullptr) return;

	RideInstance *ri = _rides_manager.GetRideInstance(number);
	if (ri == nullptr || ri->GetKind() != RTK_SHOP) return;

	new ShopManagerWindow(static_cast<ShopInstance *>(ri));
}

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
	/*SMW_TITLEBAR,         ///< Title bar widget.
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
	SMW_SHOP_OPENED,      ///< Radio button of 'shop is open'.
	SMW_SHOP_OPENED_TEXT, ///< 'shop is open' text label.
	SMW_SHOP_CLOSED,      ///< Radio button of 'shop is closed'.
	SMW_SHOP_CLOSED_TEXT, ///< 'shop is closed' text label.
	SMW_RECOLOUR1,        ///< First recolour dropdown.
	SMW_RECOLOUR2,        ///< Second recolour dropdown.
	SMW_RECOLOUR3,        ///< Third recolour dropdown.
	SMW_REMOVE,           ///< Remove button widget.*/
	// NOCOM
	GTRMW_TITLEBAR,
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
	// NOCOM
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, GTRMW_TITLEBAR, COL_RANGE_DARK_RED), SetData(GUI_GENTLE_THRILL_RIDES_MANAGER_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
			Intermediate(2, 1),
				Intermediate(2, 2),
					Widget(WT_RADIOBUTTON, GTRMW_RIDE_OPENED, COL_RANGE_DARK_RED), SetPadding(0, 2, 0, 0),
					Widget(WT_LEFT_TEXT, GTRMW_RIDE_OPENED_TEXT, COL_RANGE_DARK_RED), SetData(GUI_GENTLE_THRILL_RIDES_MANAGER_OPENED_TEXT, STR_NULL),
					Widget(WT_RADIOBUTTON, GTRMW_RIDE_CLOSED, COL_RANGE_DARK_RED), SetPadding(0, 2, 0, 0),
					Widget(WT_LEFT_TEXT, GTRMW_RIDE_CLOSED_TEXT, COL_RANGE_DARK_RED), SetData(GUI_GENTLE_THRILL_RIDES_MANAGER_CLOSED_TEXT, STR_NULL),
				Intermediate(1, 4),
					Widget(WT_DROPDOWN_BUTTON, GTRMW_RECOLOUR1, COL_RANGE_DARK_RED), SetData(GENTLE_THRILL_RIDES_DESCRIPTION_RECOLOUR1, STR_NULL), SetPadding(2, 2, 2, 2),
					Widget(WT_DROPDOWN_BUTTON, GTRMW_RECOLOUR2, COL_RANGE_DARK_RED), SetData(GENTLE_THRILL_RIDES_DESCRIPTION_RECOLOUR2, STR_NULL), SetPadding(2, 2, 2, 2),
					Widget(WT_DROPDOWN_BUTTON, GTRMW_RECOLOUR3, COL_RANGE_DARK_RED), SetData(GENTLE_THRILL_RIDES_DESCRIPTION_RECOLOUR3, STR_NULL), SetPadding(2, 2, 2, 2),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetResize(1, 0),
			Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
				Widget(WT_TEXT_PUSHBUTTON, GTRMW_REMOVE, COL_RANGE_DARK_RED),
						SetData(GUI_ENTITY_REMOVE, GUI_ENTITY_REMOVE_TOOLTIP),
		EndContainer(),
	
	/*Intermediate(0, 1),
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
			Intermediate(2, 1),
				Intermediate(2, 2),
					Widget(WT_RADIOBUTTON, SMW_SHOP_OPENED, COL_RANGE_DARK_RED), SetPadding(0, 2, 0, 0),
					Widget(WT_LEFT_TEXT, SMW_SHOP_OPENED_TEXT, COL_RANGE_DARK_RED), SetData(GUI_SHOP_MANAGER_OPENED_TEXT, STR_NULL),
					Widget(WT_RADIOBUTTON, SMW_SHOP_CLOSED, COL_RANGE_DARK_RED), SetPadding(0, 2, 0, 0),
					Widget(WT_LEFT_TEXT, SMW_SHOP_CLOSED_TEXT, COL_RANGE_DARK_RED), SetData(GUI_SHOP_MANAGER_CLOSED_TEXT, STR_NULL),
				Intermediate(1, 4),
					Widget(WT_DROPDOWN_BUTTON, SMW_RECOLOUR1, COL_RANGE_DARK_RED), SetData(SHOPS_DESCRIPTION_RECOLOUR1, STR_NULL), SetPadding(2, 2, 2, 2),
					Widget(WT_DROPDOWN_BUTTON, SMW_RECOLOUR2, COL_RANGE_DARK_RED), SetData(SHOPS_DESCRIPTION_RECOLOUR2, STR_NULL), SetPadding(2, 2, 2, 2),
					Widget(WT_DROPDOWN_BUTTON, SMW_RECOLOUR3, COL_RANGE_DARK_RED), SetData(SHOPS_DESCRIPTION_RECOLOUR3, STR_NULL), SetPadding(2, 2, 2, 2),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetResize(1, 0),
			Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_DARK_RED),
				Widget(WT_TEXT_PUSHBUTTON, SMW_REMOVE, COL_RANGE_DARK_RED),
						SetData(GUI_ENTITY_REMOVE, GUI_ENTITY_REMOVE_TOOLTIP),
		EndContainer(),*/
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
	// NOCOM
	/* if (wid_num != SMW_ITEM1_COUNT && wid_num != SMW_ITEM2_COUNT) return;

	int w, h;
	_video.GetNumberRangeSize(0, 100000, &w, &h);
	wid->min_x = std::max(wid->min_x, (uint16)w);
	wid->min_y = std::max(wid->min_y, (uint16)h); */
}

void GentleThrillRideManagerWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	// NOCOM
	switch (wid_num) {
		case GTRMW_TITLEBAR:
			_str_params.SetUint8(1, _language.GetText(this->ride->GetKind() == RTK_GENTLE ? GUI_GENTLE_RIDES_MANAGER_TITLE : GUI_THRILL_RIDES_MANAGER_TITLE));
			_str_params.SetUint8(2, this->ride->name.get());
			break;

		/* case SMW_ITEM1_COST:
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
			break; */
	}
}

void GentleThrillRideManagerWindow::OnClick(WidgetNumber wid_num, const Point16 &pos)
{
	// NOCOM
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

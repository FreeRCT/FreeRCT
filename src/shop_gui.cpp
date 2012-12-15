/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file shop_gui.cpp %Window for inter-acting with shops. */

#include "stdafx.h"
#include "window.h"
#include "language.h"
#include "sprite_store.h"
#include "ride_type.h"

/** Widgets of the shop management window. */
enum ShopManagerWidgets {
	SMW_TITLEBAR, ///< Title bar widget.
};

static const WidgetPart _shop_manager_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, SMW_TITLEBAR, COL_RANGE_DARK_GREEN), SetData(GUI_SHOP_MANAGER_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_DARK_GREEN),
		EndContainer(),
};

class ShopManagerWindow : public GuiWindow {
public:
	ShopManagerWindow(RideInstance *shop);

	/* virtual */ void SetWidgetStringParameters(WidgetNumber wid_num) const;

private:
	RideInstance *shop; ///< Shop instance getting managed by this window.
};

/**
 * Constructor of the shop management window.
 * @param ri Shop to manage.
 */
ShopManagerWindow::ShopManagerWindow(RideInstance *ri) : GuiWindow(WC_SHOP_MANAGER)
{
	this->shop = ri;
	this->SetShopType(this->shop->type);
	this->SetupWidgetTree(_shop_manager_gui_parts, lengthof(_shop_manager_gui_parts));
}

/* virtual */ void ShopManagerWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case SMW_TITLEBAR:
			_str_params.SetUint8(1, this->shop->name);
			break;
	}
}

/**
 * Open a window to manage a given shop.
 * @param ri Shop to manage.
 * @todo Enable having several windows of the same window type, by adding a sub-number.
 */
void ShowShopManagementGui(uint16 number)
{
	Window *w = GetWindowByType(WC_SHOP_MANAGER);
	if (w != NULL) _manager.DeleteWindow(w);

	RideInstance *ri = _rides_manager.GetRideInstance(number);
	if (ri == NULL) return;

	new ShopManagerWindow(ri);
}

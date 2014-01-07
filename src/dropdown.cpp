/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file dropdown.cpp Implementation of the dropdown widget. */

#include "stdafx.h"
#include "window.h"

/**
 * Defines a Dropdown menu item.
 * @param strid StringID to store.
 * @pre String parameters must be set for this string.
 */
DropdownItem::DropdownItem(StringID strid)
{
	DrawText(strid, this->str, lengthof(this->str));
}

/**
 * Dropdown menu window.
 * @todo Scrollbar.
 */
class DropdownMenuWindow : GuiWindow {
public:
	DropdownMenuWindow(WindowTypes parent_type, WindowNumber parent_num, int parent_btn, const DropdownList &items, const Point16 &pos, uint16 min_width, int initial_select, ColourRange colour);

	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
	void OnClick(WidgetNumber number) override;
	void UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid) override;

	WindowTypes parent_type; ///< Parent window type.
	WindowNumber parent_num; ///< Parent window number.
	int parent_btn;          ///< Object the dropdown originated from. Usually a #WidgetNumber.
	DropdownList items;      ///< List of strings to display.
	Rectangle16 size;        ///< Size of the window.
	int selected_index;      ///< Currently selected item in list.

private:
	void SetDropdownSize(const Point16 &pos, uint16 min_width);
};

/** Widgets of the dropdown window. */
enum DropdownMenuWidgets {
	DD_ITEMS, ///< Panel showing the dropdown items.
};

/**
 * Widget parts of a Dropdown window.
 * @ingroup gui_group
 */
static const WidgetPart _dropdown_widgets[] = {
	Widget(WT_PANEL, DD_ITEMS, COL_RANGE_GREY),
};

/**
 * Dropdown menu constructor.
 * @param parent_type Type of the parent window.
 * @param parent_num Number of the parent window.
 * @param items List of #DropdownItem items to display.
 * @param pos Initial (top left) position of the window.
 * @param min_width Minimum width of the window.
 * @param initial_select Item in list that is selected initially.
 * @param colour Colour of the window.
 */
DropdownMenuWindow::DropdownMenuWindow(WindowTypes parent_type, WindowNumber parent_num, int parent_btn, const DropdownList &items, const Point16 &pos, uint16 min_width, int initial_select, ColourRange colour)
		: GuiWindow(WC_DROPDOWN, ALL_WINDOWS_OF_TYPE), parent_type(parent_type), parent_num(parent_num), parent_btn(parent_btn), items(items), selected_index(initial_select)
{
	this->SetDropdownSize(pos, min_width);
	this->SetupWidgetTree(_dropdown_widgets, lengthof(_dropdown_widgets));

	this->GetWidget<BackgroundWidget>(DD_ITEMS)->colour = colour;
	this->SetPosition(this->size.base.x, this->size.base.y);
}

/**
 * Setup the size of the dropdown window.
 * @param pos Initial position of the window (top left).
 * @param min_width Minimum width of the window.
 */
void DropdownMenuWindow::SetDropdownSize(const Point16 &pos, uint16 min_width)
{
	this->size = {pos.x, pos.y, min_width, 0};
	int max_width = this->size.width;
	for (auto const &item : items) {
		int w, unused;
		_video->GetTextSize(item.str, &w, &unused);
		max_width = std::max(max_width, w + 2);
		this->size.height += GetTextHeight();
	}
	this->size.width = max_width;
}

void DropdownMenuWindow::DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const
{
	if (wid_num != DD_ITEMS) return;

	int y = this->rect.base.y; // wid->pos is relative to the window
	int it = 0;
	for (auto const &item : this->items) {
		if (it == this->selected_index) {
			Rectangle32 r = {this->rect.base.x, y, static_cast<uint>(wid->pos.width - 1), static_cast<uint>(GetTextHeight())};
			_video->FillSurface(GetColourRangeBase(COL_RANGE_GREY) + 7, r);
		}
		_video->BlitText(item.str, TEXT_WHITE, this->rect.base.x, y, wid->pos.width);

		y += GetTextHeight();
		it++;
	}
}

void DropdownMenuWindow::UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid)
{
	if (wid_num != DD_ITEMS) return;

	wid->min_x = this->size.width;
	wid->min_y = this->size.height;
}

void DropdownMenuWindow::OnClick(WidgetNumber number)
{
	if (number != DD_ITEMS) return;

	int index = this->mouse_pos.y / GetTextHeight();
	if (index >= (int)this->items.size()) return;

	int send = this->parent_btn << 16 | index;
	NotifyChange(this->parent_type, this->parent_num, CHG_DROPDOWN_RESULT, send);

	_manager.DeleteWindow(this);
}

/**
 * Shows a Dropdown Menu.
 * @param widnum Associated dropdown button.
 * @param items List of items to show in the list.
 * @param selected_index Currently selected item in the list.
 * @param colour Requested colour of dropdown.
 */
void GuiWindow::ShowDropdownMenu(WidgetNumber widnum, const DropdownList &items, int selected_index, ColourRange colour)
{
	Window *w = GetWindowByType(WC_DROPDOWN, ALL_WINDOWS_OF_TYPE);
	if (w != nullptr) _manager.DeleteWindow(w);

	DataWidget *wid = this->GetWidget<DataWidget>(widnum);
	assert(wid->wtype == WT_DROPDOWN_BUTTON);
	if (colour == COL_RANGE_INVALID) colour = wid->colour;

	/* Calculate top-left position of window */
	Point16 pos = {static_cast<int16>(GetWidgetScreenX(wid)), static_cast<int16>(GetWidgetScreenY(wid) + wid->pos.height)};

	new DropdownMenuWindow(this->wtype, this->wnumber, widnum, items, pos, wid->min_x, selected_index, colour);
}

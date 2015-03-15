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
#include "palette.h"
#include "bitmath.h"

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
class DropdownMenuWindow : public GuiWindow {
public:
	DropdownMenuWindow(WindowTypes parent_type, WindowNumber parent_num, int parent_btn, const DropdownList &items, const Point16 &pos, uint16 min_width, int initial_select, ColourRange colour);

	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
	void OnClick(WidgetNumber number, const Point16 &pos) override;
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
 * @param parent_btn Unique number within the parent (to differentiate between different dropdowns from the same parent).
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
		_video.GetTextSize(item.str, &w, &unused);
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
			_video.FillRectangle(r, _palette[GetColourRangeBase(COL_RANGE_GREY) + 7]);
		}
		_video.BlitText(item.str, MakeRGBA(255, 255, 255, OPAQUE), this->rect.base.x, y, wid->pos.width);

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

void DropdownMenuWindow::OnClick(WidgetNumber number, const Point16 &pos)
{
	if (number != DD_ITEMS) return;

	int index = this->mouse_pos.y / GetTextHeight();
	if (index >= (int)this->items.size()) return;

	int send = this->parent_btn << 16 | index;
	NotifyChange(this->parent_type, this->parent_num, CHG_DROPDOWN_RESULT, send);

	delete this;
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
	if (w != nullptr) delete w;

	DataWidget *wid = this->GetWidget<DataWidget>(widnum);
	assert(wid->wtype == WT_DROPDOWN_BUTTON);
	if (colour == COL_RANGE_INVALID) colour = wid->colour;

	/* Calculate top-left position of window */
	Point16 pos(static_cast<int16>(GetWidgetScreenX(wid)), static_cast<int16>(GetWidgetScreenY(wid) + wid->pos.height));

	new DropdownMenuWindow(this->wtype, this->wnumber, widnum, items, pos, wid->min_x, selected_index, colour);
}

/** Dropdown for picking a colour to use for recolouring. */
class RecolourDropdownWindow : public GuiWindow {
public:
	RecolourDropdownWindow(WindowTypes parent_type, WindowNumber parent_num, int parent_btn, const Point16 &pos, ColourRange colour, RecolourEntry *entry);

	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
	void OnClick(WidgetNumber widget, const Point16 &pos) override;

	WindowTypes parent_type; ///< Parent window type.
	WindowNumber parent_num; ///< Parent window number.
	int parent_btn;          ///< Object the dropdown originated from. Usually a #WidgetNumber.
	RecolourEntry *entry;    ///< Entry being changed.
};

/** Widgets of the #RecolourDropdownWindow. */
enum RecolourDropdownWidgets {
	RD_PANEL,
	RD_BUTTON_00, ///< Button for selecting colour range 0.
	RD_BUTTON_01, ///< Button for selecting colour range 1.
	RD_BUTTON_02, ///< Button for selecting colour range 2.
	RD_BUTTON_03, ///< Button for selecting colour range 3.
	RD_BUTTON_04, ///< Button for selecting colour range 4.
	RD_BUTTON_05, ///< Button for selecting colour range 5.
	RD_BUTTON_06, ///< Button for selecting colour range 6.
	RD_BUTTON_07, ///< Button for selecting colour range 7.
	RD_BUTTON_08, ///< Button for selecting colour range 8.
	RD_BUTTON_09, ///< Button for selecting colour range 9.
	RD_BUTTON_10, ///< Button for selecting colour range 10.
	RD_BUTTON_11, ///< Button for selecting colour range 11.
	RD_BUTTON_12, ///< Button for selecting colour range 12.
	RD_BUTTON_13, ///< Button for selecting colour range 13.
	RD_BUTTON_14, ///< Button for selecting colour range 14.
	RD_BUTTON_15, ///< Button for selecting colour range 15.
	RD_BUTTON_16, ///< Button for selecting colour range 16.
	RD_BUTTON_17, ///< Button for selecting colour range 17.
};

/** Widget description of the recolour dropdown. */
static const WidgetPart _recolour_dropdown_widgets[] = {
	Widget(WT_PANEL, RD_PANEL, COL_RANGE_GREY),
		Intermediate(6, 3),
			Widget(WT_PANEL, RD_BUTTON_00, COL_RANGE_GREY),            SetResize(0, 0), SetMinimalSize(10, 6), SetPadding(1, 1, 1, 1), EndContainer(),
			Widget(WT_PANEL, RD_BUTTON_01, COL_RANGE_GREEN_BROWN),     SetResize(0, 0), SetMinimalSize(10, 6), SetPadding(1, 1, 1, 1), EndContainer(),
			Widget(WT_PANEL, RD_BUTTON_02, COL_RANGE_ORANGE_BROWN),    SetResize(0, 0), SetMinimalSize(10, 6), SetPadding(1, 1, 1, 1), EndContainer(),
			Widget(WT_PANEL, RD_BUTTON_03, COL_RANGE_YELLOW),          SetResize(0, 0), SetMinimalSize(10, 6), SetPadding(1, 1, 1, 1), EndContainer(),
			Widget(WT_PANEL, RD_BUTTON_04, COL_RANGE_DARK_RED),        SetResize(0, 0), SetMinimalSize(10, 6), SetPadding(1, 1, 1, 1), EndContainer(),
			Widget(WT_PANEL, RD_BUTTON_05, COL_RANGE_DARK_GREEN),      SetResize(0, 0), SetMinimalSize(10, 6), SetPadding(1, 1, 1, 1), EndContainer(),
			Widget(WT_PANEL, RD_BUTTON_06, COL_RANGE_LIGHT_GREEN),     SetResize(0, 0), SetMinimalSize(10, 6), SetPadding(1, 1, 1, 1), EndContainer(),
			Widget(WT_PANEL, RD_BUTTON_07, COL_RANGE_GREEN),           SetResize(0, 0), SetMinimalSize(10, 6), SetPadding(1, 1, 1, 1), EndContainer(),
			Widget(WT_PANEL, RD_BUTTON_08, COL_RANGE_PINK_BROWN),      SetResize(0, 0), SetMinimalSize(10, 6), SetPadding(1, 1, 1, 1), EndContainer(),
			Widget(WT_PANEL, RD_BUTTON_09, COL_RANGE_DARK_PURPLE),     SetResize(0, 0), SetMinimalSize(10, 6), SetPadding(1, 1, 1, 1), EndContainer(),
			Widget(WT_PANEL, RD_BUTTON_10, COL_RANGE_BLUE),            SetResize(0, 0), SetMinimalSize(10, 6), SetPadding(1, 1, 1, 1), EndContainer(),
			Widget(WT_PANEL, RD_BUTTON_11, COL_RANGE_DARK_JADE_GREEN), SetResize(0, 0), SetMinimalSize(10, 6), SetPadding(1, 1, 1, 1), EndContainer(),
			Widget(WT_PANEL, RD_BUTTON_12, COL_RANGE_PURPLE),          SetResize(0, 0), SetMinimalSize(10, 6), SetPadding(1, 1, 1, 1), EndContainer(),
			Widget(WT_PANEL, RD_BUTTON_13, COL_RANGE_RED),             SetResize(0, 0), SetMinimalSize(10, 6), SetPadding(1, 1, 1, 1), EndContainer(),
			Widget(WT_PANEL, RD_BUTTON_14, COL_RANGE_ORANGE),          SetResize(0, 0), SetMinimalSize(10, 6), SetPadding(1, 1, 1, 1), EndContainer(),
			Widget(WT_PANEL, RD_BUTTON_15, COL_RANGE_SEA_GREEN),       SetResize(0, 0), SetMinimalSize(10, 6), SetPadding(1, 1, 1, 1), EndContainer(),
			Widget(WT_PANEL, RD_BUTTON_16, COL_RANGE_PINK),            SetResize(0, 0), SetMinimalSize(10, 6), SetPadding(1, 1, 1, 1), EndContainer(),
			Widget(WT_PANEL, RD_BUTTON_17, COL_RANGE_BROWN),           SetResize(0, 0), SetMinimalSize(10, 6), SetPadding(1, 1, 1, 1), EndContainer(),
};

/**
 * Dropdown window for a recolour selection.
 * @param parent_type Type of the parent window.
 * @param parent_num Number of the parent window.
 * @param parent_btn Unique number within the parent (to differentiate between different dropdowns from the same parent).
 * @param pos Initial position of the window (top left).
 * @param colour Requested colour of dropdown.
 * @param entry Recolour entry being changed (will be changed when clicked on a selectable different colour).
 */
RecolourDropdownWindow::RecolourDropdownWindow(WindowTypes parent_type, WindowNumber parent_num, int parent_btn, const Point16 &pos, ColourRange colour, RecolourEntry *entry)
		: GuiWindow(WC_DROPDOWN, ALL_WINDOWS_OF_TYPE), parent_type(parent_type), parent_num(parent_num), parent_btn(parent_btn), entry(entry)
{
	this->SetupWidgetTree(_recolour_dropdown_widgets, lengthof(_recolour_dropdown_widgets));
	this->GetWidget<BackgroundWidget>(RD_PANEL)->colour = colour;
	this->SetPosition(pos.x, pos.y);

	/** Disable the entries that cannot be chosen. */
	for (int i = 0; i < COL_RANGE_COUNT; i++) {
		if (GB(this->entry->dest_set, i, 1) == 0) this->GetWidget<LeafWidget>(RD_BUTTON_00 + i)->SetShaded(true);
	}
}

void RecolourDropdownWindow::OnClick(WidgetNumber widget, const Point16 &pos)
{
	if (widget >= RD_BUTTON_00 && widget <= RD_BUTTON_17) {
		if (GB(this->entry->dest_set, widget - RD_BUTTON_00, 1) == 0) return;

		if (this->entry->dest != widget - RD_BUTTON_00) {
			this->entry->dest = static_cast<ColourRange>(widget - RD_BUTTON_00);
			_video.MarkDisplayDirty();
		}

		delete this;
	}
}

void RecolourDropdownWindow::DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const
{
	if (GB(this->entry->dest_set, wid_num - RD_BUTTON_00, 1) == 1) return;
	Rectangle32 rect = {this->GetWidgetScreenX(wid), this->GetWidgetScreenY(wid), wid->pos.width, wid->pos.height};
	OverlayShaded(rect);
}

/**
 * Open a recolour dropdown from widget \a widnum.
 * @param widnum Associated dropdown button.
 * @param entry Recolour entry being changed (will be changed when clicked on a selectable different colour).
 * @param colour Requested colour of dropdown.
 */
void GuiWindow::ShowRecolourDropdown(WidgetNumber widnum, RecolourEntry *entry, ColourRange colour)
{
	Window *w = GetWindowByType(WC_DROPDOWN, ALL_WINDOWS_OF_TYPE);
	if (w != nullptr) delete w;

	DataWidget *wid = this->GetWidget<DataWidget>(widnum);
	assert(wid->wtype == WT_DROPDOWN_BUTTON);
	if (colour == COL_RANGE_INVALID) colour = wid->colour;

	/* Calculate top-left position of window */
	Point16 pos(static_cast<int16>(GetWidgetScreenX(wid)), static_cast<int16>(GetWidgetScreenY(wid) + wid->pos.height));

	new RecolourDropdownWindow(this->wtype, this->wnumber, widnum, pos, colour, entry);
}


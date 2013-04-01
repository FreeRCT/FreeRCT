/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file bottom_toolbar_gui.cpp Bottom toolbar window code. */

#include "stdafx.h"
#include "freerct.h"
#include "window.h"
#include "video.h"
#include "language.h"
#include "finances.h"
#include "dates.h"
#include "math_func.h"

/**
 * Main toolbar.
 * @ingroup gui_group
 */
class BottomToolbarWindow : public GuiWindow {
public:
	BottomToolbarWindow();

	/* virtual */ Point32 OnInitialPosition();
	/* virtual */ void SetWidgetStringParameters(WidgetNumber wid_num) const;
	/* virtual */ void OnChange(ChangeCode code, uint32 parameter);
	/* virtual */ void UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid);
};

/**
 * Widget numbers of the bottom toolbar Gui.
 * @ingroup gui_group
 */
enum ToolbarGuiWidgets {
	BTB_STATUS,  ///< Status panel containing cash and rating readout.
	BTB_SPACING, ///< Status panel containing nothing (yet).
	BTB_DATE,    ///< Status panel containing date.
};

static const uint32 BOTTOM_BAR_HEIGHT = 35;     ///< Minimum Y-coord size of the bottom toolbar (BTB) panel.
static const uint32 BOTTOM_BAR_POSITION_X = 75; ///< Separation of the toolbar from the edge of the window.

/**
 * Widget parts of the bottom toolbar Gui.
 * @ingroup gui_group
 * @todo Left/Right Padding get ignored when drawing text widgets
 */
static const WidgetPart _bottom_toolbar_widgets[] = {
	Intermediate(0, 1),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_BROWN),
			Intermediate(1, 0), SetPadding(0, 3, 0, 3),
				Widget(WT_LEFT_TEXT, BTB_STATUS, COL_RANGE_BROWN),
					SetMinimalSize(1, BOTTOM_BAR_HEIGHT), // Temp X value
					SetPadding(3, 0, 30, 0),
					SetData(STR_ARG1, STR_NULL),
				Widget(WT_EMPTY, BTB_SPACING, COL_RANGE_BROWN),
					SetMinimalSize(1, BOTTOM_BAR_HEIGHT), // Temp X value
				Widget(WT_RIGHT_TEXT, BTB_DATE, COL_RANGE_BROWN),
					SetMinimalSize(1, BOTTOM_BAR_HEIGHT), // Temp X value
					SetPadding(3, 0, 30, 0),
					SetData(STR_ARG1, STR_NULL),
			EndContainer(),
	EndContainer(),
};


BottomToolbarWindow::BottomToolbarWindow() : GuiWindow(WC_BOTTOM_TOOLBAR, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_bottom_toolbar_widgets, lengthof(_bottom_toolbar_widgets));
}

/* virtual */ Point32 BottomToolbarWindow::OnInitialPosition()
{
	static Point32 pt;
	pt.x = BOTTOM_BAR_POSITION_X;
	pt.y = _video->GetYSize() - BOTTOM_BAR_HEIGHT;
	return pt;
}

/* virtual */ void BottomToolbarWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case BTB_STATUS:
			_finances_manager.CashToStrParams();
			break;

		case BTB_DATE:
			_str_params.SetDate(1, _date);
			break;
	}
}

/* virtual */ void BottomToolbarWindow::OnChange(ChangeCode code, uint32 parameter)
{
	if (code == CHG_DISPLAY_OLD) this->MarkDirty();
}

/* virtual */ void BottomToolbarWindow::UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid)
{
	static const int64 LARGE_MONEY_AMOUNT = -9999999999; ///< -99,999,999.99 = Maximum amount of money that is worth handling for now.
	Point32 p;
	p.x = 0;
	p.y = 0;

	switch (wid_num) {
		case BTB_STATUS:
			p = GetMoneyStringSize(LARGE_MONEY_AMOUNT);
			break;

		case BTB_SPACING: {
			Point32 money = GetMoneyStringSize(LARGE_MONEY_AMOUNT);
			Point32 date = GetMaxDateSize();
			p.x = _video->GetXSize() - (2 * BOTTOM_BAR_POSITION_X) - money.x - date.x;
			p.y = BOTTOM_BAR_HEIGHT;
			break;
		}

		case BTB_DATE:
			p = GetMaxDateSize();
			break;
	}

	wid->min_x = max(wid->min_x, (uint16)p.x);
	wid->min_y = max(wid->min_y, (uint16)p.y);
}

/**
 * Open the bottom toolbar window.
 * @ingroup gui_group
 */
void ShowBottomToolbar()
{
	new BottomToolbarWindow();
}

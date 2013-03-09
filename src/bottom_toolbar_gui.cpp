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

/**
 * Main toolbar.
 * @ingroup gui_group
 */
class BottomToolbarWindow : public GuiWindow {
public:
	BottomToolbarWindow();

	virtual Point32 OnInitialPosition();
	virtual void SetWidgetStringParameters(WidgetNumber wid_num) const ;
};

/**
 * Widget numbers of the bottom toolbar Gui.
 * @ingroup gui_group
 */
enum ToolbarGuiWidgets {
	BTB_STATUS, ///< Status panel containing cash and rating readout.
};


static const uint32 BTB_SIZE_X = 75;  ///< Minimum X-coord size of the bottom toolbar (BTB) panel.
static const uint32 BTB_SIZE_Y = 35;  ///< Minimum Y-coord size of the bottom toolbar (BTB) panel.

/**
 * Widget parts of the bottom toolbar Gui.
 * @ingroup gui_group
 */
static const WidgetPart _bottom_toolbar_widgets[] = {
	Intermediate(0, 1),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_BROWN),
			Widget(WT_RIGHT_TEXT, BTB_STATUS, COL_RANGE_BROWN),
				SetMinimalSize(BTB_SIZE_X, BTB_SIZE_Y),
				SetPadding(3, 3, 30, 0),
				SetData(STR_ARG1, STR_NULL),
	EndContainer(),
};


BottomToolbarWindow::BottomToolbarWindow() : GuiWindow(WC_BOTTOM_TOOLBAR, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_bottom_toolbar_widgets, lengthof(_bottom_toolbar_widgets));
}

/* virtual */ Point32 BottomToolbarWindow::OnInitialPosition()
{
	static Point32 pt;
	pt.x = 0;
	pt.y = _video->GetYSize() - BTB_SIZE_Y;
	return pt;
}

/* virtual */ void BottomToolbarWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case BTB_STATUS:
			_finances_manager.CashToStrParams();
			break;
	}
}

/**
 * Open the bottom toolbar window.
 * @ingroup gui_group
 */
void ShowBottomToolbar()
{
	new BottomToolbarWindow();
}


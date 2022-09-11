/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file entity_gui.cpp Entity windows. */

#include "entity_gui.h"

/** Widget parts of the #EntityRemoveWindow. */
static const WidgetPart _entity_remove_widgets[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, COL_RANGE_RED), SetData(GUI_ENTITY_REMOVE_CAPTION, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_RED),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_RED),
			Intermediate(3, 1),
				Widget(WT_CENTERED_TEXT, ERW_MESSAGE, COL_RANGE_RED),
						SetData(GUI_ENTITY_REMOVE_MESSAGE, STR_NULL), SetPadding(5, 5, 5, 5), SetMinimalSize(300, 1),
				Widget(WT_CENTERED_TEXT, ERW_COST, COL_RANGE_RED),
						SetData(GUI_ENTITY_REMOVE_COST, STR_NULL), SetPadding(5, 5, 5, 5),
				Intermediate(1, 5), SetPadding(0, 0, 3, 0),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
					Widget(WT_TEXT_PUSHBUTTON, ERW_NO, COL_RANGE_YELLOW), SetData(GUI_ENTITY_REMOVE_NO, STR_NULL),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
					Widget(WT_TEXT_PUSHBUTTON, ERW_YES, COL_RANGE_YELLOW), SetData(GUI_ENTITY_REMOVE_YES, STR_NULL),
					Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
	EndContainer(),
};

Point32 EntityRemoveWindow::OnInitialPosition()
{
	Point32 pt;
	pt.x = (_video.Width() - this->rect.width ) / 2;
	pt.y = (_video.Height() - this->rect.height) / 2;
	return pt;
}

/**
 * Constructor of the entity remove window.
 * @param wtype %Window type (for finding a window in the stack).
 * @param wnumber Number of the window within the \a wtype.
 */
EntityRemoveWindow::EntityRemoveWindow(WindowTypes wtype, WindowNumber wnum) : GuiWindow(wtype, wnum)
{
	this->SetupWidgetTree(_entity_remove_widgets, lengthof(_entity_remove_widgets));
}

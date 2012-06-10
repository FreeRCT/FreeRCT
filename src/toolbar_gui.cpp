/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file toolbar_gui.cpp Main toolbar window code. */

#include "stdafx.h"
#include "freerct.h"
#include "window.h"
#include "video.h"

#include "table/strings.h"

void ShowQuitProgram();

/**
 * Main toolbar.
 * @ingroup gui_group
 */
class ToolbarWindow : public GuiWindow {
public:
	ToolbarWindow();

	virtual Point32 OnInitialPosition();
	virtual void OnClick(WidgetNumber number);
};

/**
 * Widget numbers of the toolbar Gui.
 * @ingroup gui_group
 */
enum ToolbarGuiWidgets {
	TB_GUI_QUIT,  ///< Quit program button.
	TB_GUI_PATHS, ///< Build paths button.
	TB_GUI_SAVE,  ///< Save game button.
	TB_GUI_LOAD,  ///< Load game button.
};

/**
 * Widget parts of the toolbar Gui.
 * @ingroup gui_group
 */
static const WidgetPart _toolbar_widgets[] = {
	Intermediate(1, 0),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_QUIT,  COL_RANGE_BROWN), SetData(STR_TOOLBAR_GUI_QUIT,  STR_TOOLBAR_GUI_TOOLTIP_QUIT_PROGRAM),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_PATHS, COL_RANGE_BROWN), SetData(STR_TOOLBAR_GUI_PATHS, STR_TOOLBAR_GUI_TOOLTIP_BUILD_PATHS),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_SAVE,  COL_RANGE_BROWN), SetData(STR_TOOLBAR_GUI_SAVE,  STR_TOOLBAR_GUI_TOOLTIP_SAVE_GAME),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_LOAD,  COL_RANGE_BROWN), SetData(STR_TOOLBAR_GUI_LOAD,  STR_TOOLBAR_GUI_TOOLTIP_LOAD_GAME),
	EndContainer(),
};


ToolbarWindow::ToolbarWindow() : GuiWindow(WC_TOOLBAR)
{
	this->SetupWidgetTree(_toolbar_widgets, lengthof(_toolbar_widgets));
}

/* virtual */ Point32 ToolbarWindow::OnInitialPosition()
{
	static const Point32 pt = {10, 0};
	return pt;
}

/* virtual */ void ToolbarWindow::OnClick(WidgetNumber number)
{
	switch (number) {
		case TB_GUI_QUIT:
			ShowQuitProgram();
			break;

		case TB_GUI_PATHS:
			ShowPathBuildGui();
			break;

		case TB_GUI_SAVE:
			// XXX Implement me
			break;

		case TB_GUI_LOAD:
			// XXX Implement me
			break;
	}
}

/**
 * Open the main toolbar window.
 * @ingroup gui_group
 */
void ShowToolbar()
{
	new ToolbarWindow();
}


class QuitProgramWindow : public GuiWindow {
public:
	QuitProgramWindow();

	virtual Point32 OnInitialPosition();
	virtual void OnClick(WidgetNumber number);
};

/**
 * Widget numbers of the quit-program window.
 * @ingroup gui_group
 */
enum QuitProgramWidgets {
	QP_MESSAGE, ///< Displayed message.
	QP_YES,     ///< 'yes' button.
	QP_NO,      ///< 'no' button.
};

/** Window definition of the quit program gui. */
static const WidgetPart _quit_program_widgets[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, COL_RANGE_RED), SetData(STR_QUIT_CAPTION, STR_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_RED),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_RED),
			Intermediate(2,0),
				Widget(WT_CENTERED_TEXT, QP_MESSAGE, COL_RANGE_RED),
						SetData(STR_QUIT_MESSAGE, STR_NULL), SetPadding(5, 5, 5, 5),
			EndContainer(),
			Intermediate(1, 5), SetPadding(0, 0, 3, 0),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
				Widget(WT_TEXT_PUSHBUTTON, QP_NO, COL_RANGE_YELLOW), SetData(STR_QUIT_NO, STR_NULL),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
				Widget(WT_TEXT_PUSHBUTTON, QP_YES, COL_RANGE_YELLOW), SetData(STR_QUIT_YES, STR_NULL),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
			EndContainer(),
};

/* virtual */ Point32 QuitProgramWindow::OnInitialPosition()
{
	Point32 pt;
	pt.x = (_video->GetXSize() - this->rect.width) / 2;
	pt.y = (_video->GetYSize() - this->rect.height)/ 2;
	return pt;
}

/** Default constructor. */
QuitProgramWindow::QuitProgramWindow() : GuiWindow(WC_QUIT)
{
	this->SetupWidgetTree(_quit_program_widgets, lengthof(_quit_program_widgets));
}


/* virtual */ void QuitProgramWindow::OnClick(WidgetNumber number)
{
	if (number == QP_YES) QuitProgram();
	_manager.DeleteWindow(this);
}

/** Ask the user whether the program should be stopped. */
void ShowQuitProgram()
{
	Window *w = GetWindowByType(WC_QUIT);
	if (w != NULL) _manager.DeleteWindow(w);

	new QuitProgramWindow();
}

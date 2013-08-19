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
#include "language.h"

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
 * Widget numbers of the toolbar GUI.
 * @ingroup gui_group
 */
enum ToolbarGuiWidgets {
	TB_GUI_QUIT,  ///< Quit program button.
	TB_GUI_PATHS, ///< Build paths button.
	TB_GUI_SAVE,  ///< Save game button.
	TB_GUI_LOAD,  ///< Load game button.
	TB_GUI_RIDE_SELECT, ///< Select ride button.
	TB_GUI_TERRAFORM,   ///< Terraform button.
	TB_GUI_FINANCES,   ///< Finances button.
};

/**
 * Widget parts of the toolbar GUI.
 * @ingroup gui_group
 */
static const WidgetPart _toolbar_widgets[] = {
	Intermediate(1, 0),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_QUIT,  COL_RANGE_BROWN), SetData(GUI_TOOLBAR_GUI_QUIT,  GUI_TOOLBAR_GUI_TOOLTIP_QUIT_PROGRAM),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_PATHS, COL_RANGE_BROWN), SetData(GUI_TOOLBAR_GUI_PATHS, GUI_TOOLBAR_GUI_TOOLTIP_BUILD_PATHS),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_SAVE,  COL_RANGE_BROWN), SetData(GUI_TOOLBAR_GUI_SAVE,  GUI_TOOLBAR_GUI_TOOLTIP_SAVE_GAME),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_LOAD,  COL_RANGE_BROWN), SetData(GUI_TOOLBAR_GUI_LOAD,  GUI_TOOLBAR_GUI_TOOLTIP_LOAD_GAME),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_RIDE_SELECT, COL_RANGE_BROWN),
				SetData(GUI_TOOLBAR_GUI_RIDE_SELECT,  GUI_TOOLBAR_GUI_TOOLTIP_RIDE_SELECT),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_TERRAFORM, COL_RANGE_BROWN),
				SetData(GUI_TOOLBAR_GUI_TERRAFORM,  GUI_TOOLBAR_GUI_TOOLTIP_TERRAFORM),
		Widget(WT_TEXT_PUSHBUTTON, TB_GUI_FINANCES, COL_RANGE_BROWN),
				SetData(GUI_TOOLBAR_GUI_FINANCES,  GUI_TOOLBAR_GUI_TOOLTIP_FINANCES),
	EndContainer(),
};


ToolbarWindow::ToolbarWindow() : GuiWindow(WC_TOOLBAR, ALL_WINDOWS_OF_TYPE)
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
			/// \todo Implement saving.
			break;

		case TB_GUI_LOAD:
			/// \todo Implement loading.
			break;

		case TB_GUI_RIDE_SELECT:
			ShowRideSelectGui();
			break;

		case TB_GUI_TERRAFORM:
			ShowTerraformGui();
			break;

		case TB_GUI_FINANCES:
			ShowFinancesGui();
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

/** %Window to ask confirmation for ending the program. */
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

/** Window definition of the quit program GUI. */
static const WidgetPart _quit_program_widgets[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, COL_RANGE_RED), SetData(GUI_QUIT_CAPTION, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_RED),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_RED),
			Intermediate(2,0),
				Widget(WT_CENTERED_TEXT, QP_MESSAGE, COL_RANGE_RED),
						SetData(GUI_QUIT_MESSAGE, STR_NULL), SetPadding(5, 5, 5, 5),
			EndContainer(),
			Intermediate(1, 5), SetPadding(0, 0, 3, 0),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
				Widget(WT_TEXT_PUSHBUTTON, QP_NO, COL_RANGE_YELLOW), SetData(GUI_QUIT_NO, STR_NULL),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, 0), SetFill(1, 0),
				Widget(WT_TEXT_PUSHBUTTON, QP_YES, COL_RANGE_YELLOW), SetData(GUI_QUIT_YES, STR_NULL),
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
QuitProgramWindow::QuitProgramWindow() : GuiWindow(WC_QUIT, ALL_WINDOWS_OF_TYPE)
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
	Window *w = GetWindowByType(WC_QUIT, ALL_WINDOWS_OF_TYPE);
	if (w != NULL) _manager.DeleteWindow(w);

	new QuitProgramWindow();
}

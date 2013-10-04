/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file error_gui.cpp GUI related to errors. */

#include "stdafx.h"
#include "error.h"
#include "language.h"
#include "window.h"

/** Widgets of the error message window. */
enum ErrorMessageWidgets {
	EMW_TITLEBAR,      ///< Title bar widget.
	EMW_ERROR_MESSAGE, ///< Where the error message is shown.
};

/** Widget parts of the #ErrorMessageWindow. */
static const WidgetPart _error_message_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, EMW_TITLEBAR, COL_RANGE_RED), SetData(GUI_TERRAFORM_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_RED),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_RED),
			Widget(WT_CENTERED_TEXT, EMW_ERROR_MESSAGE, COL_RANGE_RED),
				SetPadding(0, 2, 0, 2),
				SetData(STR_ARG1, STR_NULL),
	EndContainer(),
};

/** GUI window for showing an error message. */
class ErrorMessageWindow : public GuiWindow {
public:
	ErrorMessageWindow(StringID strid);

	void SetWidgetStringParameters(WidgetNumber wid_num) const override;

private:
	StringID strid; ///< The error message to be displayed.
};

/**
 * Constructor of the error message window.
 * @param strid #StringID to show.
 */
ErrorMessageWindow::ErrorMessageWindow(StringID strid) : GuiWindow(WC_ERROR_MESSAGE, strid)
{
	this->strid = strid;
	this->SetupWidgetTree(_error_message_gui_parts, lengthof(_error_message_gui_parts));
}

void ErrorMessageWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case EMW_ERROR_MESSAGE:
			_str_params.SetStrID(1, strid);
			break;

		default: break;
	}
}

/**
 * Open an error message window.
 * @ingroup gui_group
 */
void ShowErrorMessage(StringID strid)
{
	if (HighlightWindowByType(WC_ERROR_MESSAGE, strid)) return;
	new ErrorMessageWindow(strid);
}

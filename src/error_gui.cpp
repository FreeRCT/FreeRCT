/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file error_gui.cpp GUI related to errors. */

#include "stdafx.h"
#include "language.h"
#include "window.h"

/** Widgets of the error message window. */
enum ErrorMessageWidgets {
	EMW_TITLEBAR,      ///< Title bar widget.
	EMW_ERROR_MESSAGE, ///< Where the error message is shown.
};

/** GUI window for showing an error message. */
class ErrorMessageWindow : public GuiWindow {
public:
	ErrorMessageWindow(StringID str1, StringID str2, const std::function<void()> &string_params, uint32 timeout);

	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void TimeoutCallback() override;

private:
	std::function<void()> set_string_params;  ///< Function that sets the string parameters for the error message.
	bool timeout_timer_running;               ///< Whether the window is going to auto-close soon.
	uint32 timeout_duration;                  ///< Number of ticks after which the window auto-closes.
};

/**
 * Constructor of the error message window.
 * @param str1 First #StringID to show.
 * @param str2 Second #StringID to show.
 */
ErrorMessageWindow::ErrorMessageWindow(const StringID str1, const StringID str2, const std::function<void()> &string_params, const uint32 t)
: GuiWindow(WC_ERROR_MESSAGE, str1)
{
	this->set_string_params = string_params;
	this->timeout_duration = t;
	this->timeout_timer_running = false;
	const WidgetPart _error_message_gui_parts[] = {
		Intermediate(0, 1),
			Intermediate(1, 0),
				Widget(WT_TITLEBAR, EMW_TITLEBAR, COL_RANGE_RED), SetData(GUI_TERRAFORM_TITLE, GUI_TITLEBAR_TIP),
				Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_RED),
			EndContainer(),
			Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_RED),
				Intermediate(2, 1),
					Widget(WT_CENTERED_TEXT, INVALID_WIDGET_INDEX, COL_RANGE_RED), SetData(str1, STR_NULL), SetMinimalSize(200, 40),
					Widget(WT_CENTERED_TEXT, EMW_ERROR_MESSAGE, COL_RANGE_RED),    SetData(str2, STR_NULL), SetMinimalSize(200, 40),
		EndContainer(),
	};
	this->SetupWidgetTree(_error_message_gui_parts, lengthof(_error_message_gui_parts));
}

void ErrorMessageWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case EMW_ERROR_MESSAGE:
			this->set_string_params();
			break;

		default: break;
	}
}

void ErrorMessageWindow::TimeoutCallback()
{
	if (this->timeout_timer_running) {
		delete this;
	} else {
		GuiWindow::TimeoutCallback();
		if (this->timeout_duration > 0) {
			this->timeout = this->timeout_duration;
			this->timeout_timer_running = true;
		}
	}
}

/**
 * Open an error message window.
 * @param str1 Message to display in the first line.
 * @param str2 Message to display in the second line.
 * @param string_params Function that sets the string parameters for the error message.
 * @param timeout Number of ticks after which the window auto-closes (\c 0 means never).
 * @ingroup gui_group
 */
void ShowErrorMessage(const StringID str1, const StringID str2, const std::function<void()> &string_params, const uint32 timeout)
{
	Window *w;
	do {
		w = HighlightWindowByType(WC_ERROR_MESSAGE, ALL_WINDOWS_OF_TYPE);
		delete w;
	} while (w != nullptr);
	new ErrorMessageWindow(str1, str2, string_params, timeout);
}

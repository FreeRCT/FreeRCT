/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file confirmation_gui.cpp GUI related to confirmation prompts. */

#include "stdafx.h"
#include "language.h"
#include "window.h"

/**
 * Widget numbers of the confirmation window.
 * @ingroup gui_group
 */
enum ConfirmationWidgets {
	CF_TITLE,   ///< Window title bar.
	CF_MESSAGE, ///< Displayed message.
	CF_YES,     ///< 'yes' button.
	CF_NO,      ///< 'no' button.
};

/**
 * Widget parts of the confirmation window.
 * @ingroup gui_group
 */
static const WidgetPart _confirmation_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, CF_TITLE            , COL_RANGE_RED), SetData(STR_ARG1, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_RED),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_RED),
			Intermediate(2,0),
				Widget(WT_CENTERED_TEXT, CF_MESSAGE, COL_RANGE_RED),
						SetData(STR_ARG1, STR_NULL), SetPadding(5, 5, 5, 5),
			EndContainer(),
			Intermediate(1, 5), SetPadding(0, 0, 3, 0),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
				Widget(WT_TEXT_PUSHBUTTON, CF_NO, COL_RANGE_YELLOW), SetData(GUI_CONFIRM_NO, STR_NULL),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
				Widget(WT_TEXT_PUSHBUTTON, CF_YES, COL_RANGE_YELLOW), SetData(GUI_CONFIRM_YES, STR_NULL),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
			EndContainer(),
};

/** %Window to ask confirmation for a user action. */
class ConfirmationWindow : public GuiWindow {
public:
	ConfirmationWindow(StringID title, StringID message, std::function<void()> callback);

	Point32 OnInitialPosition() override;
	void OnClick(WidgetNumber number, const Point16 &pos) override;
	bool OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol) override;
	void SetWidgetStringParameters(WidgetNumber wid_num) const override;

private:
	StringID title;                  ///< Caption to display.
	StringID message;                ///< String to display.
	std::function<void()> callback;  ///< Callback when the user clicks OK.
};

/**
 * Constructor.
 * @param title Window caption to show.
 * @param message Message to display.
 * @param callback Function to invoke when the user confirms the prompt.
 */
ConfirmationWindow::ConfirmationWindow(StringID title, StringID message, std::function<void()> callback)
: GuiWindow(WC_CONFIRM, ALL_WINDOWS_OF_TYPE), title(title), message(message), callback(callback)
{
	this->SetupWidgetTree(_confirmation_gui_parts, lengthof(_confirmation_gui_parts));
}

void ConfirmationWindow::SetWidgetStringParameters(const WidgetNumber wid_num) const
{
	switch (wid_num) {
		case CF_TITLE:
			_str_params.SetStrID(1, this->title);
			break;
		case CF_MESSAGE:
			_str_params.SetStrID(1, this->message);
			break;

		default: break;
	}
}

Point32 ConfirmationWindow::OnInitialPosition()
{
	Point32 pt;
	pt.x = (_video.Width() - this->rect.width ) / 2;
	pt.y = (_video.Height() - this->rect.height) / 2;
	return pt;
}

bool ConfirmationWindow::OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol)
{
	switch (key_code) {
		case WMKC_CONFIRM:
			this->OnClick(CF_YES, Point16());
			return true;

		case WMKC_CANCEL:
			this->OnClick(CF_NO, Point16());
			return true;

		default:
			return GuiWindow::OnKeyEvent(key_code, mod, symbol);
	}
}

void ConfirmationWindow::OnClick(WidgetNumber number, [[maybe_unused]] const Point16 &pos)
{
	if (number == CF_YES) {
		this->callback();
	} else if (number != CF_NO) {
		return;
	}
	delete this;
}

/**
 * Show a prompt to the user to confirm an action.
 * @param title Window caption to show.
 * @param message Message to display.
 * @param callback Function to invoke when the user confirms the prompt.
 */
void ShowConfirmationPrompt(StringID title, StringID message, std::function<void()> callback)
{
	Window *w;
	do {
		w = HighlightWindowByType(WC_CONFIRM, ALL_WINDOWS_OF_TYPE);
		delete w;
	} while (w != nullptr);

	new ConfirmationWindow(title, message, callback);
}

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file textbox.cpp Implementation of the textbox widget. */

#include "stdafx.h"
#include "window.h"
#include "palette.h"
#include "bitmath.h"
#include "text_buffer.h"
#include "string_func.h"

class EditTextWindow : public GuiWindow {
public:
	EditTextWindow(uint8 *text);
	~EditTextWindow();

	Point32 OnInitialPosition() override;
	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void OnChange(ChangeCode code, uint32 parameter) override;
	void OnClick(WidgetNumber number, const Point16 &pos) override;
	bool OnKeyEvent(WmKeyCode key_code, const uint8 *symbol) override;

private:
	TextBuffer text_buffer;
	uint8 *text;
};

/**
 * Widget numbers of the setting window.
 * @ingroup gui_group
 */
enum EditTextWidgets {
	ETW_TITLEBAR,  ///< Titlebar widget.
	ETW_EDIT_TEXT, ///< Edit text box.
	ETW_OK,        ///< 'ok' button.
	ETW_CANCEL,    ///< 'cancel' button.
};

/**
 * Widget parts of the edit text window.
 * @ingroup gui_group
 */
static const WidgetPart _edit_text_widgets[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, ETW_TITLEBAR, COL_RANGE_BLUE), SetData(GUI_EDIT_TEXT_CAPTION, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_BLUE),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_BLUE),
			Intermediate(2, 0),
				Widget(WT_EDIT_TEXT, ETW_EDIT_TEXT, COL_RANGE_BLUE),
						SetData(STR_ARG1, STR_NULL), SetPadding(5, 5, 5, 5),
						SetMinimalSize(200, 10),
			EndContainer(),
			Intermediate(1, 5), SetPadding(0, 0, 3, 0),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
				Widget(WT_TEXT_PUSHBUTTON, ETW_OK, COL_RANGE_YELLOW), SetData(GUI_EDIT_TEXT_OK, STR_NULL),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
				Widget(WT_TEXT_PUSHBUTTON, ETW_CANCEL, COL_RANGE_YELLOW), SetData(GUI_EDIT_TEXT_CANCEL, STR_NULL),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
			EndContainer()
};

EditTextWindow::EditTextWindow(uint8 *text) : GuiWindow(WC_EDIT_TEXT, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_edit_text_widgets, lengthof(_edit_text_widgets));
	this->text_buffer.InsertText((char *)text); //\0x5f");
	this->text = text;
}

EditTextWindow::~EditTextWindow()
{
}

Point32 EditTextWindow::OnInitialPosition()
{
	Point32 pt;
	pt.x = (_video.GetXSize() - this->rect.width ) / 2;
	pt.y = (_video.GetYSize() - this->rect.height) / 2;
	return pt;
}

void EditTextWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	switch (wid_num) {
		case ETW_EDIT_TEXT:
			_str_params.SetUint8(1, (uint8 *)this->text_buffer.GetText().c_str());
			break;
		default:
			break;
	}
}

void EditTextWindow::OnChange(ChangeCode code, uint32 parameter)
{
	if (code == CHG_DISPLAY_OLD) this->MarkDirty();
}

void EditTextWindow::OnClick(WidgetNumber number, const Point16 &pos)
{
	if (number == ETW_OK || number == ETW_CANCEL) {
		if (number == ETW_OK) {
			this->text = (uint8 *)this->text_buffer.GetText().c_str();
		}
		delete this;
	}
}

bool EditTextWindow::OnKeyEvent(WmKeyCode key_code, const uint8 *symbol)
{
	switch (key_code) {
		case WMKC_BACKSPACE:
			this->text_buffer.RemovePrevCharacter();
			break;
		case WMKC_DELETE:
			this->text_buffer.RemoveCurrentCharacter();
			break;
		case WMKC_CURSOR_LEFT:
			this->text_buffer.DecPosition();
			break;
		case WMKC_CURSOR_RIGHT:
			this->text_buffer.IncPosition();
			break;
		case WMKC_SPACE:
			this->text_buffer.InsertText((const char *)" ");
			break;
		case WMKC_SYMBOL:
			if (SDL_GetModState() & KMOD_CTRL) {
				if (symbol[0] == 'v') {
					this->text_buffer.InsertText(SDL_GetClipboardText());
				} else if (symbol[0] == 'c') {
					SDL_SetClipboardText(this->text_buffer.GetText().c_str());
				}
			} else {
				this->text_buffer.InsertText((const char *)symbol);
			}
			break;
		default:
			break;
	}
	this->MarkDirty();
	return true;
}

/**
 * Open the edit text window.
 * @ingroup gui_group
 */
void ShowEditTextGui(uint8 *text)
{
	if (HighlightWindowByType(WC_EDIT_TEXT, ALL_WINDOWS_OF_TYPE)) return;
	new EditTextWindow(text);
}

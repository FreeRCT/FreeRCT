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
	EditTextWindow(uint8 **text, uint max_length);
	~EditTextWindow();

	Point32 OnInitialPosition() override;
	void SetWidgetStringParameters(WidgetNumber wid_num) const override;
	void OnChange(ChangeCode code, uint32 parameter) override;
	void OnClick(WidgetNumber number, const Point16 &pos) override;
	bool OnKeyEvent(WmKeyCode key_code, const uint8 *symbol) override;

	void Complete();

private:
	TextBuffer text_buffer;
	uint8 **text;
	uint start;
};

/**
 * Widget numbers of the setting window.
 * @ingroup gui_group
 */
enum EditTextWidgets {
	ETW_TITLEBAR,  ///< Titlebar widget.
	ETW_EDIT_TEXT, ///< Edit text box.
	ETW_SCROLL_TEXT, ///< Scroll bar for text box.
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
			Intermediate(2, 1),
				Widget(WT_EDIT_TEXT, ETW_EDIT_TEXT, COL_RANGE_BLUE),
						SetData(STR_ARG1, STR_NULL), SetPadding(5, 5, 5, 5),
						SetMinimalSize(200, 10),
				Widget(WT_HOR_SCROLLBAR, ETW_SCROLL_TEXT, COL_RANGE_BLUE),
			EndContainer(),
			Intermediate(1, 5), SetPadding(0, 0, 3, 0),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
				Widget(WT_TEXT_PUSHBUTTON, ETW_OK, COL_RANGE_YELLOW), SetData(GUI_EDIT_TEXT_OK, STR_NULL),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
				Widget(WT_TEXT_PUSHBUTTON, ETW_CANCEL, COL_RANGE_YELLOW), SetData(GUI_EDIT_TEXT_CANCEL, STR_NULL),
				Widget(WT_EMPTY, INVALID_WIDGET_INDEX, COL_RANGE_INVALID), SetFill(1, 0),
			EndContainer()
};

EditTextWindow::EditTextWindow(uint8 **text, uint max_length) : GuiWindow(WC_EDIT_TEXT, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_edit_text_widgets, lengthof(_edit_text_widgets));
	this->SetScrolledWidget(ETW_EDIT_TEXT, ETW_SCROLL_TEXT);
	this->text_buffer.SetMaxLength(max_length);
	this->text_buffer.InsertText((char *)*text);
	this->text = text;
	ScrollbarWidget *sb = this->GetWidget<ScrollbarWidget>(ETW_SCROLL_TEXT);
	int item_width, item_height = 0;
	_video.GetTextSize((const uint8 *)"_", &item_width, &item_height);
	sb->SetItemSize(item_width);
	sb->SetItemCount(this->text_buffer.GetText().length());
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
		case ETW_EDIT_TEXT: {
			_str_params.SetUint8(1, (uint8 *)this->text_buffer.GetText().c_str());
			const DataWidget *wid = this->GetWidget<DataWidget>(wid_num);
			//const ScrollbarWidget *sb = this->GetWidget<ScrollbarWidget>(ETW_SCROLL_TEXT);
			int width, second_width, height, second_height = 0;
			//_video.GetTextSize((const uint8 *)this->text_buffer.GetText().substr(sb->GetStart(), sb->GetVisibleCount()).c_str(), &width, &height);
			_video.GetTextSize((const uint8 *)this->text_buffer.GetText().substr(0, this->text_buffer.GetPosition()).c_str(), &width, &height);
			_video.GetTextSize((const uint8 *)"_", &second_width, &second_height);
			_video.BlitText((const uint8 *)"_", MakeRGBA(255, 255, 255, OPAQUE), GetWidgetScreenX(wid) + second_width + width, GetWidgetScreenY(wid) + (height / 2), second_width);
			}
			break;
		default:
			break;
	}
}

/*
void EditTextWindow::GetVisibleCount() const
{
	//_video.GetTextSize((const uint8 *)this->text_buffer.GetText().substr(0, this->text_buffer.GetPosition()).c_str(), &width, &height);
	//wid->pos.width;
}

void EditTextWindow::ScrollTo(uint offset)
{
	int text_length = this->text_buffer.GetText().length();
	//if (offset >= text_length) offset = (text_length > 0) ? text_length - 1 : 0;

	//if (offset < start) {
	// set start
	//} else {
	//visible_count;
	//if (offset >= start + visible_count) set start (offset - visible_count);
	//}
}
*/

void EditTextWindow::OnChange(ChangeCode code, uint32 parameter)
{
	if (code == CHG_DISPLAY_OLD) this->MarkDirty();
}

void EditTextWindow::OnClick(WidgetNumber number, const Point16 &pos)
{
	if (number == ETW_OK || number == ETW_CANCEL) {
		if (number == ETW_OK) {
			Complete();
		}
		delete this;
	} else if (number == ETW_EDIT_TEXT) {
		int second_width, second_height;
		_video.GetTextSize((const uint8 *)"_", &second_width, &second_height);
		this->text_buffer.SetPosition((pos.x - second_width) / second_width);
	}
}

bool EditTextWindow::OnKeyEvent(WmKeyCode key_code, const uint8 *symbol)
{
	bool finish = false;
	bool completed = false;

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
		case WMKC_CONFIRM:
			completed = true;
		case WMKC_CANCEL:
			finish = true;
			break;
		case WMKC_SYMBOL:
			if (SDL_GetModState() & KMOD_CTRL) {
				switch (symbol[0]) {
					case 'v':
						this->text_buffer.InsertText(SDL_GetClipboardText());
						break;
					case 'c':
						SDL_SetClipboardText(this->text_buffer.GetText().c_str());
						break;
					case 'x':
						SDL_SetClipboardText(this->text_buffer.GetText().c_str());
						this->text_buffer.SetText((const char *)"");
						break;
					default:
						break;
				}
			} else {
				this->text_buffer.InsertText((const char *)symbol);
			}
			break;
		default:
			break;
	}
	if (!finish) {
		this->MarkDirty();
	} else {
		if (completed) Complete();
		delete this;
	}
	return true;
}

void EditTextWindow::Complete()
{
	int len = this->text_buffer.GetText().length();
	SafeStrncpy(*this->text, (const uint8 *)this->text_buffer.GetText().c_str(), len + 1);
}

/**
 * Open the edit text window.
 * @ingroup gui_group
 */
void ShowEditTextGui(uint8 **text, uint max_length)
{
	if (HighlightWindowByType(WC_EDIT_TEXT, ALL_WINDOWS_OF_TYPE)) return;
	new EditTextWindow(text, max_length);
}

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file inbox_gui.cpp Inbox gui code. */

#include "messages.h"
#include "person.h"
#include "people.h"
#include "window.h"
#include "sprite_data.h"
#include "sprite_store.h"
#include "gui_sprites.h"

/**
  * GUI for viewing and managing the list of inbox messages.
  * @ingroup gui_group
  */
class InboxGui : public GuiWindow {
public:
	InboxGui();

	void OnDraw(MouseModeSelector *selector) override;
	void DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const override;
	void OnClick(WidgetNumber wid_num, const Point16 &pos) override;

	const Message *GetMessage(WidgetNumber wid_num) const;
};

static const int IBX_NR_ROWS = 5;      ///< Number of message rows in the inbox window.
static const int BUTTON_WIDTH = 400;   ///< Pixel width of a message row.
static const int BUTTON_HEIGHT = 65;   ///< Pixel height of a message row.
static const int MESSAGE_PADDING = 2;  ///< Padding inside a message row.

/**
 * Widget numbers of the inbox GUI.
 * The widget indices for the message rows are 1..#IBX_NR_ROWS.
 * @ingroup gui_group
 */
enum InboxWidgets {
	IBX_SCROLLBAR = 2 * IBX_NR_ROWS,
	IBX_MAIN_PANEL,
};

#define INBOX_ROW_BUTTON(index) \
	Widget(WT_EMPTY, index, COL_RANGE_GREY), SetData(STR_ARG1, STR_NULL), SetResize(0, BUTTON_HEIGHT), SetMinimalSize(BUTTON_WIDTH, BUTTON_HEIGHT) \

/** Widget parts of the #InboxGui window. */
static const WidgetPart _inbox_gui_parts[] = {
	Intermediate(0, 1),
		Intermediate(1, 0),
			Widget(WT_TITLEBAR, INVALID_WIDGET_INDEX, COL_RANGE_GREY), SetData(GUI_INBOX_TITLE, GUI_TITLEBAR_TIP),
			Widget(WT_CLOSEBOX, INVALID_WIDGET_INDEX, COL_RANGE_GREY),
		EndContainer(),
		Widget(WT_PANEL, INVALID_WIDGET_INDEX, COL_RANGE_GREY),
			Intermediate(1, 2),
				Widget(WT_PANEL, IBX_MAIN_PANEL, COL_RANGE_GREY),
					Intermediate(IBX_NR_ROWS, 1),
						INBOX_ROW_BUTTON(1),
						INBOX_ROW_BUTTON(2),
						INBOX_ROW_BUTTON(3),
						INBOX_ROW_BUTTON(4),
						INBOX_ROW_BUTTON(5),
			Widget(WT_VERT_SCROLLBAR, IBX_SCROLLBAR, COL_RANGE_GREY),
	EndContainer(),
};
#undef INBOX_ROW_BUTTON

InboxGui::InboxGui() : GuiWindow(WC_INBOX, ALL_WINDOWS_OF_TYPE)
{
	this->SetupWidgetTree(_inbox_gui_parts, lengthof(_inbox_gui_parts));
	this->SetScrolledWidget(IBX_MAIN_PANEL, IBX_SCROLLBAR);
}

/**
 * The message which is represented by the indicated message row.
 * @param wid_num Message row ID.
 * @return The message, or \c nullptr if there is no message here.
 */
const Message *InboxGui::GetMessage(const WidgetNumber wid_num) const
{
	if (wid_num < 1 || wid_num > IBX_NR_ROWS) return nullptr;
	const ScrollbarWidget *scrollbar = this->GetWidget<ScrollbarWidget>(IBX_SCROLLBAR);
	const int nr_messages = _inbox.messages.size();
	const int message_index = nr_messages - wid_num - scrollbar->GetStart();
	if (message_index < 0 || message_index >= nr_messages) return nullptr;
	auto it = _inbox.messages.begin();
	std::advance(it, message_index);
	return it->get();
}

void InboxGui::OnClick(const WidgetNumber wid_num, const Point16 &pos)
{
	const Message *msg = this->GetMessage(wid_num);
	if (msg == nullptr) return GuiWindow::OnClick(wid_num, pos);
	msg->OnClick();
}

void InboxGui::OnDraw(MouseModeSelector *s)
{
	this->GetWidget<ScrollbarWidget>(IBX_SCROLLBAR)->SetItemCount(_inbox.messages.size());
	GuiWindow::OnDraw(s);
}

void InboxGui::DrawWidget(const WidgetNumber wid_num, const BaseWidget *wid) const
{
	const Message *msg = this->GetMessage(wid_num);
	if (msg == nullptr) return GuiWindow::DrawWidget(wid_num, wid);

	const int x = this->GetWidgetScreenX(wid) + MESSAGE_PADDING;
	const int y = this->GetWidgetScreenY(wid) + MESSAGE_PADDING;
	const int w = wid->pos.width - 2 * MESSAGE_PADDING;
	const int h = wid->pos.height - 2 * MESSAGE_PADDING;
	DrawMessage(msg, Rectangle32(x, y, w, h), false);
}

/**
 * Draw an inbox message on the screen.
 * @param msg Message to draw.
 * @param rect Rectangle on the screen.
 * @param narrow Whether to use a condensed layout.
 */
void DrawMessage(const Message *msg, const Rectangle32 &rect, const bool narrow)
{
	const int text_w = rect.width - rect.height - 3 * MESSAGE_PADDING;
	const int text_y = (narrow ? MESSAGE_PADDING : GetTextHeight()) + MESSAGE_PADDING;
	_video.FillRectangle(rect, 0xff);

	uint8 colour;
	switch (msg->category) {
		case MSC_GOOD: colour = COL_RANGE_BLUE;   break;
		case MSC_INFO: colour = COL_RANGE_YELLOW; break;
		case MSC_BAD:  colour = COL_RANGE_RED;    break;
		default: NOT_REACHED();
	}
	/* Conversion from a Colour Range to a Palette index. */
	colour *= COL_SERIES_LENGTH;
	colour += COL_SERIES_START + COL_SERIES_LENGTH / 2;

	msg->SetStringParameters();
	DrawMultilineString(msg->message, rect.base.x + MESSAGE_PADDING, rect.base.y + text_y, text_w, rect.height - text_y - MESSAGE_PADDING, colour);

	if (!narrow) {
		_str_params.SetDate(1, msg->timestamp);
		DrawString(STR_ARG1, TEXT_WHITE, rect.base.x + MESSAGE_PADDING, rect.base.y, text_w);
	}

	int sprite_to_draw;
	switch (msg->data_type) {
		case MDT_NONE: return;  // No button needed.
		case MDT_GOTO:          sprite_to_draw = SPR_GUI_MESSAGE_GOTO; break;
		case MDT_PARK:          sprite_to_draw = SPR_GUI_MESSAGE_PARK; break;
		case MDT_GUEST:         sprite_to_draw = SPR_GUI_MESSAGE_GUEST; break;
		case MDT_RIDE_INSTANCE: sprite_to_draw = SPR_GUI_MESSAGE_RIDE; break;
		case MDT_RIDE_TYPE:     sprite_to_draw = SPR_GUI_MESSAGE_RIDE_TYPE; break;
		default: NOT_REACHED();
	}
	static Recolouring rc;  // Never modified.
	const ImageData *imgdata = _sprite_manager.GetTableSprite(sprite_to_draw);
	if (imgdata != nullptr) {
		_video.BlitImage(Point32(
				rect.base.x + rect.width - rect.height,
				rect.base.y + (static_cast<int>(rect.height) - static_cast<int>(imgdata->height)) / 2),
			imgdata, rc, GS_NORMAL);
	}
}

/** Open the inbox window (or if it is already open, highlight and raise it). */
void ShowInboxGui()
{
	if (HighlightWindowByType(WC_INBOX, ALL_WINDOWS_OF_TYPE) != nullptr) return;
	new InboxGui;
}

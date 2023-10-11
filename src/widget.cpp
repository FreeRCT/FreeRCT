/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file widget.cpp Widget code. */

#include "stdafx.h"
#include "math_func.h"
#include "sprite_store.h"
#include "sprite_data.h"
#include "string_func.h"
#include "widget.h"
#include "window.h"
#include "video.h"
#include "gui_sprites.h"

/**
 * @defgroup widget_group Widgets and supporting functions of the program
 */

/**
 * @defgroup widget_parts_group %Widget parts for specifying windows
 * @ingroup widget_group
 */

/**
 * Base class widget constructor.
 * @param wtype %Widget type.
 */
BaseWidget::BaseWidget(WidgetType wtype)
:
	wtype(wtype),
	number(INVALID_WIDGET_INDEX),
	visible(true),
	smallest_x(0),
	smallest_y(0),
	min_x(0),
	min_y(0),
	pos(0, 0, 0, 0),
	fill_x(0),
	fill_y(0),
	resize_x(0),
	resize_y(0),
	receive_repeated_events(false),
	tooltip(STR_NULL)
{
	for (int i = 0; i < PAD_COUNT; i++) this->paddings[i] = 0;

	switch (wtype) {
		case WT_TITLEBAR:
			this->fill_x = 1;
			this->resize_x = 1;
			break;

		case WT_LEFT_TEXT:
		case WT_CENTERED_TEXT:
		case WT_RIGHT_TEXT:
		case WT_TEXT_INPUT:
			this->fill_x = 1;
			break;

		case WT_TEXT_BUTTON:
		case WT_TEXT_PUSHBUTTON:
			this->fill_x = 1;
			this->fill_y = 1;
			break;

		case WT_VERT_SCROLLBAR:
			this->fill_y = 1;
			this->resize_y = 1;
			break;

		case WT_HOR_SCROLLBAR:
			this->fill_x = 1;
			this->resize_x = 1;
			break;

		default: break; // Default: Leave all zero.
	}
}

BaseWidget::~BaseWidget()
= default;

/**
 * Initialize the minimal size of the widget based on the width and height of the content, and the necessary border space.
 * @param content_width Minimal width of the content.
 * @param content_height Minimal height of the content.
 * @param border_hor Horizontal border space.
 * @param border_vert Vertical border space.
 */
void BaseWidget::InitMinimalSize(uint16 content_width, uint16 content_height, uint16 border_hor, uint16 border_vert)
{
	this->min_x = std::max(this->min_x, (uint16)(content_width + border_hor + this->paddings[PAD_LEFT] + this->paddings[PAD_RIGHT]));
	this->min_y = std::max(this->min_y, (uint16)(content_height + border_vert + this->paddings[PAD_TOP] + this->paddings[PAD_BOTTOM]));
}

/**
 * Initialize the minimal size of the widget based on the width and height of the content, and the border sprites.
 * @param bsd Used border sprites.
 * @param content_width Minimal width of the content.
 * @param content_height Minimal height of the content.
 */
void BaseWidget::InitMinimalSize(const BorderSpriteData *bsd, uint16 content_width, uint16 content_height)
{
	content_width = std::max(content_width, bsd->min_width);
	if (bsd->hor_stepsize > 0) content_width = bsd->min_width + (content_width - bsd->min_width + bsd->hor_stepsize - 1) / bsd->hor_stepsize;

	content_height = std::max(content_height, bsd->min_height);
	if (bsd->vert_stepsize > 0) content_height = bsd->min_height + (content_height - bsd->min_height + bsd->vert_stepsize - 1) / bsd->vert_stepsize;

	this->InitMinimalSize(content_width, content_height,  bsd->border_left + bsd->border_right, bsd->border_top + bsd->border_bottom);
	this->fill_x = LeastCommonMultiple(this->fill_x, bsd->hor_stepsize);
	this->fill_y = LeastCommonMultiple(this->fill_y, bsd->vert_stepsize);
	this->resize_x = LeastCommonMultiple(this->resize_x, bsd->hor_stepsize);
	this->resize_y = LeastCommonMultiple(this->resize_y, bsd->vert_stepsize);
}

/**
 * Add the widget to the widget array \a wid_array.
 * @param wid_array The widget array.
 */
void BaseWidget::SetWidget(BaseWidget **wid_array)
{
	if (this->number < 0) return;
	assert(wid_array[this->number] == nullptr);
	wid_array[this->number] = this;
}

/**
 * Set up minimal size (#min_x and #min_y), fill step (#fill_x and #fill_y) and resize step (#resize_x and #resize_y) of the widget.
 * In addition, if the widget has a non-negative widget number, add the widget to the \a wid_array after verifying the position is still empty.
 * @param w %Window owning the widget.
 * @param wid_array [out] Array of widget pointers.
 */
void BaseWidget::SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array)
{
	if (!w->initialized) this->SetWidget(wid_array);
	if (this->visible) {
		this->min_x = this->smallest_x;
		this->min_y = this->smallest_y;

		switch (this->wtype) {
			case WT_EMPTY:
				/* Do nothing (all variables are already set while converting from widget parts). */
				break;

			case WT_RESIZEBOX:
				this->min_x = std::max((uint16)10, this->min_x);
				this->min_y = std::max((uint16)10, this->min_y);
				this->fill_x = 0;
				this->fill_y = 1;
				this->resize_x = 0;
				this->resize_y = 0;
				break;

			default:
				NOT_REACHED();
		}
	} else {
		this->min_x = 0;
		this->min_y = 0;
		this->fill_x = 0;
		this->fill_y = 0;
		this->resize_x = 0;
		this->resize_y = 0;
	}

	if (this->number >= 0) w->UpdateWidgetSize(this->number, this);
}

/**
 * Set the minimal size of the widget, and assign \a rect to the current position and size.
 * @param rect Smallest size, and suggested position of the (entire) widget.
 */
void BaseWidget::SetSmallestSizePosition(const Rectangle16 &rect)
{
	this->pos = rect;
	this->min_x = rect.width;
	this->min_y = rect.height;
}

/**
 * Find the widget for which a tooltip should be shown.
 * @param pt Mouse cursor position relative to the widget.
 * @return The widget (may be \c nullptr).
 */
BaseWidget *BaseWidget::FindTooltipWidget(Point16 pt)
{
	return (this->tooltip != STR_NULL && this->pos.IsPointInside(pt)) ? this : nullptr;
}

static const int TOOLTIP_MAX_WIDTH = 300;     ///< Maximum width of a tooltip.
static const int TOOLTIP_BORDER_SPACING = 2;  ///< Spacing around a tooltip's borders.

/**
 * Draw the widget's tooltip.
 * @param p Absolute pixel to which this widget's location is relative.
 */
void BaseWidget::DrawTooltip(Point32 p)
{
	int inner_w, inner_h;
	GetMultilineTextSize(this->tooltip, TOOLTIP_MAX_WIDTH, &inner_w, &inner_h);
	const int outer_w = inner_w + 2 * TOOLTIP_BORDER_SPACING;
	const int outer_h = inner_h + 2 * TOOLTIP_BORDER_SPACING;
	const int x = std::max(0, std::min<int>(_video.Width() - outer_w, p.x + this->pos.base.x));
	const int y = std::max(0, std::min<int>(_video.Height() - outer_h, p.y + this->pos.base.y + this->pos.height));

	Rectangle32 r(x, y, outer_w, outer_h);
	_video.FillRectangle(r, _palette[TEXT_TOOLTIP_BACKGROUND]);
	DrawMultilineString(this->tooltip, x + TOOLTIP_BORDER_SPACING, y + TOOLTIP_BORDER_SPACING, TOOLTIP_MAX_WIDTH, inner_h + GetTextHeight(), TEXT_TOOLTIP_TEXT);
	_video.DrawRectangle(r, _palette[TEXT_TOOLTIP_BORDER]);
}

/**
 * Update the widget's drawing information and actually draw the widget.
 * @param w %Window being drawn.
 */
void BaseWidget::Draw(const GuiWindow *w)
{
	this->cached_window_base = w->rect.base;
	if (this->visible) this->DoDraw(w);  // Polymorphic function call.
}

/**
 * Draw the widget.
 * @param w %Window being drawn.
 */
void BaseWidget::DoDraw(const GuiWindow *w)
{
	/* Nothing to do for WT_EMPTY */
	if (this->number != INVALID_WIDGET_INDEX) w->DrawWidget(this->number, this);
}

/**
 * Change this widget's visibility state and update its window.
 * @param w The window this widget belongs to.
 * @param v The widget should be visible.
 */
void BaseWidget::SetVisible(GuiWindow *w, const bool v)
{
	this->visible = v;
	w->ResetSize();
}

/**
 * Get the widget at the given relative window position.
 * @param pt Relative point.
 * @return The widget underneath the point, or \c nullptr.
 */
BaseWidget *BaseWidget::GetWidgetByPosition(const Point16 &pt)
{
	if (this->pos.IsPointInside(pt)) return this;
	return nullptr;
}

/**
 * Raise all push buttons in the tree.
 * @param base Base position of the window.
 */
void BaseWidget::AutoRaiseButtons([[maybe_unused]] const Point32 &base)
{
}

/**
 * Widget was clicked.
 * @param base Base-coordinate of the window.
 * @param pos Position of the click in the window.
 * @return Mouse event has been processed.
 */
bool BaseWidget::OnClick([[maybe_unused]] const Point32 &base, [[maybe_unused]] const Point16 &pos)
{
	return false;
}

/**
 * Process input from the keyboard.
 * @param key_code Kind of input.
 * @param mod Bitmask of pressed modifiers.
 * @param symbol Entered symbol, if \a key_code is #WMKC_SYMBOL. Utf-8 encoded.
 * @return Key event has been processed.
 */
bool BaseWidget::OnKeyEvent([[maybe_unused]] WmKeyCode key_code, [[maybe_unused]] WmKeyMod mod, [[maybe_unused]] const std::string &symbol)
{
	return false;
}

/**
 * Process input from the mouse wheel.
 * @param direction How far the wheel has been moved (sign indicates direction).
 * @return Mouse wheel event has been processed.
 */
bool BaseWidget::OnMouseWheelEvent([[maybe_unused]] int direction)
{
	return false;
}

/**
 * Base class leaf widget constructor.
 * @param wtype %Widget type.
 */
LeafWidget::LeafWidget(WidgetType wtype) : BaseWidget(wtype), flags(0), colour(COL_RANGE_INVALID), shift(GS_NORMAL)
{
}

/**
 * Compute smallest size of the widget.
 * @param w %Window owning the widget.
 * @param wid_array [out] Array of widget pointers.
 * @todo Add support for #WT_RESIZEBOX.
 */
void LeafWidget::SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array)
{
	if (!w->initialized) this->SetWidget(wid_array);
	if (!this->visible) {
		this->min_x = 0;
		this->min_y = 0;
		this->fill_x = 0;
		this->fill_y = 0;
		this->resize_x = 0;
		this->resize_y = 0;
		return;
	}

	this->min_x = this->smallest_x;
	this->min_y = this->smallest_y;

	switch (this->wtype) {
		case WT_CLOSEBOX: {
			const ImageData *im = _gui_sprites.close_sprite;
			this->InitMinimalSize(&_gui_sprites.panel, im->width + 2, im->height + 2);

			this->fill_x = 0;
			this->fill_y = 1;
			this->resize_x = 0;
			this->resize_y = 0;
			break;
		}

		case WT_RADIOBUTTON: {
			const CheckableWidgetSpriteData cwsd = _gui_sprites.radio_button;
			this->InitMinimalSize(cwsd.width, cwsd.height, 0, 0);

			this->fill_x = 0;
			this->fill_y = 0;
			this->resize_x = 0;
			this->resize_y = 0;
			break;
		}

		case WT_CHECKBOX: {
			const CheckableWidgetSpriteData cwsd = _gui_sprites.checkbox;
			this->InitMinimalSize(cwsd.width, cwsd.height, 0, 0);

			this->fill_x = 0;
			this->fill_y = 0;
			this->resize_x = 0;
			this->resize_y = 0;
			break;
		}

		case WT_LEFT_FILLER_TAB:
			this->InitMinimalSize(&_gui_sprites.left_tabbar, 0, 0);
			break;

		case WT_RIGHT_FILLER_TAB:
			this->InitMinimalSize(&_gui_sprites.right_tabbar, 0, 0);
			break;


		default:
			NOT_REACHED();
	}
}

void LeafWidget::DoDraw(const GuiWindow *w)
{
	int left   = w->GetWidgetScreenX(this) + this->paddings[PAD_LEFT];
	int top    = w->GetWidgetScreenY(this) + this->paddings[PAD_TOP];
	int right  = w->GetWidgetScreenX(this) + this->pos.width - 1 - this->paddings[PAD_RIGHT];
	int bottom = w->GetWidgetScreenY(this) + this->pos.height - 1 - this->paddings[PAD_BOTTOM];

	static Recolouring rc;
	rc.Set(0, RecolourEntry(COL_RANGE_BROWN, this->colour));

	if (this->wtype == WT_RADIOBUTTON || this->wtype == WT_CHECKBOX) {
		int spr_num = ((this->flags & LWF_CHECKED) != 0) ? WCS_CHECKED : WCS_EMPTY;
		if ((this->flags & LWF_SHADED) != 0) {
			spr_num += WCS_SHADED_EMPTY;
		} else if ((this->flags & LWF_PRESSED) != 0) {
			spr_num += WCS_EMPTY_PRESSED;
		}
		_video.BlitImage({left, top}, (this->wtype == WT_CHECKBOX ? _gui_sprites.checkbox : _gui_sprites.radio_button).sprites[spr_num], rc, this->shift);
		return;
	}

	const BorderSpriteData *bsd;
	switch (this->wtype) {
		case WT_CLOSEBOX:         bsd = &_gui_sprites.panel; break;
		case WT_LEFT_FILLER_TAB:  bsd = &_gui_sprites.left_tabbar; break;
		case WT_RIGHT_FILLER_TAB: bsd = &_gui_sprites.right_tabbar; break;
		default: NOT_REACHED();
	}

	left += bsd->border_left;
	top += bsd->border_top;
	right -= bsd->border_right;
	bottom -= bsd->border_bottom;
	assert(right - left + 1 >= 0);
	assert(bottom - top + 1 >= 0);

	Rectangle32 rect(left, top, right - left + 1, bottom - top + 1);
	DrawBorderSprites(*bsd, false, rect, this->colour);

	if (this->wtype == WT_CLOSEBOX) {
		int xoffset = left + (right - 1 - left - _gui_sprites.close_sprite->width) / 2;
		int yoffset = top + (bottom - 1 - top - _gui_sprites.close_sprite->height) / 2;

		const ImageData *imgdata = _gui_sprites.close_sprite;
		if (imgdata != nullptr) _video.BlitImage({xoffset + 1, yoffset + 1}, imgdata, rc, GS_NORMAL);
		/* Closebox is never shaded. */
	}
}

void LeafWidget::AutoRaiseButtons([[maybe_unused]] const Point32 &base)
{
	if ((this->wtype == WT_TEXT_PUSHBUTTON || this->wtype == WT_IMAGE_PUSHBUTTON) && this->IsPressed()) {
		this->SetPressed(false);
	}
}

/**
 * Data widget constructor.
 * @param wtype %Widget type.
 */
DataWidget::DataWidget(WidgetType wtype) : LeafWidget(wtype), value(0), value_width(0), value_height(0)
{
}

/**
 * Compute smallest size of the widget.
 * @param w %Window owning the widget.
 * @param wid_array [out] Array of widget pointers.
 */
void DataWidget::SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array)
{
	_str_params.Clear();
	if (!w->initialized) this->SetWidget(wid_array);
	if (this->visible) {
		this->min_x = this->smallest_x;
		this->min_y = this->smallest_y;

		const BorderSpriteData *bsd = nullptr;
		uint8 pressable = 0; // Add extra space for a pressable widget.
		switch (this->wtype) {
			case WT_TITLEBAR:
				bsd = &_gui_sprites.titlebar;
				break;

			case WT_LEFT_TEXT:
			case WT_CENTERED_TEXT:
			case WT_RIGHT_TEXT:
				break;

			case WT_TEXT_BUTTON:
			case WT_TEXT_PUSHBUTTON:
				bsd = &_gui_sprites.button;
				pressable = 1;
				break;

			case WT_TEXT_TAB:
				bsd = &_gui_sprites.tab_tabbar;
				pressable = 1;
				break;

			case WT_IMAGE_TAB: {
				const Rectangle16 &rect = _sprite_manager.GetTableSpriteSize(this->value);
				this->value_width = rect.width;
				this->value_height = rect.height;
				this->InitMinimalSize(&_gui_sprites.tab_tabbar, this->value_width + 1, this->value_height + 1);
				if (this->number >= 0) w->UpdateWidgetSize(this->number, this);
				return;
			}

			case WT_IMAGE_BUTTON:
			case WT_IMAGE_PUSHBUTTON: {
				const Rectangle16 &rect = _sprite_manager.GetTableSpriteSize(this->value);
				this->value_width = rect.width;
				this->value_height = rect.height;
				this->InitMinimalSize(&_gui_sprites.button, this->value_width + 1, this->value_height + 1);
				if (this->number >= 0) w->UpdateWidgetSize(this->number, this);
				return;
			}

			case WT_IMAGE_DROPDOWN_BUTTON: {
				const Rectangle16 rect1 = _sprite_manager.GetTableSpriteSize(this->value);
				const Rectangle16 rect2 = _sprite_manager.GetTableSpriteSize(SPR_GUI_TRIANGLE_DOWN);
				this->value_width  = std::max(rect1.width,  rect2.width );
				this->value_height = std::max(rect1.height, rect2.height);
				this->InitMinimalSize(&_gui_sprites.button, this->value_width + 1, this->value_height + 1);
				if (this->number >= 0) w->UpdateWidgetSize(this->number, this);
				return;
			}

			case WT_DROPDOWN_BUTTON: {
				const Rectangle16 &rect = _sprite_manager.GetTableSpriteSize(SPR_GUI_TRIANGLE_DOWN);
				if (this->number >= 0) w->SetWidgetStringParameters(this->number);
				GetTextSize(w->TranslateStringNumber(this->value), &this->value_width, &this->value_height);
				this->value_width += rect.width;
				this->value_height = std::max(this->value_height, (int)rect.height);
				this->InitMinimalSize(&_gui_sprites.button, this->value_width + 1, this->value_height + 1);
				if (this->number >= 0) w->UpdateWidgetSize(this->number, this);
				return;
			}

			default:
				NOT_REACHED();
		}

		if (this->number >= 0) w->SetWidgetStringParameters(this->number);
		this->value_width = 0;
		this->value_height = 0;
		if (this->value != STR_NULL) {
			std::string rendered_text = DrawText(w->TranslateStringNumber(this->value));
			for (;;) {
				int w, h;
				const size_t pos = rendered_text.find('\n');
				_video.GetTextSize(rendered_text.substr(0, pos), &w, &h);
				this->value_width = std::max(this->value_width, w);
				this->value_height += h;
				if (pos == std::string::npos) break;
				rendered_text.erase(0, pos + 1);
			}
		}
		if (bsd != nullptr) {
			this->InitMinimalSize(bsd, this->value_width + pressable, this->value_height + pressable);
		} else {
			this->InitMinimalSize(this->value_width + pressable, this->value_height + pressable, 0, 0);
		}
	} else {
		this->min_x = 0;
		this->min_y = 0;
		this->fill_x = 0;
		this->fill_y = 0;
		this->resize_x = 0;
		this->resize_y = 0;
	}

	if (this->number >= 0) w->UpdateWidgetSize(this->number, this);
}

/**
 * Draw a #DataWidget.
 * @param w Window that the widget belongs to.
 * @todo Fix the hard-coded colour of the text.
 */
void DataWidget::DoDraw(const GuiWindow *w)
{
	_str_params.Clear();
	const BorderSpriteData *bsd = nullptr;
	uint8 pressed = 0;
	switch (this->wtype) {
		case WT_TITLEBAR:
			bsd = &_gui_sprites.titlebar;
			break;

		case WT_LEFT_TEXT:
		case WT_CENTERED_TEXT:
		case WT_RIGHT_TEXT:
			break;

		case WT_TEXT_BUTTON:
		case WT_IMAGE_BUTTON:
		case WT_TEXT_PUSHBUTTON:
		case WT_IMAGE_PUSHBUTTON:
		case WT_DROPDOWN_BUTTON:
		case WT_IMAGE_DROPDOWN_BUTTON:
			bsd = &_gui_sprites.button;
			pressed = this->IsPressed() ? 1 : 0;
			break;

		case WT_TEXT_TAB:
		case WT_IMAGE_TAB:
			bsd = &_gui_sprites.tab_tabbar;
			pressed = this->IsPressed() ? 1 : 0;
			break;

		default:
			NOT_REACHED();
	}
	int left   = w->GetWidgetScreenX(this) + this->paddings[PAD_LEFT];
	int top    = w->GetWidgetScreenY(this) + this->paddings[PAD_TOP];
	int right  = w->GetWidgetScreenX(this) + this->pos.width  - 1 - this->paddings[PAD_RIGHT];
	int bottom = w->GetWidgetScreenY(this) + this->pos.height - 1 - this->paddings[PAD_BOTTOM];
	Rectangle32 border_rect;
	if (bsd != nullptr) {
		left += bsd->border_left;
		top += bsd->border_top;
		right -= bsd->border_right;
		bottom -= bsd->border_bottom;
		assert(right - left + 1 >= 0);
		assert(bottom - top + 1 >= 0);

		border_rect.base.x = left;
		border_rect.base.y = top;
		border_rect.width = right - left + 1;
		border_rect.height = bottom - top + 1;

		DrawBorderSprites(*bsd, (pressed != 0), border_rect, this->colour);
	}
	Alignment align = ALG_CENTER;
	if (this->wtype == WT_LEFT_TEXT) {
		align = ALG_LEFT;
	} else if (this->wtype == WT_RIGHT_TEXT) {
		align = ALG_RIGHT;
	}
	int yoffset = top + (bottom + 1 - top - this->value_height) / 2;
	switch (this->wtype) {
		case WT_IMAGE_TAB:
		case WT_IMAGE_BUTTON:
		case WT_IMAGE_PUSHBUTTON: {
			const Rectangle16 rect = _sprite_manager.GetTableSpriteSize(this->value);
			int xoffset = left + (right + 1 - left - this->value_width) / 2 - rect.base.x;
			yoffset -= rect.base.y;
			const ImageData *imgdata = _sprite_manager.GetTableSprite(this->value);
			if (imgdata != nullptr) _video.BlitImage({xoffset + pressed, yoffset + pressed}, imgdata);
			break;
		}

		case WT_IMAGE_DROPDOWN_BUTTON: {
			const Rectangle16 rect = _sprite_manager.GetTableSpriteSize(this->value);
			int xoffset = left + (right + 1 - left - this->value_width) / 2 - rect.base.x;
			yoffset -= rect.base.y;
			const ImageData *imgdata = _sprite_manager.GetTableSprite(this->value);
			if (imgdata != nullptr) _video.BlitImage({xoffset + pressed, yoffset + pressed}, imgdata);

			const Rectangle16 imgrect = _sprite_manager.GetTableSpriteSize(SPR_GUI_TRIANGLE_DOWN);
			imgdata = _sprite_manager.GetTableSprite(SPR_GUI_TRIANGLE_DOWN);
			if (imgdata != nullptr) {
				int triangle_yoff = top + (bottom + 1 - top - imgrect.height) / 2 + pressed;
				_video.BlitImage({right - imgrect.width + pressed, triangle_yoff}, imgdata);
			}
			break;
		}

		case WT_DROPDOWN_BUTTON: {
			const Rectangle16 imgrect = _sprite_manager.GetTableSpriteSize(SPR_GUI_TRIANGLE_DOWN);
			const ImageData *imgdata = _sprite_manager.GetTableSprite(SPR_GUI_TRIANGLE_DOWN);
			int unused, text_w;
			if (this->number >= 0) w->SetWidgetStringParameters(this->number);
			GetTextSize(w->TranslateStringNumber(this->value), &text_w, &unused);

			if (imgdata != nullptr) {
				int triangle_yoff = top + (bottom + 1 - top - imgrect.height) / 2 + pressed;
				_video.BlitImage({right - imgrect.width + pressed, triangle_yoff}, imgdata);
			}
			/* Note: Reusing the same string parameters from above */
			if (this->value != STR_NULL) DrawString(w->TranslateStringNumber(this->value), TEXT_WHITE, left + pressed, yoffset + pressed, right - left - imgrect.width, align);
			break;
		}

		default:
			if (this->number >= 0) w->SetWidgetStringParameters(this->number);
			if (this->value != STR_NULL) {
				std::string rendered_text = DrawText(w->TranslateStringNumber(this->value));
				for (int y = yoffset + pressed - GetTextHeight(); y += GetTextHeight();) {
					const size_t pos = rendered_text.find('\n');
					DrawString(rendered_text.substr(0, pos), TEXT_WHITE, left + pressed, y, right - left, align, this->wtype == WT_TITLEBAR);
					if (pos == std::string::npos) break;
					rendered_text.erase(0, pos + 1);
				}
			}
			break;
	}
	if (this->number > 0) w->DrawWidget(this->number, this);
	if (bsd != nullptr && this->IsShaded()) OverlayShaded(border_rect);
}

static const int TEXT_INPUT_MARGIN = 2;  ///< Margin around a text input field.

TextInputWidget::TextInputWidget(WidgetType wtype) : LeafWidget(wtype), cursor_pos(0), has_focus(false)
{
	this->SetText("");
}

/**
 * Move the cursor.
 * @param pos New cursor position.
 */
void TextInputWidget::SetCursorPos(const size_t pos)
{
	assert(pos <= this->buffer.size());
	this->cursor_pos = pos;
}

/**
 * Retrieve the currently held text. Do not modify the returned pointer.
 * @return The text.
 */
const std::string &TextInputWidget::GetText() const
{
	return this->buffer;
}

/**
 * Change the currently held text.
 * @param text New text.
 */
void TextInputWidget::SetText(const std::string &text)
{
	this->buffer = text;
	this->cursor_pos = std::min(this->cursor_pos, this->buffer.size());
	if (this->text_changed) this->text_changed();
}

/**
 * Grab or release the keyboard focus.
 * @param focus Widget should be focused.
 */
void TextInputWidget::SetFocus(bool focus)
{
	this->has_focus = focus;
}

bool TextInputWidget::OnClick([[maybe_unused]] const Point32 &base, const Point16 &pos)
{
	this->has_focus = true;

	/* Find clicked cursor pos. */
	const int text_len = this->buffer.size();
	int w = 0;
	int xpos = pos.x;
	int smallest_distance = INT_MAX;
	for (int p = 0; p <= text_len; ++p) {
		if (p < text_len) _video.GetTextSize({this->buffer.at(p)}, &w, nullptr, false);
		int distance = abs(w / 2 - xpos);
		if (distance < smallest_distance) {
			smallest_distance = distance;
			this->cursor_pos = p;
			xpos -= w;
		} else {
			break;
		}
	}

	return true;
}

bool TextInputWidget::OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol)
{
	if (!this->visible || !this->has_focus) return false;
	switch (key_code) {
		case WMKC_CANCEL:
		case WMKC_CONFIRM:
			this->has_focus = false;
			return true;

		case WMKC_CURSOR_LEFT:
			if (this->cursor_pos > 0) {
				this->cursor_pos = GetPrevChar(this->buffer, this->cursor_pos);
			}
			return true;
		case WMKC_CURSOR_RIGHT:
			if (this->cursor_pos < this->buffer.size()) {
				this->cursor_pos = GetNextChar(this->buffer, this->cursor_pos);
			}
			return true;
		case WMKC_CURSOR_HOME:
			this->cursor_pos = 0;
			return true;
		case WMKC_CURSOR_END:
			this->cursor_pos = this->buffer.size();
			return true;

		case WMKC_BACKSPACE:
			if (this->cursor_pos > 0) {
				uint8 nr_chars_to_delete = this->cursor_pos - GetPrevChar(this->buffer, this->cursor_pos);
				if (nr_chars_to_delete == 0) return true;

				this->cursor_pos -= nr_chars_to_delete;
				this->buffer.erase(this->cursor_pos, nr_chars_to_delete);
				if (this->text_changed) this->text_changed();
			}
			return true;
		case WMKC_DELETE:
			if (this->cursor_pos < this->buffer.size()) {
				uint8 nr_chars_to_delete = GetNextChar(this->buffer, this->cursor_pos) - this->cursor_pos;
				if (nr_chars_to_delete == 0) return true;

				this->buffer.erase(this->cursor_pos, nr_chars_to_delete);
				if (this->text_changed) this->text_changed();
			}
			return true;

		case WMKC_SYMBOL: {
			if (symbol.empty() || mod != WMKM_NONE) break;

			this->buffer.insert(this->cursor_pos, symbol);
			this->cursor_pos += symbol.size();
			if (this->text_changed) this->text_changed();
			return true;
		}

		default: break;
	}
	return LeafWidget::OnKeyEvent(key_code, mod, symbol);
}

void TextInputWidget::DoDraw(const GuiWindow *w)
{
	Rectangle32 r = this->pos;
	r.base += w->rect.base;
	_video.FillRectangle(r, _palette[COL_SERIES_START + this->colour * COL_SERIES_LENGTH + (this->has_focus ? 2 : 4)]);

	/* Update text dimensions. */
	_video.GetTextSize(this->buffer, &this->value_width, &this->value_height);
	int cursor_offset;
	if (this->cursor_pos == 0) {
		cursor_offset = 0;
	} else if (this->cursor_pos == this->buffer.size()) {
		cursor_offset = this->value_width;
	} else {
		_video.GetTextSize(this->buffer.substr(0, this->cursor_pos), &cursor_offset, nullptr, false);
	}

	r.base.x += TEXT_INPUT_MARGIN;
	r.base.y += TEXT_INPUT_MARGIN;
	r.width -= 2 * TEXT_INPUT_MARGIN;
	r.height -= 2 * TEXT_INPUT_MARGIN;
	if (!this->buffer.empty()) {
		_video.BlitText(this->buffer, _palette[COL_SERIES_START + (this->colour + 1) * COL_SERIES_LENGTH - (this->has_focus ? 2 : 4)],
				r.base.x, r.base.y, r.width, ALG_LEFT);
	}

	if (this->has_focus) {
		_video.DrawLine(Point16(r.base.x + cursor_offset, r.base.y), Point16(r.base.x + cursor_offset, r.base.y + r.height),
				_palette[COL_SERIES_START + (this->colour + 1) * COL_SERIES_LENGTH - 1]);
	}
}

void TextInputWidget::SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array)
{
	if (!w->initialized) this->SetWidget(wid_array);
	if (this->visible) {
		this->min_x = this->smallest_x;
		this->min_y = this->smallest_y;

		_video.GetTextSize(this->buffer, &this->value_width, &this->value_height);
		this->InitMinimalSize(this->value_width, this->value_height, 0, 0);
	} else {
		this->min_x = 0;
		this->min_y = 0;
		this->fill_x = 0;
		this->fill_y = 0;
		this->resize_x = 0;
		this->resize_y = 0;
	}

	if (this->number >= 0) w->UpdateWidgetSize(this->number, this);
}

/**
 * Scrollbar widget constructor.
 * @param wtype %Widget type.
 */
ScrollbarWidget::ScrollbarWidget(WidgetType wtype) : LeafWidget(wtype), item_count(0), start(0), item_size(0), canvas(nullptr)
{
	this->receive_repeated_events = true;
}

/**
 * Set the widget being controlled by the scrollbar.
 * @param canvas %Widget being controlled by the scrollbar.
 */
void ScrollbarWidget::SetScrolled(const BaseWidget *canvas)
{
	this->canvas = canvas;
}

/**
 * Compute smallest size of the widget.
 * @param w %Window owning the widget.
 * @param wid_array [out] Array of widget pointers.
 */
void ScrollbarWidget::SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array)
{
	if (!w->initialized) this->SetWidget(wid_array);

	if (!this->visible) {
		this->min_x = 0;
		this->min_y = 0;
		this->fill_x = 0;
		this->fill_y = 0;
		this->resize_x = 0;
		this->resize_y = 0;
	} else if (this->wtype == WT_HOR_SCROLLBAR) {
		this->min_x = _gui_sprites.hor_scroll.min_length_all;
		this->min_y = _gui_sprites.hor_scroll.height;
		this->fill_x = _gui_sprites.hor_scroll.stepsize_bar;
		this->fill_y = 0;
		this->resize_x = _gui_sprites.hor_scroll.stepsize_bar;
		this->resize_y = 0;
	} else {
		this->min_x = _gui_sprites.vert_scroll.height;
		this->min_y = _gui_sprites.vert_scroll.min_length_all;
		this->fill_x = 0;
		this->fill_y = _gui_sprites.vert_scroll.stepsize_bar;
		this->resize_x = 0;
		this->resize_y = _gui_sprites.vert_scroll.stepsize_bar;
	}
}

void ScrollbarWidget::DoDraw(const GuiWindow *w)
{
	static Recolouring rc; // Only COL_RANGE_BROWN is modified each time.
	rc.Set(0, RecolourEntry(COL_RANGE_BROWN, this->colour));

	const ScrollbarSpriteData &scroll_sprites = (this->wtype == WT_HOR_SCROLLBAR) ? _gui_sprites.hor_scroll : _gui_sprites.vert_scroll;
	if (!scroll_sprites.IsLoaded()) return;

	const ImageData * const *imd = this->IsShaded() ? scroll_sprites.shaded : scroll_sprites.normal;

	Point32 pos(w->GetWidgetScreenX(this), w->GetWidgetScreenY(this));

	/* Draw left/up button. */
	_video.BlitImage(pos, imd[WLS_LEFT_BUTTON], rc, GS_NORMAL);
	if (this->wtype == WT_HOR_SCROLLBAR) pos.x += imd[WLS_LEFT_BUTTON]->width;
	if (this->wtype != WT_HOR_SCROLLBAR) pos.y += imd[WLS_LEFT_BUTTON]->height;

	/* Draw top/left underground. */
	_video.BlitImage(pos, imd[WLS_LEFT_BED], rc, GS_NORMAL);
	if (this->wtype == WT_HOR_SCROLLBAR) pos.x += imd[WLS_LEFT_BED]->width;
	if (this->wtype != WT_HOR_SCROLLBAR) pos.y += imd[WLS_LEFT_BED]->height;

	/* Draw middle underground. */
	if (this->wtype == WT_HOR_SCROLLBAR) {
		int others = imd[WLS_LEFT_BUTTON]->width + imd[WLS_LEFT_BED]->width + imd[WLS_RIGHT_BED]->width + imd[WLS_RIGHT_BUTTON]->width;
		if (others < this->pos.width) {
			_video.TileImage(imd[WLS_MIDDLE_BED], Rectangle32(
					pos.x + imd[WLS_MIDDLE_BED]->xoffset, pos.y + imd[WLS_MIDDLE_BED]->yoffset,
					this->pos.width - others, imd[WLS_MIDDLE_BED]->height), true, false, rc);
			pos.x += this->pos.width - others;
		}
	} else {
		int others = imd[WLS_LEFT_BUTTON]->height + imd[WLS_LEFT_BED]->height + imd[WLS_RIGHT_BED]->height + imd[WLS_RIGHT_BUTTON]->height;
		if (others < this->pos.height) {
			_video.TileImage(imd[WLS_MIDDLE_BED], Rectangle32(
					pos.x + imd[WLS_MIDDLE_BED]->xoffset, pos.y + imd[WLS_MIDDLE_BED]->yoffset,
					imd[WLS_MIDDLE_BED]->width, this->pos.height - others), false, true, rc);
			pos.y += this->pos.height - others;
		}
	}

	/* Draw bottom/right underground. */
	_video.BlitImage(pos, imd[WLS_RIGHT_BED], rc, GS_NORMAL);
	if (this->wtype == WT_HOR_SCROLLBAR) {
		pos.x += imd[WLS_RIGHT_BED]->width;
	} else {
		pos.y += imd[WLS_RIGHT_BED]->height;
	}

	/* Draw right/bottom button. */
	_video.BlitImage(pos, imd[WLS_RIGHT_BUTTON], rc, GS_NORMAL);

	int start_edge, slider_length;
	this->CalculateSliderPosition(&start_edge, &slider_length);

	if (this->wtype == WT_HOR_SCROLLBAR) {
		pos.x = w->GetWidgetScreenX(this) + imd[WLS_LEFT_BUTTON]->width + start_edge;
	} else {
		pos.y = w->GetWidgetScreenY(this) + imd[WLS_LEFT_BUTTON]->height + start_edge;
	}

	/* Draw top/left slider. */
	_video.BlitImage(pos, imd[WLS_LEFT_SLIDER], rc, GS_NORMAL);
	if (this->wtype == WT_HOR_SCROLLBAR) {
		pos.x += imd[WLS_LEFT_SLIDER]->width;
	} else {
		pos.y += imd[WLS_LEFT_SLIDER]->height;
	}

	/* Draw middle slider. */
	if (this->wtype == WT_HOR_SCROLLBAR) {
		int slider_size = slider_length - imd[WLS_LEFT_SLIDER]->width - imd[WLS_RIGHT_SLIDER]->width;
		if (slider_size > 0) {
			_video.TileImage(imd[WLS_MIDDLE_SLIDER], Rectangle32(
					pos.x + imd[WLS_MIDDLE_SLIDER]->xoffset, pos.y + imd[WLS_MIDDLE_SLIDER]->yoffset,
					slider_size, imd[WLS_MIDDLE_SLIDER]->height), true, false, rc);
			pos.x += slider_size;
		}
	} else {
		int slider_size = slider_length - imd[WLS_LEFT_SLIDER]->height - imd[WLS_RIGHT_SLIDER]->height;
		if (slider_size > 0) {
			_video.TileImage(imd[WLS_MIDDLE_SLIDER], Rectangle32(
					pos.x + imd[WLS_MIDDLE_SLIDER]->xoffset, pos.y + imd[WLS_MIDDLE_SLIDER]->yoffset,
					imd[WLS_MIDDLE_SLIDER]->width, slider_size), false, true, rc);
			pos.y += slider_size;
		}
	}

	/* Draw bottom/right slider. */
	_video.BlitImage(pos, imd[WLS_RIGHT_SLIDER], rc, GS_NORMAL);
}

/**
 * Decide which part of the scrollbar was clicked.
 * @param pos Clicked position in the scrollbar widget.
 * @return Component being clicked.
 */
ScrollbarComponent ScrollbarWidget::GetClickedComponent(const Point16 &pos)
{
	int start_edge, slider_length;
	this->CalculateSliderPosition(&start_edge, &slider_length);

	int location, size;
	if (this->wtype == WT_HOR_SCROLLBAR) {
		location = pos.x;
		size = this->pos.width;
	} else {
		location = pos.y;
		size = this->pos.height;
	}

	int dec_button_size = this->GetDecrementButtonSize();
	if (location < dec_button_size) return SBC_DECREMENT_BUTTON;
	if (location < dec_button_size + start_edge) return SBC_BEFORE_SLIDER;
	if (location < dec_button_size + start_edge + slider_length) return SBC_SLIDER;
	if (location < size - this->GetIncrementButtonSize()) return SBC_AFTER_SLIDER;
	return SBC_INCREMENT_BUTTON;
}

bool ScrollbarWidget::OnClick([[maybe_unused]] const Point32 &base, const Point16 &pos)
{
	switch (this->GetClickedComponent(pos)) {
		case SBC_INCREMENT_BUTTON:
			this->SetStart(this->start + 1);
			break;

		case SBC_DECREMENT_BUTTON:
			if (this->start > 0) {
				this->SetStart(this->start - 1);
			}
			break;

		case SBC_BEFORE_SLIDER:
			this->SetStart(this->start - std::min(this->start, this->GetVisibleCount()));
			break;

		case SBC_AFTER_SLIDER:
			this->SetStart(this->start + this->GetVisibleCount());
			break;

		default:
			/// \todo Implement clicking above at the slider.
			break;
	}
	return true;
}

bool ScrollbarWidget::OnMouseWheelEvent(int direction)
{
	if (!this->visible) return false;

	if (direction > 0 && this->start < static_cast<unsigned>(direction)) {
		this->SetStart(0);
	} else {
		this->SetStart(this->start - direction);
	}

	return true;
}

/**
 * Set the height/width of an item in the scrolled widget.
 * @param size Height or width of an item.
 */
void ScrollbarWidget::SetItemSize(uint size)
{
	this->item_size = size;
}

/**
 * Set the number of items displayed in the scrolled widget.
 * @param count Number of items.
 */
void ScrollbarWidget::SetItemCount(uint count)
{
	this->item_count = count;
	this->SetStart(this->start);
}

/**
 * Which row was clicked in the scrolled widget?
 * @param pos Clicked position in the scrolled widget (relative to top-left of the widget).
 * @return The index of the clicked item in the scrolled widget.
 */
uint ScrollbarWidget::GetClickedRow(const Point16 &pos) const
{
	uint itemsize = this->GetItemSize();
	if (this->wtype == WT_HOR_SCROLLBAR) {
		return this->GetStart() + pos.x / itemsize;
	} else {
		return this->GetStart() + pos.y / itemsize;
	}
}

/**
 * Get the height or width of an item.
 * @return Size of an item in the list.
 */
uint ScrollbarWidget::GetItemSize() const
{
	assert(this->canvas != nullptr);

	uint itemsize;
	if (this->item_size != 0) {
		return this->item_size;
	} else if (this->wtype == WT_HOR_SCROLLBAR) {
		itemsize = this->canvas->resize_x;
	} else {
		itemsize = this->canvas->resize_y;
	}
	assert(itemsize != 0);
	return itemsize;
}

/**
 * Get the number of items visible in the scrolled widget.
 * @return Number of visible items.
 */
uint ScrollbarWidget::GetVisibleCount() const
{
	assert(this->canvas != nullptr);
	uint itemsize = this->GetItemSize();

	uint count;
	if (this->wtype == WT_HOR_SCROLLBAR) {
		count = this->canvas->pos.width / itemsize;
	} else {
		count = this->canvas->pos.height / itemsize;
	}
	return (count != 0) ? count : 1;
}

/**
 * Set the number of the first visible item in the scrolled widget.
 * @param offset Suggested index.
 */
void ScrollbarWidget::SetStart(uint offset)
{
	uint visible_count = this->GetVisibleCount();
	uint max_start = (this->item_count > visible_count) ? this->item_count - visible_count : 0;
	this->start = std::min(offset, max_start);
}

/**
 * Get the index of the first visible item in the scrolled widget.
 * @return Index of the first visible item in the list.
 */
uint ScrollbarWidget::GetStart() const
{
	return this->start;
}

/**
 * Scroll the scrolled widget such that it shows item number \a offset.
 * @param offset Item number that should be visible in the scrolled widget.
 */
void ScrollbarWidget::ScrollTo(uint offset)
{
	if (offset >= this->item_count) offset = (this->item_count > 0) ? this->item_count - 1 : 0;

	if (offset < this->start) {
		this->SetStart(offset);
	} else {
		uint visible_count = this->GetVisibleCount();
		if (offset >= this->start + visible_count) this->SetStart(offset - visible_count);
	}
}

/**
 * Get the size of the left / top / decrement button in the direction of the scrollbar.
 * @return Size of the decrement button in the direction of the scrollbar.
 */
int ScrollbarWidget::GetDecrementButtonSize() const
{
	const ScrollbarSpriteData &scroll_sprites = (this->wtype == WT_HOR_SCROLLBAR) ? _gui_sprites.hor_scroll : _gui_sprites.vert_scroll;
	const ImageData * const *imd = this->IsShaded() ? scroll_sprites.shaded : scroll_sprites.normal;

	return (this->wtype == WT_HOR_SCROLLBAR) ? imd[WLS_LEFT_BUTTON]->width : imd[WLS_LEFT_BUTTON]->height;
}

/**
 * Get the size of the right / bottom / increment button in the direction of the scrollbar.
 * @return Size of the increment button in the direction of the scrollbar.
 */
int ScrollbarWidget::GetIncrementButtonSize() const
{
	const ScrollbarSpriteData &scroll_sprites = (this->wtype == WT_HOR_SCROLLBAR) ? _gui_sprites.hor_scroll : _gui_sprites.vert_scroll;
	const ImageData * const *imd = this->IsShaded() ? scroll_sprites.shaded : scroll_sprites.normal;

	return (this->wtype == WT_HOR_SCROLLBAR) ? imd[WLS_RIGHT_BUTTON]->width : imd[WLS_RIGHT_BUTTON]->height;
}

/**
 * Calculate the position and sizes of the displayed scrollbar components.
 * @param start_edge [out] Starting point of the slider in pixels, relative to the scrollbar top or left.
 * @param slider_length [out] Length of the slider in pixels, relative to the scrollbar top or left.
 */
void ScrollbarWidget::CalculateSliderPosition(int *start_edge, int *slider_length)
{
	int min_slider_length = this->GetDecrementButtonSize() + this->GetIncrementButtonSize();
	int range = ((this->wtype == WT_HOR_SCROLLBAR) ? this->pos.width : this->pos.height) - min_slider_length;
	uint visible_count = this->GetVisibleCount();

	if (this->item_count == 0 || visible_count == 0 || this->item_count <= visible_count) {
		*slider_length = range;
	} else {
		*slider_length = range * visible_count / this->item_count;
		if (*slider_length < min_slider_length) *slider_length = min_slider_length;
	}

	if (this->item_count <= visible_count) {
		*start_edge = 0;
	} else {
		*start_edge = (range - *slider_length) * this->start / (this->item_count - visible_count);
	}
}

/**
 * Base class background widget constructor.
 * @param wtype %Widget type.
 */
BackgroundWidget::BackgroundWidget(WidgetType wtype) : LeafWidget(wtype), child(nullptr)
{
}

/**
 * Compute smallest size of the widget.
 * @param w %Window owning the widget.
 * @param wid_array [out] Array of widget pointers.
 */
void BackgroundWidget::SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array)
{
	if (!w->initialized) this->SetWidget(wid_array);
	if (this->visible) {
		this->min_x = this->smallest_x;
		this->min_y = this->smallest_y;

		if (this->child != nullptr) {
			this->child->SetupMinimalSize(w, wid_array);
			this->min_x = this->child->min_x;
			this->min_y = this->child->min_y;
			this->fill_x = this->child->fill_x;
			this->fill_y = this->child->fill_y;
			this->resize_x = this->child->resize_x;
			this->resize_y = this->child->resize_y;
		}
		const BorderSpriteData *bsd = (this->wtype == WT_PANEL) ? &_gui_sprites.panel : &_gui_sprites.tabbar_panel;
		this->InitMinimalSize(bsd, this->min_x, this->min_y);
	} else {
		this->min_x = 0;
		this->min_y = 0;
		this->fill_x = 0;
		this->fill_y = 0;
		this->resize_x = 0;
		this->resize_y = 0;
	}
	if (this->child == nullptr && this->number >= 0) w->UpdateWidgetSize(this->number, this);
}

void BackgroundWidget::SetSmallestSizePosition(const Rectangle16 &rect)
{
	this->pos = rect;
	this->min_x = rect.width;
	this->min_y = rect.height;

	if (this->child != nullptr) {
		uint16 left = rect.base.x;
		uint16 right = left + rect.width; // One pixel further than right, actually.
		left += this->paddings[PAD_LEFT] + _gui_sprites.panel.border_left;
		right -= this->paddings[PAD_RIGHT] + _gui_sprites.panel.border_right;
		if (right < left) right = left;

		uint16 top = rect.base.y;
		uint16 bottom = top + rect.height; // One pixel below the bottom, actually.
		top += this->paddings[PAD_TOP] + _gui_sprites.panel.border_top;
		bottom -= this->paddings[PAD_BOTTOM] + _gui_sprites.panel.border_bottom;
		if (bottom < top) bottom = top;

		Rectangle16 rect_child(left, top, right - left, bottom - top);
		this->child->SetSmallestSizePosition(rect_child);
	}
}

void BackgroundWidget::DoDraw(const GuiWindow *w)
{
	int left   = w->GetWidgetScreenX(this) + this->paddings[PAD_LEFT];
	int top    = w->GetWidgetScreenY(this) + this->paddings[PAD_TOP];
	int right  = w->GetWidgetScreenX(this) + this->pos.width  - 1 - this->paddings[PAD_RIGHT];
	int bottom = w->GetWidgetScreenY(this) + this->pos.height - 1 - this->paddings[PAD_BOTTOM];
	left += _gui_sprites.panel.border_left;
	top += _gui_sprites.panel.border_top;
	right -= _gui_sprites.panel.border_right;
	bottom -= _gui_sprites.panel.border_bottom;
	assert(right - left + 1 >= 0);
	assert(bottom - top + 1 >= 0);

	Rectangle32 rect(left, top, right - left + 1, bottom - top + 1);
	const BorderSpriteData &bsd = (this->wtype == WT_PANEL) ? _gui_sprites.panel : _gui_sprites.tabbar_panel;
	DrawBorderSprites(bsd, false, rect, this->colour);

	if (this->number != INVALID_WIDGET_INDEX) w->DrawWidget(this->number, this);
	if (this->child != nullptr) this->child->Draw(w);
}

BaseWidget *BackgroundWidget::GetWidgetByPosition(const Point16 &pt)
{
	if (this->pos.IsPointInside(pt)) {
		if (this->child != nullptr) {
			BaseWidget *res = this->child->GetWidgetByPosition(pt);
			if (res != nullptr) return res;
		}
		return this;
	}
	return nullptr;
}

bool BackgroundWidget::OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol)
{
	if (!this->visible) return false;
	return (this->child != nullptr && this->child->OnKeyEvent(key_code, mod, symbol)) || LeafWidget::OnKeyEvent(key_code, mod, symbol);
}

bool BackgroundWidget::OnMouseWheelEvent(int direction)
{
	if (!this->visible) return false;
	return (this->child != nullptr && this->child->OnMouseWheelEvent(direction)) || LeafWidget::OnMouseWheelEvent(direction);
}

void BackgroundWidget::AutoRaiseButtons(const Point32 &base)
{
	if (this->child != nullptr) this->child->AutoRaiseButtons(base);
}

/** Initialize the row/column data. */
void RowColData::InitRowColData()
{
	this->min_size = 0;
	this->fill = 1;
	this->resize = 1;
}

/**
 * Merge a new minimal size, fill step, and resize step into the data.
 * @param min_size Minimal size to merge.
 * @param fill Fill step to merge.
 * @param resize Resize step to merge.
 */
void RowColData::Merge(uint16 min_size, uint16 fill, uint16 resize)
{
	this->min_size = std::max(this->min_size, min_size);
	this->fill = LeastCommonMultiple(this->fill, fill);
	this->resize = LeastCommonMultiple(this->resize, resize);
}

/**
 * Constructor for intermediate widgets.
 * @param num_rows Number of rows. Use \c 0 for 'manual' claiming.
 * @param num_cols Number of columns. Use \c 0 for 'manual' claiming.
 * @see FillWidget
 */
IntermediateWidget::IntermediateWidget(uint8 num_rows, uint8 num_cols) : BaseWidget(WT_GRID),
	childs(nullptr),
	rows(nullptr),
	columns(nullptr),
	num_rows(num_rows),
	num_cols(num_cols),
	flags(0)
{
	if (this->num_cols > 0 && this->num_rows > 0) this->ClaimMemory();
}

/**
 * Claim memory for child widgets 'manually'.
 * @pre Memory must not have been claimed, and #num_cols and #num_rows must be bigger than \c 0.
 */
void IntermediateWidget::ClaimMemory()
{
	assert(this->num_cols > 0 && this->num_rows > 0);
	assert(this->childs == nullptr);

	this->childs.reset(new std::unique_ptr<BaseWidget>[this->num_rows * this->num_cols]);
	assert(this->childs != nullptr);
	for (unsigned idx = 0; idx < static_cast<unsigned>(this->num_rows) * this->num_cols; idx++) {
		this->childs[idx] = nullptr;
	}

	this->rows.reset(new RowColData[this->num_rows]);
	assert(this->rows != nullptr);

	this->columns.reset(new RowColData[this->num_cols]);
	assert(this->columns != nullptr);
}

/**
 * Add a child widget.
 * @param x Horizontal index of the child in the grid.
 * @param y Vertical index of the child in the grid.
 * @param w Child widget to add.
 */
void IntermediateWidget::AddChild(uint8 x, uint8 y, BaseWidget *w)
{
	assert(x < this->num_cols && y < this->num_rows);
	assert(this->childs[y * (uint16)this->num_cols + x] == nullptr);
	this->childs[y * (uint16)this->num_cols + x].reset(w);
}

/**
 * Compute smallest size of the widget.
 * @param w %Window owning the widget.
 * @param wid_array [out] Array of widget pointers.
 */
void IntermediateWidget::SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array)
{
	if (!w->initialized) this->SetWidget(wid_array);
	if (!this->visible) {
		this->min_x = 0;
		this->min_y = 0;
		this->fill_x = 0;
		this->fill_y = 0;
		this->resize_x = 0;
		this->resize_y = 0;
		return;
	}

	this->min_x = this->smallest_x;
	this->min_y = this->smallest_y;

	/* Step 1: Initialize rows and columns. */
	for (uint8 y = 0; y < this->num_rows; y++) {
		this->rows[y].InitRowColData();
	}
	for (uint8 x = 0; x < this->num_cols; x++) {
		this->columns[x].InitRowColData();
	}

	/* Step 2: Process child widgets. */
	for (uint8 y = 0; y < this->num_rows; y++) {
		for (uint8 x = 0; x < this->num_cols; x++) {
			BaseWidget *bw = this->childs[y * (uint16)this->num_cols + x].get();
			bw->SetupMinimalSize(w, wid_array);
			this->rows[y].Merge(bw->min_y, bw->fill_y, bw->resize_y);
			this->columns[x].Merge(bw->min_x, bw->fill_x, bw->resize_x);
		}
	}

	/* Step 3a: Handle vertical equal size. */
	uint16 max_minsize = 0;
	if ((this->flags & EQS_VERTICAL) != 0) { // Equal sizes vertically requested, do a pre-size computation.
		for (uint8 y = 0; y < this->num_rows; y++) {
			max_minsize = std::max(max_minsize, this->rows[y].min_size);
		}
		for (uint8 y = 0; y < this->num_rows; y++) {
			if (this->rows[y].fill > 0) {
				uint16 diff = max_minsize - this->rows[y].min_size;
				this->rows[y].min_size += diff - diff % this->rows[y].fill;
			}
		}
	}

	uint16 child_tmp_minsize[50]; // Temporary array for minimal size of children. Size is arbitrary.
	assert(this->num_rows < lengthof(child_tmp_minsize));
	assert(this->num_cols < lengthof(child_tmp_minsize));

	/* Step 3b: Make sure we derive a minimal vertical size that is actually feasible for the child widgets. */
	for (uint8 y = 0; y < this->num_rows; y++) {
		/* Initialize the child_tmp_minsize array with current minsize of each child widget in the row. */
		for (uint8 x = 0; x < this->num_cols; x++) {
			BaseWidget *bw = this->childs[y * (uint16)this->num_cols + x].get();
			child_tmp_minsize[x] = bw->min_y;
		}
		/* Try to find a consistent minimal vertical size for all children. Due to a fill size > 1, this may be
		 * bigger than the current minimal vertical size.
		 */
		unsigned cur_minsize = this->rows[y].min_size; // the target we are aiming for currently.
		bool consistent = false;
		/* No overflow, and not insanely bigger (upper limit is arbitrary). */
		while (cur_minsize >= this->rows[y].min_size && this->rows[y].min_size + 50u > cur_minsize) {
			consistent = true;
			for (uint8 x = 0; x < this->num_cols; x++) {
				if (child_tmp_minsize[x] == cur_minsize) continue;

				BaseWidget *bw = this->childs[y * (uint16)this->num_cols + x].get();
				if (bw->fill_y <= 1) continue; // 0 does not play, 1 will always work

				/* Increment minimal size if y fill steps. */
				while (child_tmp_minsize[x] < cur_minsize) child_tmp_minsize[x] += bw->fill_y;
				consistent &= (child_tmp_minsize[x] == cur_minsize);
				assert(cur_minsize <= child_tmp_minsize[x]);
				cur_minsize = child_tmp_minsize[x]; // May cause increasing of cur_minsize.
			}
			if (consistent) break;
		}
		if (consistent) this->rows[y].min_size = cur_minsize;
	}

	/* Step 3c: Compute vertical fields in rows[y]. */
	this->min_y = this->paddings[PAD_BOTTOM];
	this->fill_y = 0;
	this->resize_y = 0;
	for (uint8 y = 0; y < this->num_rows; y++) {
		this->min_y += ((y == 0) ? this->paddings[PAD_TOP] : this->paddings[PAD_VERTICAL]) + this->rows[y].min_size;
		if (this->rows[y].fill > 0 && (this->fill_y == 0 || this->fill_y > this->rows[y].fill)) this->fill_y = this->rows[y].fill;
		if (this->rows[y].resize > 0 && (this->resize_y == 0 || this->resize_y > this->rows[y].resize)) {
			this->resize_y = this->rows[y].resize;
		}
	}

	/* Step 4a: Handle horizontal equal size. */
	max_minsize = 0;
	if ((this->flags & EQS_HORIZONTAL) != 0) { // Equal sizes vertically requested, do a pre-size computation.
		for (uint8 x = 0; x < this->num_cols; x++) {
			max_minsize = std::max(max_minsize, this->columns[x].min_size);
		}
		for (uint8 x = 0; x < this->num_cols; x++) {
			if (this->columns[x].fill > 0) {
				uint16 diff = max_minsize - this->columns[x].min_size;
				this->columns[x].min_size += diff - diff % this->columns[x].fill;
			}
		}
	}

	/* Step 4b: Make sure we derive a minimal horizontal size that is actually feasible for the child widgets. */
	for (uint8 x = 0; x < this->num_cols; x++) {
		/* Initialize the child_tmp_minsize array with current minsize of each child widget in the column. */
		for (uint8 y = 0; y < this->num_rows; y++) {
			BaseWidget *bw = this->childs[y * (uint16)this->num_cols + x].get();
			child_tmp_minsize[y] = bw->min_x;
		}
		/*
		 * Try to find a consistent minimal horizontal size for all children. Due to a fill size > 1, this may be
		 * bigger than the current minimal horizontal size.
		 */
		unsigned cur_minsize = this->columns[x].min_size; // the target we are aiming for currently.
		bool consistent = false;
		/* No overflow, and not insanely bigger (upper limit is arbitrary). */
		while (cur_minsize >= this->columns[x].min_size && this->columns[x].min_size + 50u > cur_minsize) {
			consistent = true;
			for (uint8 y = 0; y < this->num_rows; y++) {
				if (child_tmp_minsize[y] == cur_minsize) continue;

				BaseWidget *bw = this->childs[y * (uint16)this->num_cols + x].get();
				if (bw->fill_x <= 1) continue; // 0 does not play, 1 will always work

				/* Increment minimal size if y fill steps. */
				while (child_tmp_minsize[y] < cur_minsize) child_tmp_minsize[y] += bw->fill_x;
				consistent &= (child_tmp_minsize[y] == cur_minsize);
				assert(cur_minsize <= child_tmp_minsize[y]);
				cur_minsize = child_tmp_minsize[y]; // May cause increasing of cur_minsize.
			}
			if (consistent) break;
		}
		if (consistent) this->columns[x].min_size = cur_minsize;
	}

	/* Step 4c: Compute horizontal fields in columns[x]. */
	this->min_x = this->paddings[PAD_RIGHT];
	this->fill_x = 0;
	this->resize_x = 0;
	for (uint8 x = 0; x < this->num_cols; x++) {
		this->min_x += ((x == 0) ? this->paddings[PAD_LEFT] : this->paddings[PAD_HORIZONTAL]) + this->columns[x].min_size;
		if (this->columns[x].fill > 0 && (this->fill_x == 0 || this->fill_x > this->columns[x].fill)) this->fill_x = this->columns[x].fill;
		if (this->columns[x].resize > 0 && (this->resize_x == 0 || this->resize_x > this->columns[x].resize)) {
			this->resize_x = this->columns[x].resize;
		}
	}
}

/**
 * Set the smallest size of the intermediate widget (and all widgets under it).
 * @param rect Smallest size to set.
 * @todo Handle RTL languages too.
 */
void IntermediateWidget::SetSmallestSizePosition(const Rectangle16 &rect)
{
	this->pos = rect;

	/* Distribute additional vertical size over fillable children. */
	uint16 diff = this->paddings[PAD_BOTTOM];
	uint8 count = 0;
	uint16 max_step = 0;
	for (uint8 y = 0; y < this->num_rows; y++) {
		diff += ((y == 0) ? this->paddings[PAD_TOP] : this->paddings[PAD_VERTICAL]) + this->rows[y].min_size;
		if (this->rows[y].fill > 0) {
			if (count == 0 || this->rows[y].fill > max_step) max_step = this->rows[y].fill;
			count++;
		}
	}
	diff = (diff < rect.height) ? rect.height - diff : 0;

	while (diff > 0 && count > 0) {
		uint16 new_max = 0;
		for (uint8 y = 0; y < this->num_rows; y++) {
			if (this->rows[y].fill == 0 || this->rows[y].fill > max_step) continue;
			if (this->rows[y].fill == max_step) {
				uint16 increment = diff / count;
				increment -= increment % max_step;
				this->rows[y].min_size += increment;
				diff -= increment;
				count--;
				continue;
			}
			new_max = std::max(new_max, this->rows[y].fill);
		}
		max_step = new_max;
	}

	/* Distribute additional horizontal size over fillable children. */
	diff = this->paddings[PAD_RIGHT];
	count = 0;
	max_step = 0;
	for (uint8 x = 0; x < this->num_cols; x++) {
		diff += ((x == 0) ? this->paddings[PAD_LEFT] : this->paddings[PAD_HORIZONTAL]) + this->columns[x].min_size;
		if (this->columns[x].fill > 0) {
			if (count == 0 || this->columns[x].fill > max_step) max_step = this->columns[x].fill;
			count++;
		}
	}
	diff = (diff < rect.width) ? rect.width - diff : 0;

	while (diff > 0 && count > 0) {
		uint16 new_max = 0;
		for (uint8 x = 0; x < this->num_cols; x++) {
			if (this->columns[x].fill == 0 || this->columns[x].fill > max_step) continue;
			if (this->columns[x].fill == max_step) {
				uint16 increment = diff / count;
				increment -= increment % max_step;
				this->columns[x].min_size += increment;
				diff -= increment;
				count--;
				continue;
			}
			new_max = std::max(new_max, this->columns[x].fill);
		}
		max_step = new_max;
	}

	/* Tell the children about the allocated sizes. */
	uint16 top = rect.base.y;
	for (uint8 y = 0; y < this->num_rows; y++) {
		top += (y == 0) ? this->paddings[PAD_TOP] : this->paddings[PAD_VERTICAL];
		uint16 left = rect.base.x;
		for (uint8 x = 0; x < this->num_cols; x++) {
			left += (x == 0) ? this->paddings[PAD_LEFT] : this->paddings[PAD_HORIZONTAL];
			BaseWidget *bw = this->childs[y * (uint16)this->num_cols + x].get();
			Rectangle16 rect2(left, top, this->columns[x].min_size, this->rows[y].min_size);
			bw->SetSmallestSizePosition(rect2);

			left += this->columns[x].min_size;
		}
		top += this->rows[y].min_size;
	}
}

void IntermediateWidget::DoDraw(const GuiWindow *w)
{
	for (unsigned idx = 0; idx < static_cast<unsigned>(this->num_rows) * this->num_cols; idx++) {
		this->childs[idx]->Draw(w);
	}
}

BaseWidget *IntermediateWidget::FindTooltipWidget(Point16 pt)
{
	BaseWidget *w = this->GetWidgetByPosition(pt);
	return w == nullptr ? nullptr : w->FindTooltipWidget(pt);
}

bool IntermediateWidget::OnKeyEvent(WmKeyCode key_code, WmKeyMod mod, const std::string &symbol)
{
	if (!this->visible) return false;
	for (uint16 idx = 0; idx < static_cast<uint16>(this->num_rows * this->num_cols); idx++) {
		if (this->childs[idx]->OnKeyEvent(key_code, mod, symbol)) return true;
	}
	return BaseWidget::OnKeyEvent(key_code, mod, symbol);
}

bool IntermediateWidget::OnMouseWheelEvent(int direction)
{
	if (!this->visible) return false;
	for (uint16 idx = 0; idx < static_cast<uint16>(this->num_rows * this->num_cols); idx++) {
		if (this->childs[idx]->OnMouseWheelEvent(direction)) return true;
	}
	return BaseWidget::OnMouseWheelEvent(direction);
}

BaseWidget *IntermediateWidget::GetWidgetByPosition(const Point16 &pt)
{
	BaseWidget *res = nullptr;
	if (this->pos.IsPointInside(pt)) {
		for (unsigned idx = 0; idx < static_cast<unsigned>(this->num_rows) * this->num_cols; idx++) {
			res = this->childs[idx]->GetWidgetByPosition(pt);
			if (res != nullptr) break;
		}
	}
	return res;
}

void IntermediateWidget::AutoRaiseButtons(const Point32 &base)
{
	for (unsigned idx = 0; idx < static_cast<unsigned>(this->num_rows) * this->num_cols; idx++) {
		this->childs[idx]->AutoRaiseButtons(base);
	}
}

/**
 * Create a new #WT_GRID widget.
 * @param num_rows Number of rows.
 * @param num_cols Number of columns.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart Intermediate(uint8 num_rows, uint8 num_cols)
{
	WidgetPart part;

	part.type = WPT_NEW_INTERMEDIATE;
	part.data.new_intermediate.num_rows = num_rows;
	part.data.new_intermediate.num_cols = num_cols;
	return part;
}

/**
 * Create a new widget.
 * @param wtype Widget type.
 * @param number Widget number.
 * @param colour Widget colour.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart Widget(WidgetType wtype, WidgetNumber number, ColourRange colour)
{
	WidgetPart part;

	part.type = WPT_NEW_WIDGET;
	part.data.new_widget.wtype = wtype;
	part.data.new_widget.number = number;
	part.data.new_widget.colour = colour;
	return part;
}

/**
 * Set minimal size.
 * @param x Horizontal minimal size.
 * @param y Vertical minimal size.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart SetMinimalSize(int16 x, int16 y)
{
	WidgetPart part;

	assert(x > 0 && y > 0);
	part.type = WPT_MIN_SIZE;
	part.data.size.x = x;
	part.data.size.y = y;
	return part;
}

/**
 * Set whether the widget will receive repeating mouse events.
 * @param repeat Receive repeating events.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart SetRepeating(bool repeat)
{
	WidgetPart part;
	part.type = WPT_REPEATING;
	part.data.flags = repeat ? 1 : 0;
	return part;
}

/**
 * Set fill step.
 * @param x Horizontal fill step.
 * @param y Vertical fill step.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart SetFill(uint8 x, uint8 y)
{
	WidgetPart part;

	part.type = WPT_FILL;
	part.data.size.x = x;
	part.data.size.y = y;
	return part;
}

/**
 * Set resize step.
 * @param x Horizontal resize step.
 * @param y Vertical resize step.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart SetResize(uint8 x, uint8 y)
{
	WidgetPart part;

	part.type = WPT_RESIZE;
	part.data.size.x = x;
	part.data.size.y = y;
	return part;
}

/**
 * Set padding around a widget (start upwards, and then around with the clock).
 * @param top Padding at the top.
 * @param right Padding at the right.
 * @param bottom Padding at the bottom.
 * @param left Padding at the left.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart SetPadding(uint8 top, uint8 right, uint8 bottom, uint8 left)
{
	WidgetPart part;

	part.type = WPT_PADDING;
	for (int i = 0; i < PAD_COUNT; i++) part.data.padding[i] = 0;
	part.data.padding[PAD_TOP] = top;
	part.data.padding[PAD_LEFT] = left;
	part.data.padding[PAD_RIGHT] = right;
	part.data.padding[PAD_BOTTOM] = bottom;
	return part;
}

/**
 * Set padding of a horizontal bar.
 * @param pre Padding at the left.
 * @param inter Vertical padding between elements.
 * @param post Padding at the right.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart SetHorPIP(uint8 pre, uint8 inter, uint8 post)
{
	WidgetPart part;

	part.type = WPT_HOR_PIP;
	for (int i = 0; i < PAD_COUNT; i++) part.data.padding[i] = 0;
	part.data.padding[PAD_LEFT] = pre;
	part.data.padding[PAD_RIGHT] = post;
	part.data.padding[PAD_VERTICAL] = inter;
	return part;
}

/**
 * Set padding of a vertical bar.
 * @param pre Padding at the top.
 * @param inter Horizontal padding between elements.
 * @param post Padding at the bottom.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart SetVertPIP(uint8 pre, uint8 inter, uint8 post)
{
	WidgetPart part;

	part.type = WPT_VERT_PIP;
	for (int i = 0; i < PAD_COUNT; i++) part.data.padding[i] = 0;
	part.data.padding[PAD_TOP] = pre;
	part.data.padding[PAD_BOTTOM] = post;
	part.data.padding[PAD_HORIZONTAL] = inter;
	return part;
}

/**
 * Set data and tool tip of the widget.
 * @param value Additional data of the widget (string or sprite id).
 * @param tip Tool tip text.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart SetData(uint16 value, uint16 tip)
{
	WidgetPart part;

	part.type = WPT_DATA;
	part.data.dat.value = value;
	part.data.dat.tip = tip;
	return part;
}

/**
 * Define equal size of child widgets for intermediate widgets (in both directions).
 * @param hor_equal Try to keep all child widgets equally wide.
 * @param vert_equal Try to keep all child widgets equally high.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart SetEqualSize(bool hor_equal, bool vert_equal)
{
	WidgetPart part;

	part.type = WPT_EQUAL_SIZE;
	part.data.flags = (hor_equal ? EQS_HORIZONTAL : 0) | (vert_equal ? EQS_VERTICAL : 0);
	return part;
}

/**
 * Denote the end of a container.
 * @return Widget part containing the provided data for storage in an array.
 * @ingroup widget_parts_group
 */
WidgetPart EndContainer()
{
	WidgetPart part;

	part.type = WPT_END_CON;
	return part;
}

/**
 * Construct a widget from widget parts.
 * @param parts Base of parts array.
 * @param remaining Number of parts still available.
 * @param dest Pointer for storing the constructed widget.
 * @return Read number of parts.
 * @ingroup widget_parts_group
 */
static int MakeWidget(const WidgetPart *parts, int remaining, BaseWidget **dest)
{
	int num_used = 0;

	*dest = nullptr;
	while (num_used < remaining) {
		switch (parts->type) {
			case WPT_NEW_WIDGET: {
				if (*dest != nullptr) return num_used;
				switch (parts->data.new_widget.wtype) {
					case WT_EMPTY:
						*dest = new BaseWidget(parts->data.new_widget.wtype);
						break;

					case WT_PANEL:
					case WT_TAB_PANEL:
						*dest = new BackgroundWidget(parts->data.new_widget.wtype);
						break;

					case WT_TEXT_BUTTON:
					case WT_IMAGE_BUTTON:
					case WT_TEXT_TAB:
					case WT_IMAGE_TAB:
					case WT_TEXT_PUSHBUTTON:
					case WT_IMAGE_PUSHBUTTON:
					case WT_DROPDOWN_BUTTON:
					case WT_IMAGE_DROPDOWN_BUTTON:
					case WT_TITLEBAR:
					case WT_LEFT_TEXT:
					case WT_CENTERED_TEXT:
					case WT_RIGHT_TEXT:
						*dest = new DataWidget(parts->data.new_widget.wtype);
						break;

					case WT_TEXT_INPUT:
						*dest = new TextInputWidget(parts->data.new_widget.wtype);
						break;

					case WT_RADIOBUTTON:
					case WT_CHECKBOX:
					case WT_CLOSEBOX:
					case WT_RESIZEBOX:
					case WT_LEFT_FILLER_TAB:
					case WT_RIGHT_FILLER_TAB:
						*dest = new LeafWidget(parts->data.new_widget.wtype);
						break;

					case WT_HOR_SCROLLBAR:
					case WT_VERT_SCROLLBAR:
						*dest = new ScrollbarWidget(parts->data.new_widget.wtype);
						break;

					default: NOT_REACHED();
				}
				if (parts->data.new_widget.number >= 0) (*dest)->number = parts->data.new_widget.number;
				LeafWidget *lw = dynamic_cast<LeafWidget *>(*dest);
				if (lw != nullptr) lw->colour = parts->data.new_widget.colour;
				break;
			}

			case WPT_NEW_INTERMEDIATE:
				if (*dest != nullptr) return num_used;
				(*dest) = new IntermediateWidget(parts->data.new_intermediate.num_rows, parts->data.new_intermediate.num_cols);
				break;

			case WPT_MIN_SIZE: {
				if (*dest == nullptr) break;
				BaseWidget *bw = dynamic_cast<BaseWidget *>(*dest);
				if (bw != nullptr) {
					bw->smallest_x = parts->data.size.x;
					bw->smallest_y = parts->data.size.y;
				}
				break;
			}

			case WPT_REPEATING: {
				if (*dest == nullptr) break;
				BaseWidget *bw = dynamic_cast<BaseWidget *>(*dest);
				if (bw != nullptr) {
					bw->receive_repeated_events = parts->data.flags != 0;
				}
				break;
			}

			case WPT_FILL: {
				if (*dest == nullptr) break;
				BaseWidget *bw = dynamic_cast<BaseWidget *>(*dest);
				if (bw != nullptr) {
					bw->fill_x = parts->data.size.x;
					bw->fill_y = parts->data.size.y;
				}
				break;
			}

			case WPT_RESIZE: {
				if (*dest == nullptr) break;
				BaseWidget *bw = dynamic_cast<BaseWidget *>(*dest);
				if (bw != nullptr) {
					bw->resize_x = parts->data.size.x;
					bw->resize_y = parts->data.size.y;
				}
				break;
			}

			case WPT_PADDING:
			case WPT_HOR_PIP:
			case WPT_VERT_PIP: {
				if (*dest == nullptr) break;
				BaseWidget *bw = dynamic_cast<BaseWidget *>(*dest);
				if (bw != nullptr) {
					for (int i = 0; i < PAD_COUNT; i++) bw->paddings[i] += parts->data.padding[i];
				}
				break;
			}

			case WPT_DATA: {
				if (*dest == nullptr) break;
				(*dest)->tooltip = parts->data.dat.tip;
				DataWidget *bw = dynamic_cast<DataWidget *>(*dest);
				if (bw != nullptr) bw->value = parts->data.dat.value;
				break;
			}

			case WPT_EQUAL_SIZE: {
				if (*dest == nullptr) break;
				IntermediateWidget *iw = dynamic_cast<IntermediateWidget *>(*dest);
				if (iw != nullptr) iw->flags = parts->data.flags;
				break;
			}

			case WPT_END_CON:
				return num_used;

			default: NOT_REACHED();
		}
		num_used++;
		parts++;
	}
	return num_used;
}

static int MakeWidgetSubTree(const WidgetPart *parts, int remaining, BaseWidget **dest, int16 *biggest);

/**
 * Fill a row of an intermediate widget with its children.
 * @param parts Parts to assemble into child widgets of the row.
 * @param remaining_parts Number of parts still available.
 * @param widgets Temporary storage for the child widgets.
 * @param remaining_widgets Amount of space left for the child widgets.
 * @param cols [inout] Number of elements of a row. \c 0 means it is not known.
 * @param biggest [out] Pointer to stored biggest widget index number.
 * @return Number of used parts.
 * @ingroup widget_parts_group
 */
static int FillWidgetRow(const WidgetPart *parts, int remaining_parts, BaseWidget **widgets, int remaining_widgets, uint8 *cols, int16 *biggest)
{
	int total_used = 0;

	uint8 c = 0;
	for (;;) {
		if (remaining_parts == 0 || remaining_widgets == 0) break;
		if (parts->type == WPT_END_CON) break;

		int used = MakeWidgetSubTree(parts, remaining_parts, widgets, biggest);
		parts += used;
		remaining_parts -= used;
		total_used += used;
		if (*widgets == nullptr) break;

		widgets++;
		remaining_widgets--;

		if (c == 255) break;
		c++;
		if (*cols == c) break;
	}

	if (*cols == 0) {
		assert(c > 0);
		*cols = c;
		if (remaining_parts > 0 && parts->type == WPT_END_CON) return total_used + 1;
	}
	return total_used;
}

/**
 * Fill an intermediate widget with its children.
 * @param parts Parts to assemble into children of the widget.
 * @param remaining_parts Number of parts still available.
 * @param wid Intermediate widget to fill.
 * @param biggest [out] Pointer to stored biggest widget index number.
 * @return Number of used parts for the children.
 * @ingroup widget_parts_group
 */
static int FillWidget(const WidgetPart *parts, int remaining_parts, IntermediateWidget *wid, int16 *biggest)
{
	int total_used = 0;

	static const int MAX_CHILDS = 500;
	BaseWidget *grid[500];
	for (int i = 0; i < 500; i++) grid[i] = nullptr;

	bool need_claim_memory = wid->num_rows == 0 || wid->num_cols == 0;

	int remaining_widgets = MAX_CHILDS;
	BaseWidget **widgets = &grid[0];
	uint8 r = 0;
	for (;;) {
		int used = FillWidgetRow(parts, remaining_parts, widgets, remaining_widgets, &wid->num_cols, biggest);
		parts += used;
		remaining_parts -= used;
		total_used += used;

		remaining_widgets -= wid->num_cols;
		widgets += wid->num_cols;

		if (r == 255) break;
		r++;
		if (r == wid->num_rows) break;

		if (wid->num_rows == 0 && (remaining_parts == 0 || parts->type == WPT_END_CON)) break;
	}

	if (wid->num_rows == 0) {
		assert(r > 0);
		wid->num_rows = r;
		if (remaining_parts > 0 && parts->type == WPT_END_CON) total_used++;
	}

	if (need_claim_memory) wid->ClaimMemory();
	assert(remaining_widgets == MAX_CHILDS - (int)wid->num_rows * wid->num_cols);

	widgets = &grid[0];
	for (uint8 r = 0; r < wid->num_rows; r++) {
		for (uint8 c = 0; c < wid->num_cols; c++) {
			wid->AddChild(c, r, *widgets);
			widgets++;
		}
	}

	return total_used;
}

/**
 * Build a (sub) tree of widgets.
 * @param parts Parts to assemble into a tree.
 * @param remaining Number of parts still available.
 * @param dest Pointer for storing the constructed widget tree.
 * @param biggest [out] Pointer to stored biggest widget index number.
 * @return Number of used parts for the tree.
 * @ingroup widget_parts_group
 */
static int MakeWidgetSubTree(const WidgetPart *parts, int remaining, BaseWidget **dest, int16 *biggest)
{
	int total_used = 0;

	*dest = nullptr;
	int used = MakeWidget(parts, remaining, dest);
	parts += used;
	remaining -= used;
	total_used += used;

	if (*dest == nullptr) return total_used;

	*biggest = std::max(*biggest, (*dest)->number); // Update biggest widget number.

	if ((*dest)->wtype == WT_PANEL || (*dest)->wtype == WT_TAB_PANEL) {
		/* Panel widget. */
		BackgroundWidget *bg = static_cast<BackgroundWidget *>(*dest);
		if (remaining > 0 && parts->type == WPT_END_CON) {
			used = 1;
		} else {
			BaseWidget *pointer;
			used = MakeWidgetSubTree(parts, remaining, &pointer, biggest);
			bg->child.reset(pointer);
		}
		total_used += used;
	} else if ((*dest)->wtype == WT_GRID) {
		/* Container widget; also load child widgets. */
		IntermediateWidget *iw = static_cast<IntermediateWidget *>(*dest);
		used = FillWidget(parts, remaining, iw, biggest);
		total_used += used;
	}
	return total_used;
}

/**
 * Construct a widget tree from its parts.
 * @param parts Parts to assemble into a tree.
 * @param length Number of parts available.
 * @param biggest [out] Pointer to stored biggest widget index number.
 * @return Constructed widget tree.
 * @ingroup widget_parts_group
 */
BaseWidget *MakeWidgetTree(const WidgetPart *parts, int length, int16 *biggest)
{
	BaseWidget *root = nullptr;
	*biggest = INVALID_WIDGET_INDEX;
	MakeWidgetSubTree(parts, length, &root, biggest);
	return root;
}

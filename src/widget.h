/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file widget.h Widget declarations. */

#ifndef WIDGET_H
#define WIDGET_H

#include "enum_type.h"
#include "geometry.h"
#include "language.h"
#include "gui_graphics.h"

struct BorderSpriteData;
class GuiWindow;

typedef int16 WidgetNumber; ///< Type of a widget number.

static const WidgetNumber INVALID_WIDGET_INDEX = -1; ///< Widget number of invalid index.

/**
 * Available widget types.
 * @ingroup widget_group
 */
enum WidgetType {
	WT_EMPTY,            ///< Empty widget (used for creating empty space and/or centring).
	WT_TITLEBAR,         ///< Title of the window.
	WT_CLOSEBOX,         ///< Close box.
	WT_RESIZEBOX,        ///< Resize box.
	WT_LEFT_TEXT,        ///< Text label with left-aligned text.
	WT_CENTERED_TEXT,    ///< Text label with centred text.
	WT_RIGHT_TEXT,       ///< Text label with right-aligned text.
	WT_PANEL,            ///< Panel.
	WT_TEXT_BUTTON,      ///< Button with text (bi-stable).
	WT_IMAGE_BUTTON,     ///< Button with a sprite (bi-stable).
	WT_TEXT_PUSHBUTTON,  ///< Button with text (mono-stable).
	WT_IMAGE_PUSHBUTTON, ///< Button with a sprite (mono-stable).
	WT_RADIOBUTTON,      ///< Radio button widget.
	WT_HOR_SCROLLBAR,    ///< Scrollbar widget.
	WT_VERT_SCROLLBAR,   ///< Scrollbar widget.
	WT_DROPDOWN_BUTTON,  ///< Dropdown widget.
	WT_TEXT_TAB,         ///< Tab with text (centred).
	WT_IMAGE_TAB,        ///< Tab with image(s).
	WT_LEFT_FILLER_TAB,  ///< Filler widget at the left of the tab bar.
	WT_RIGHT_FILLER_TAB, ///< Filler widget at the right of the tab bar.
	WT_TAB_PANEL,
	WT_GRID,             ///< Intermediate widget.
	WT_EDIT_TEXT,        ///< Text box for text input.
};

/**
 * Padding space around widgets.
 * @ingroup widget_group
 */
enum PaddingDirection {
	PAD_TOP,        ///< Padding at the top.
	PAD_LEFT,       ///< Padding at the left.
	PAD_RIGHT,      ///< Padding at the right.
	PAD_BOTTOM,     ///< Padding at the bottom.
	PAD_VERTICAL,   ///< Inter-child vertical padding space.
	PAD_HORIZONTAL, ///< Inter-child horizontal padding space.

	PAD_COUNT,      ///< Number of paddings.
};

/**
 * Base class for all widgets.
 * Also implements #WT_EMPTY.
 * @ingroup widget_group
 */
class BaseWidget {
public:
	BaseWidget(WidgetType wtype);
	virtual ~BaseWidget();

	virtual void SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array);
	virtual void SetSmallestSizePosition(const Rectangle16 &rect);
	virtual void Draw(const GuiWindow *w);
	virtual BaseWidget *GetWidgetByPosition(const Point16 &pt);
	virtual void AutoRaiseButtons(const Point32 &base);

	void MarkDirty(const Point32 &base) const;

	WidgetType wtype;    ///< Widget type.
	WidgetNumber number; ///< Widget number.

	uint16 smallest_x;         ///< Original horizontal size.
	uint16 smallest_y;         ///< Original vertical size.
	uint16 min_x;              ///< Minimal horizontal size.
	uint16 min_y;              ///< Minimal vertical size.
	Rectangle16 pos;           ///< Current position and size (relative to window top-left edge).
	uint16 fill_x;             ///< Horizontal fill step.
	uint16 fill_y;             ///< Vertical fill step.
	uint16 resize_x;           ///< Horizontal resize step.
	uint16 resize_y;           ///< Vertical resize step.
	uint8 paddings[PAD_COUNT]; ///< Padding.

protected:
	void SetWidget(BaseWidget **wid_array);

	void InitMinimalSize(uint16 content_width, uint16 content_height, uint16 border_hor, uint16 border_vert);
	void InitMinimalSize(const BorderSpriteData *bsd, uint16 content_width, uint16 content_height);
};

/**
 * Flags of the #LeafWidget widget.
 * @ingroup widget_group
 */
enum LeafWidgetFlags {
	LWF_CHECKED = 1, ///< Widget is checked (on/off).
	LWF_PRESSED = 2, ///< Widget is pressed (button up/down).
	LWF_SHADED  = 4, ///< Widget is shaded (enabled/disabled).
};
DECLARE_ENUM_AS_BIT_SET(LeafWidgetFlags)

/**
 * Base class for a (visible) leaf widget.
 * Implements #WT_RADIOBUTTON, #WT_CLOSEBOX, #WT_RESIZEBOX.
 * @ingroup widget_group
 */
class LeafWidget : public BaseWidget {
public:
	LeafWidget(WidgetType wtype);

	virtual void SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array) override;
	virtual void Draw(const GuiWindow *w) override;
	virtual void AutoRaiseButtons(const Point32 &base) override;

	/**
	 * Is the 'checked' flag on?
	 * @return whether the 'checked' flag is set.
	 */
	bool IsChecked() const
	{
		return (this->flags & LWF_CHECKED) != 0;
	}

	/**
	 * Is the 'pressed' flag on?
	 * @return whether the 'pressed' flag is set.
	 */
	bool IsPressed() const
	{
		return (this->flags & LWF_PRESSED) != 0;
	}

	/**
	 * Is the 'shaded' flag on?
	 * @return whether the 'shaded' flag is set.
	 */
	bool IsShaded() const
	{
		return (this->flags & LWF_SHADED) != 0;
	}

	/**
	 * Set the 'checked' flag to the new value.
	 * @param value New value of the 'checked' flag.
	 */
	void SetChecked(bool value)
	{
		this->flags = (value) ? (this->flags | LWF_CHECKED) : (this->flags & ~LWF_CHECKED);
	}

	/**
	 * Set the 'pressed' flag to the new value.
	 * @param value New value of the 'pressed' flag.
	 */
	void SetPressed(bool value)
	{
		this->flags = (value) ? (this->flags | LWF_PRESSED) : (this->flags & ~LWF_PRESSED);
	}

	/**
	 * Set the 'shaded' flag to the new value.
	 * @param value New value of the 'shaded' flag.
	 */
	void SetShaded(bool value)
	{
		this->flags = (value) ? (this->flags | LWF_SHADED) : (this->flags & ~LWF_SHADED);
	}

	uint8 flags;        ///< Flags of the leaf widget. @see LeafWidgetFlags
	ColourRange colour; ///< Colour of the widget.
	StringID tooltip;   ///< Tool-tip of the widget.
};

/**
 * Data widget.
 * Implements #WT_TITLEBAR, #WT_LEFT_TEXT, #WT_CENTERED_TEXT, #WT_RIGHT_TEXT, #WT_TEXT_BUTTON,
 * #WT_IMAGE_BUTTON, #WT_TEXT_PUSHBUTTON, #WT_IMAGE_PUSHBUTTON and #WT_DROPDOWN_BUTTON.
 * @ingroup widget_group
 */
class DataWidget : public LeafWidget {
public:
	DataWidget(WidgetType wtype);

	void SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array) override;
	void Draw(const GuiWindow *w) override;

	uint16 value;     ///< String number or sprite id.
	int value_width;  ///< Width of the image or the string.
	int value_height; ///< Height of the image or the string.
};

/**
 * Components of the scroll bar.
 * @ingroup widget_group
 */
enum ScrollbarComponent {
	SBC_DECREMENT_BUTTON, ///< Top (or left) button.
	SBC_INCREMENT_BUTTON, ///< Bottom (or right) button.
	SBC_SLIDER,           ///< Repositionable slider.
	SBC_BEFORE_SLIDER,    ///< Region between decrement-button and slider.
	SBC_AFTER_SLIDER,     ///< Region between increment-button and slider.
};

/**
 * Scrollbar widget.
 * Implements #WT_HOR_SCROLLBAR and #WT_VERT_SCROLLBAR.
 * @ingroup widget_group
 */
class ScrollbarWidget : public LeafWidget {
public:
	ScrollbarWidget(WidgetType wtype);
	void SetScrolled(const BaseWidget *canvas);

	void SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array) override;
	void Draw(const GuiWindow *w) override;

	void OnClick(const Point32 &base, const Point16 &pos);

	void SetItemSize(uint size);
	void SetItemCount(uint count);
	void SetStart(uint offset);
	uint GetItemSize() const;
	uint GetVisibleCount() const;
	uint GetStart() const;
	void ScrollTo(uint offset);
	uint GetClickedRow(const Point16 &pos) const;

private:
	uint item_count;          ///< Number of items represented by the scroll bar.
	uint start;               ///< Index of first visible item.
	uint item_size;           ///< Size of an item if not \c 0, else use #canvas.
	const BaseWidget *canvas; ///< %Widget being scrolled.

	int GetDecrementButtonSize() const;
	int GetIncrementButtonSize() const;
	void CalculateSliderPosition(int *start_edge, int *slider_length);
	ScrollbarComponent GetClickedComponent(const Point16 &pos);
};

/**
 * Base class for canvas-like widgets.
 * Implements #WT_PANEL
 * @ingroup widget_group
 */
class BackgroundWidget : public LeafWidget {
public:
	BackgroundWidget(WidgetType wtype);
	~BackgroundWidget();

	void SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array) override;
	void SetSmallestSizePosition(const Rectangle16 &rect) override;
	void Draw(const GuiWindow *w) override;
	BaseWidget *GetWidgetByPosition(const Point16 &pt) override;
	void AutoRaiseButtons(const Point32 &base) override;

	BaseWidget *child; ///< Child widget displayed on top of the background widget.
};

/**
 * Equal size settings of child widgets.
 * @ingroup widget_group
 */
enum EqualSize {
	EQS_HORIZONTAL = 1, ///< Try to keep equal size for all widgets in horizontal direction.
	EQS_VERTICAL   = 2, ///< Try to keep equal size for all widgets in vertical direction.
};

/**
 * Data about a row or a column.
 * @ingroup widget_group
 */
struct RowColData {
	uint16 min_size; ///< Minimal size.
	uint16 fill;     ///< Fill step.
	uint16 resize;   ///< Resize step.

	void InitRowColData();
	void Merge(uint16 min_size, uint16 fill, uint16 resize);
};

/**
 * Base class for intermediate (that is, non-leaf) widget.
 * @ingroup widget_group
 */
class IntermediateWidget : public BaseWidget {
public:
	IntermediateWidget(uint8 num_rows, uint8 num_cols);
	~IntermediateWidget();

	void SetupMinimalSize(GuiWindow *w, BaseWidget **wid_array) override;
	void SetSmallestSizePosition(const Rectangle16 &rect) override;
	void Draw(const GuiWindow *w) override;
	BaseWidget *GetWidgetByPosition(const Point16 &pt) override;
	void AutoRaiseButtons(const Point32 &base) override;

	void AddChild(uint8 col, uint8 row, BaseWidget *sub);
	void ClaimMemory();

	BaseWidget **childs; ///< Grid of child widget pointers.
	RowColData *rows;    ///< Row data.
	RowColData *columns; ///< Column data.
	uint8 num_rows;      ///< Number of rows.
	uint8 num_cols;      ///< Number of columns.
	uint8 flags;         ///< Equal size flags.
};

/**
 * Available widget parts.
 * @ingroup widget_parts_group
 */
enum WidgetPartType {
	WPT_NEW_WIDGET,       ///< Start a new widget.
	WPT_NEW_INTERMEDIATE, ///< Start a new widget.
	WPT_MIN_SIZE,         ///< Set minimal size.
	WPT_FILL,             ///< Set fill sizes.
	WPT_RESIZE,           ///< Set resize sizes.
	WPT_PADDING,          ///< Set padding.
	WPT_HOR_PIP,          ///< Set horizontal PIP.
	WPT_VERT_PIP,         ///< Set vertical PIP.
	WPT_DATA,             ///< Additional data values.
	WPT_EQUAL_SIZE,       ///< Define how sizing of child widgets behaves.
	WPT_END_CON,          ///< End of container or row.
};

/**
 * Class for describing a widget-tree in linear source code.
 * @ingroup widget_parts_group
 */
class WidgetPart {
public:
	WidgetPartType type; ///< Type of this widget part.
	union {
		struct {
			WidgetType wtype; ///< Widget type of the new widget.
			int16 number;     ///< Widget number of the new widget.
			ColourRange colour; ///< Colour of the new widget.
		} new_widget;             ///< Data for #WPT_NEW_WIDGET.
		struct {
			uint8 num_rows;   ///< Number of rows.
			uint8 num_cols;   ///< Number of columns.
		} new_intermediate;       ///< Data for #WPT_NEW_INTERMEDIATE.
		struct {
			uint16 value;     ///< Value of the widget (string or sprite id).
			uint16 tip;       ///< Tool tip string. Use #STR_NULL for no tip.
		} dat;                    ///< Additional data of the widget.
		Point16 size;             ///< Data for #WPT_MIN_SIZE, #WPT_FILL, #WPT_RESIZE.
		uint8 flags;              ///< Equal size flags for intermediate widgets.
		uint8 padding[PAD_COUNT]; ///< Data for #WPT_PADDING, #WPT_HOR_PIP, #WPT_VERT_PIP.
	} data; ///< Data of the widget part.
};

WidgetPart Widget(WidgetType wtype, WidgetNumber number, ColourRange colour);
WidgetPart Intermediate(uint8 num_rows, uint8 num_cols = 0);
WidgetPart SetMinimalSize(int16 x, int16 y);
WidgetPart SetFill(uint8 x, uint8 y);
WidgetPart SetResize(uint8 x, uint8 y);
WidgetPart SetPadding(uint8 top, uint8 right, uint8 bottom, uint8 left);
WidgetPart SetHorPIP(uint8 pre, uint8 inter, uint8 post);
WidgetPart SetVertPIP(uint8 pre, uint8 inter, uint8 post);
WidgetPart SetData(uint16 data, uint16 tip);
WidgetPart SetEqualSize(bool hor_equal, bool vert_equal);
WidgetPart EndContainer();

BaseWidget *MakeWidgetTree(const WidgetPart *parts, int length, int16 *biggest);

#endif

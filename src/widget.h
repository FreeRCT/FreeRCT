/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file widget.h Widget declarations. */

#ifndef WIDGET_H
#define WIDGET_H

#include "geometry.h"

static const int INVALID_WIDGET_INDEX = -1; ///< Widget number of invalid index.

/** Available widget types. */
enum WidgetType {
	WT_EMPTY,          ///< Empty widget (used for creating empty space and/or centering).
	WT_TITLEBAR,       ///< Title of the window.
	WT_CLOSEBOX,       ///< Close box.
	WT_RESIZEBOX,      ///< Resize box.
	WT_LEFT_TEXT,      ///< Text label with left-aligned text.
	WT_CENTERED_TEXT,  ///< Text label with centered text.
	WT_RIGHT_TEXT,     ///< Text label with right-aligned text.
	WT_PANEL,          ///< Panel.
	WT_TEXTBUTTON,     ///< Button with text.
	WT_IMAGEBUTTON,    ///< Button with a sprite.
	WT_RADIOBUTTON,    ///< Radio button widget.
	WT_HOR_SCROLLBAR,  ///< Scrollbar widget.
	WT_VERT_SCROLLBAR, ///< Scrollbar widget.
	WT_GRID,           ///< Intermediate widget.
	WT_REFERENCE,      ///< Reference to an existing widget.
};

/** Padding space around widgets. */
enum PaddingDirection {
	PAD_TOP,        ///< Padding at the top.
	PAD_LEFT,       ///< Padding at the left.
	PAD_RIGHT,      ///< Padding at the right.
	PAD_BOTTOM,     ///< Padding at the bottom.
	PAD_VERTICAL,   ///< Inter-child vertical padding space.
	PAD_HORIZONTAL, ///< Inter-child horizontal padding space.

	PAD_COUNT,      ///< Number of paddings.
};

/** Base class for all widget. */
class CoreWidget {
public:
	CoreWidget(WidgetType wtype);
	virtual ~CoreWidget();

	WidgetType wtype; ///< Widget type.
	int16 number;     ///< Widget number.
};

/** Reference to another widget. */
class ReferenceWidget : public CoreWidget {
public:
	ReferenceWidget(int widnum);
};

/**
 * Base class for the displayed widgets.
 * Also implements #WT_EMPTY, #WT_CLOSEBOX, #WT_RESIZEBOX.
 */
class BaseWidget : public CoreWidget {
public:
	BaseWidget(WidgetType wtype);

	uint16 min_x;    ///< Minimal horizontal size.
	uint16 min_y;    ///< Minimal vertical size.
	Rectangle16 pos; ///< Current position and size (relative to window top-left edge).
	uint16 fill_x;   ///< Horizontal fill step.
	uint16 fill_y;   ///< Vertical fill step.
	uint16 resize_x; ///< Horizontal resize step.
	uint16 resize_y; ///< Vertical resize step.
	uint8 paddings[PAD_COUNT]; ///< Padding.
};

/**
 * Base class for a (visible) leaf widget.
 * Implements #WT_RADIOBUTTON
 * @todo use #LeafWidget::colour.
 */
class LeafWidget : public BaseWidget {
public:
	LeafWidget(WidgetType wtype);

	bool pressed;   ///< Is widget pushed down?
	bool shaded;    ///< Is widget disabled?
	uint8 colour;   ///< Colour of the widget.
	uint16 tooltip; ///< Tool-tip of the widget.
};

/**
 * Data widget
 * Implements #WT_TITLEBAR, #WT_LEFT_TEXT, #WT_CENTERED_TEXT, #WT_RIGHT_TEXT, #WT_TEXTBUTTON and #WT_IMAGEBUTTON.
 */
class DataWidget : public LeafWidget {
public:
	DataWidget(WidgetType wtype);

	uint16 value; ///< String number or sprite id.
};

/**
 * Scrollbar widget.
 * Implements #WT_HOR_SCROLLBAR and #WT_VERT_SCROLLBAR.
 */
class ScrollbarWidget : public LeafWidget {
public:
	ScrollbarWidget(WidgetType wtype);

	int16 canvas_widget; ///< Widget number of the canvas.
};

/**
 * Base class for canvas-like widgets.
 * Implements #WT_PANEL
 */
class BackgroundWidget : public LeafWidget {
public:
	BackgroundWidget(WidgetType wtype);
	virtual ~BackgroundWidget();

	CoreWidget *child; ///< Child widget displayed on top of the background widget.
};

/** Equal size settings of child widgets. */
enum EqualSize {
	EQS_HORIZONTAL = 1, ///< Try to keep equal size for all widgets in horizontal direction.
	EQS_VERTICAL   = 2, ///< Try to keep equal size for all widgets in vertical direction.
};

/** Base class for intermediate (that is, non-leaf) widget. */
class IntermediateWidget : public BaseWidget {
public:
	IntermediateWidget(uint8 num_rows, uint8 num_cols);
	~IntermediateWidget();

	void AddChild(uint8 col, uint8 row, CoreWidget *sub);
	void ClaimMemory();

	uint8 num_rows; ///< Number of rows.
	uint8 num_cols; ///< Number of columns.
	uint8 flags;    ///< Equal size flags.

	CoreWidget **childs; ///< Grid of child widget pointers.
};


/** Available widget parts. */
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
	WPT_REFERENCE,        ///< Reference to another widget.
	WPT_END_CON,          ///< End of container or row.
};

/** Class for describing a widget-tree in linear source code. */
class WidgetPart {
public:
	WidgetPartType type; ///< Type of this widget part.
	union {
		struct {
			WidgetType wtype; ///< Widget type of the new widget.
			int16 number;     ///< Widget number of the new widget.
			uint8 colour;     ///< Colour of the new widget.
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
		int16 reference;          ///< Reference to a widget.
		uint8 flags;              ///< Equal size flags for intermediate widgets.
		uint8 padding[PAD_COUNT]; ///< Data for #WPT_PADDING, #WPT_HOR_PIP, #WPT_VERT_PIP.
	} data; ///< Data of the widget part.
};

WidgetPart Widget(WidgetType wtype, int16 number, uint8 colour);
WidgetPart Intermediate(uint8 num_rows, uint8 num_cols = 0);
WidgetPart SetFill(uint8 x, uint8 y);
WidgetPart SetResize(uint8 x, uint8 y);
WidgetPart SetPadding(uint8 top, uint8 right, uint8 bottom, uint8 left);
WidgetPart SetHorPIP(uint8 pre, uint8 inter, uint8 post);
WidgetPart SetVertPIP(uint8 pre, uint8 inter, uint8 post);
WidgetPart SetData(uint16 data, uint16 tip);
WidgetPart SetEqualSize(bool hor_equal, bool vert_equal);
WidgetPart WidgetReference(int widnum);
WidgetPart EndContainer();

void DrawPanel(const Rectangle &rect);
CoreWidget *MakeWidgetTree(const WidgetPart *parts, int length, int16 *biggest);

#endif

/* $Id$ */

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
#include "widget.h"
#include "video.h"

#include "language.h"

/**
 * Draw border sprites around some contents.
 * @param vid Video display.
 * @param bsd Border sprites to use.
 * @param pressed Draw pressed down sprites.
 * @param rect Content rectangle to draw around.
 * @todo [easy] Argh, another _manager.video why is there no _video thingie instead?
 * @todo [??] Similarly, needing to state the bounding box 'screen' seems weird and is not the right solution.
 */
static void DrawBorderSprites(VideoSystem *vid, const BorderSpriteData &bsd, bool pressed, const Rectangle &rect)
{
	Point pt;
	const Sprite * const *spr_base = pressed ? bsd.pressed : bsd.normal;

	pt.x = rect.base.x;
	pt.y = rect.base.y;
	vid->BlitImage(pt, spr_base[WBS_TOP_LEFT]);
	int xleft = pt.x + spr_base[WBS_TOP_LEFT]->xoffset + spr_base[WBS_TOP_LEFT]->img_data->width;
	int ytop = pt.y + spr_base[WBS_TOP_LEFT]->yoffset + spr_base[WBS_TOP_LEFT]->img_data->height;

	pt.x = rect.base.x + rect.width - 1;
	vid->BlitImage(pt, spr_base[WBS_TOP_RIGHT]);
	int xright = pt.x + spr_base[WBS_TOP_RIGHT]->xoffset;

	uint16 numx = (xright - xleft) / spr_base[WBS_TOP_MIDDLE]->img_data->width;
	vid->BlitHorizontal(xleft, numx, pt.y, spr_base[WBS_TOP_MIDDLE]);

	pt.x = rect.base.x;
	pt.y = rect.base.y + rect.height - 1;
	vid->BlitImage(pt, spr_base[WBS_BOTTOM_LEFT]);
	int ybot = pt.y + spr_base[WBS_BOTTOM_LEFT]->yoffset;

	uint16 numy = (ybot - ytop) / spr_base[WBS_MIDDLE_LEFT]->img_data->height;
	vid->BlitVertical(ytop, numy, pt.x, spr_base[WBS_MIDDLE_LEFT]);

	pt.x = rect.base.x + rect.width - 1;
	pt.y = rect.base.y + rect.height - 1;
	vid->BlitImage(pt, spr_base[WBS_BOTTOM_RIGHT]);

	vid->BlitHorizontal(xleft, numx, pt.y, spr_base[WBS_BOTTOM_MIDDLE]);

	pt.x = rect.base.x + rect.width - 1;
	vid->BlitVertical(ytop, numy, pt.x, spr_base[WBS_MIDDLE_RIGHT]);

	vid->BlitImages(xleft, ytop, spr_base[WBS_MIDDLE_MIDDLE], numx, numy);
}

/**
 * Draw a panel at the given coordinates.
 * @param vid Video display.
 * @param rect Outer rectangle to draw in.
 * @todo [difficult] Outer rectangle does not sound very useful in general.
 */
void DrawPanel(VideoSystem *vid, const Rectangle &rect)
{
	const BorderSpriteData &bsd = _gui_sprites.panel;
	Rectangle rect2(rect.base.x + bsd.border_left, rect.base.y + bsd.border_top,
			rect.width - bsd.border_left -  bsd.border_right, rect.height - bsd.border_top - bsd.border_bottom);

	DrawBorderSprites(vid, bsd, false, rect2);
}

/**
 * Constructor of all widgets.
 */
CoreWidget::CoreWidget(WidgetType wtype)
{
	this->wtype = wtype;
	this->number = INVALID_WIDGET_INDEX;
}

CoreWidget::~CoreWidget()
{
}

/**
 * Reference to another widget.
 * @param widnum %Widget number of the referenced widget.
 */
ReferenceWidget::ReferenceWidget(int widnum) : CoreWidget(WT_REFERENCE)
{
	this->number = widnum;
}

/**
 * Base class widget constructor.
 * @param wtype %Widget type.
 */
BaseWidget::BaseWidget(WidgetType wtype) : CoreWidget(wtype)
{
	this->min_x = 0;
	this->min_y = 0;
	this->pos.base.x = 0;
	this->pos.base.y = 0;
	this->pos.width  = 0;
	this->pos.height = 0;

	this->fill_x = 0;
	this->fill_y = 0;
	this->resize_x = 0;
	this->resize_y = 0;
	for (int i = 0; i < PAD_COUNT; i++) this->paddings[i] = 0;
}


/**
 * Base class leaf widget constructor.
 * @param wtype %Widget type.
 */
LeafWidget::LeafWidget(WidgetType wtype) : BaseWidget(wtype)
{
	this->pressed = false;
	this->shaded = false;
	this->colour = 0;
	this->tooltip = STR_NULL;
}

/**
 * Data widget constructor.
 * @param wtype %Widget type.
 */
DataWidget::DataWidget(WidgetType wtype) : LeafWidget(wtype)
{
	this->value = 0;
}

/**
 * Scrollbar widget constructor.
 * @param wtype %Widget type.
 */
ScrollbarWidget::ScrollbarWidget(WidgetType wtype) : LeafWidget(wtype)
{
	this->canvas_widget = canvas_widget;
}

/**
 * Baseclass background widget constructor.
 * @param wtype %Widget type.
 */
BackgroundWidget::BackgroundWidget(WidgetType wtype) : LeafWidget(wtype)
{
	this->child = NULL;
}

BackgroundWidget::~BackgroundWidget()
{
	if (this->child != NULL) delete this->child;
}

/**
 * Constructor for intermediate widgets.
 * @param num_rows Number of rows. Use \c 0 for 'manual' claiming.
 * @param num_cols Number of columns. Use \c 0 for 'manual' claiming.
 * @see FillWidget
 */
IntermediateWidget::IntermediateWidget(uint8 num_rows, uint8 num_cols) : BaseWidget(WT_GRID)
{
	this->num_rows = num_rows;
	this->num_cols = num_cols;

	this->childs = NULL;
	if (this->num_cols > 0 && this->num_rows > 0) this->ClaimMemory();
}

/**
 * Claim memory for childs 'manually'.
 * @pre Memory must not have been claimed, and #num_cols and #num_rows must be bigger than \c 0.
 */
void IntermediateWidget::ClaimMemory()
{
	assert(this->num_cols > 0 && this->num_rows > 0);
	assert(this->childs == NULL);

	this->childs = (CoreWidget **)malloc(num_rows * num_cols * sizeof(CoreWidget **));
	assert(this->childs != NULL);
	for (uint8 y = 0; y < this->num_rows; y++) {
		for (uint8 x = 0; x < this->num_cols; x++) {
			this->childs[y * this->num_cols + x] = NULL;
		}
	}
}

IntermediateWidget::~IntermediateWidget()
{
	if (this->childs != NULL) {
		for (uint8 y = 0; y < this->num_rows; y++) {
			for (uint8 x = 0; x < this->num_cols; x++) {
				CoreWidget *w = this->childs[y * this->num_cols + x];
				if (w != NULL) delete w;
			}
		}
		free(this->childs);
	}
}

/**
 * Add a child widget.
 * @param x Horizontal index of the child in the grid.
 * @param y Vertical index of the child in the grid.
 * @param w Child widget to add.
 */
void IntermediateWidget::AddChild(uint8 x, uint8 y, CoreWidget *w)
{
	assert(x < this->num_cols && y < this->num_rows);
	assert(this->childs[y * this->num_cols + x] == NULL);
	this->childs[y * this->num_cols + x] = w;
}

/**
 * Create a new #WT_GRID widget.
 * @param num_rows Number of rows.
 * @param num_cols Number of columns.
 * @return Widget part containing the provided data for storage in an array.
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
 */
WidgetPart Widget(WidgetType wtype, int16 number, uint8 colour)
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
 */
WidgetPart SetMinimalSize(uint8 x, uint8 y)
{
	WidgetPart part;

	part.type = WPT_MIN_SIZE;
	part.data.size.x = x;
	part.data.size.y = y;
	return part;
}

/**
 * Set fill step.
 * @param x Horizontal fill step.
 * @param y Vertical fill step.
 * @return Widget part containing the provided data for storage in an array.
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
 * @param hor_equal Try to keep all childs equally wide.
 * @param vert_equal Try to keep all child equally high.
 * @return Widget part containing the provided data for storage in an array.
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
 */
WidgetPart EndContainer()
{
	WidgetPart part;

	part.type = WPT_END_CON;
	return part;
}

/**
 * Hook a child widget again into the intermediate widget.
 * @param widnum Widget number of the child widget.
 * @return Widget part containing the provided data for storage in an array.
 */
WidgetPart WidgetReference(int widnum)
{
	WidgetPart part;

	assert(widnum >= 0);
	part.type = WPT_REFERENCE;
	part.data.reference = widnum;
	return part;
}


/**
 * Construct a widget from widget parts.
 * @param parts Base of parts array.
 * @param remaining Number of parts still available.
 * @param dest Pointer for storing the constructed widget.
 * @return Read number of parts.
 */
static int MakeWidget(const WidgetPart *parts, int remaining, CoreWidget **dest)
{
	int num_used = 0;

	*dest = NULL;
	while (num_used < remaining) {
		switch (parts->type) {
			case WPT_REFERENCE:
				if (*dest != NULL) return num_used;
				*dest = new ReferenceWidget(parts->data.reference);
				return num_used + 1;

			case WPT_NEW_WIDGET: {
				if (*dest != NULL) return num_used;
				switch (parts->data.new_widget.wtype) {
					case WT_EMPTY:
					case WT_CLOSEBOX:
					case WT_RESIZEBOX:
						*dest = new BaseWidget(parts->data.new_widget.wtype);
						break;

					case WT_PANEL:
						*dest = new BackgroundWidget(WT_PANEL);
						break;

					case WT_TEXTBUTTON:
					case WT_IMAGEBUTTON:
					case WT_TITLEBAR:
					case WT_LEFT_TEXT:
					case WT_CENTERED_TEXT:
					case WT_RIGHT_TEXT:
						*dest = new DataWidget(parts->data.new_widget.wtype);
						break;

					case WT_RADIOBUTTON:
						*dest = new LeafWidget(WT_RADIOBUTTON);
						break;

					case WT_HOR_SCROLLBAR:
					case WT_VERT_SCROLLBAR:
						*dest = new ScrollbarWidget(parts->data.new_widget.wtype);
						break;

					default: NOT_REACHED();
				}
				if (parts->data.new_widget.number >= 0) (*dest)->number = parts->data.new_widget.number;
				LeafWidget *lw = dynamic_cast<LeafWidget *>(*dest);
				if (lw != NULL) lw->colour = parts->data.new_widget.colour;
				break;
			}

			case WPT_NEW_INTERMEDIATE:
				if (*dest != NULL) return num_used;
				(*dest) = new IntermediateWidget(parts->data.new_intermediate.num_rows, parts->data.new_intermediate.num_cols);
				break;

			case WPT_MIN_SIZE: {
				if (*dest == NULL) break;
				BaseWidget *bw = dynamic_cast<BaseWidget *>(*dest);
				if (bw != NULL) {
					bw->min_x = parts->data.size.x;
					bw->min_y = parts->data.size.y;
				}
				break;
			}

			case WPT_FILL: {
				if (*dest == NULL) break;
				BaseWidget *bw = dynamic_cast<BaseWidget *>(*dest);
				if (bw != NULL) {
					bw->fill_x = parts->data.size.x;
					bw->fill_y = parts->data.size.y;
				}
				break;
			}

			case WPT_RESIZE: {
				if (*dest == NULL) break;
				BaseWidget *bw = dynamic_cast<BaseWidget *>(*dest);
				if (bw != NULL) {
					bw->resize_x = parts->data.size.x;
					bw->resize_y = parts->data.size.y;
				}
				break;
			}

			case WPT_PADDING:
			case WPT_HOR_PIP:
			case WPT_VERT_PIP: {
				if (*dest == NULL) break;
				BaseWidget *bw = dynamic_cast<BaseWidget *>(*dest);
				if (bw != NULL) {
					for (int i = 0; i < PAD_COUNT; i++) bw->paddings[i] += parts->data.padding[i];
				}
				break;
			}

			case WPT_DATA: {
				if (*dest == NULL) break;
				LeafWidget *lw = dynamic_cast<LeafWidget *>(*dest);
				if (lw != NULL) lw->tooltip = parts->data.dat.tip;
				DataWidget *bw = dynamic_cast<DataWidget *>(*dest);
				if (bw != NULL) bw->value = parts->data.dat.value;
				break;
			}

			case WPT_EQUAL_SIZE: {
				if (*dest == NULL) break;
				IntermediateWidget *iw = dynamic_cast<IntermediateWidget *>(*dest);
				if (iw != NULL) iw->flags = parts->data.flags;
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

static int MakeWidgetSubTree(const WidgetPart *parts, int remaining, CoreWidget **dest, int16 *biggest);

/**
 * Fill a row of an intermediate widget with its children.
 * @param parts Parts to assemble into child widgets of the row.
 * @param remaining_parts Number of parts still available.
 * @param widgets Temporary storage for the child widgets.
 * @param remaining_widgets Amount of space left for the child widgets.
 * @param cols [inout] Number of elements of a row. \c 0 means it is not known.
 * @param biggest [out] Pointer to stored biggest widget index number.
 * @return Number of used parts.
 */
static int FillWidgetRow(const WidgetPart *parts, int remaining_parts, CoreWidget **widgets, int remaining_widgets, uint8 *cols, int16 *biggest)
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
		if (*widgets == NULL) break;

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
 */
static int FillWidget(const WidgetPart *parts, int remaining_parts, IntermediateWidget *wid, int16 *biggest)
{
	int total_used = 0;

	static const int MAX_CHILDS = 500;
	CoreWidget *grid[500];
	for (int i = 0; i < 500; i++) grid[i] = NULL;

	bool need_claim_memory = wid->num_rows == 0 || wid->num_cols == 0;

	int remaining_widgets = MAX_CHILDS;
	CoreWidget **widgets = &grid[0];
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
	int count = (int)wid->num_rows * wid->num_cols;
	assert(remaining_widgets == MAX_CHILDS - count);

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
 */
static int MakeWidgetSubTree(const WidgetPart *parts, int remaining, CoreWidget **dest, int16 *biggest)
{
	int total_used = 0;

	*dest = NULL;
	int used = MakeWidget(parts, remaining, dest);
	parts += used;
	remaining -= used;
	total_used += used;

	if (*dest == NULL) return total_used;

	*biggest = max(*biggest, (*dest)->number); // Update biggest widget number.

	if ((*dest)->wtype == WT_PANEL) {
		/* Panel widget. */
		BackgroundWidget *bg = static_cast<BackgroundWidget *>(*dest);
		if (remaining > 0 && parts->type == WPT_END_CON) {
			used = 1;
		} else {
			used = MakeWidgetSubTree(parts, remaining, &bg->child, biggest);
		}
		parts += used;
		remaining -= used;
		total_used += used;
	} else if ((*dest)->wtype == WT_GRID) {
		/* Container widget; also load child widgets. */
		IntermediateWidget *iw = static_cast<IntermediateWidget *>(*dest);
		used = FillWidget(parts, remaining, iw, biggest);
		parts += used;
		remaining -= used;
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
 */
CoreWidget *MakeWidgetTree(const WidgetPart *parts, int length, int16 *biggest)
{
	CoreWidget *root = NULL;
	*biggest = INVALID_WIDGET_INDEX;
	MakeWidgetSubTree(parts, length, &root, biggest);
	return root;
}

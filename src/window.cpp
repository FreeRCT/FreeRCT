/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file window.cpp %Window handling functions.
 * @todo [low] Consider the case where we don't have a video system.
 */

/**
 * @defgroup window_group %Window code
 */

/**
 * @defgroup gui_group Actual windows
 * @ingroup window_group
 */

#include "stdafx.h"
#include "window.h"
#include "video.h"
#include "math_func.h"
#include "palette.h"
#include "sprite_store.h"
#include "ride_type.h"

/**
 * %Window manager.
 * @ingroup window_group
 */
WindowManager _manager;

static uint GetWindowZPriority(WindowTypes wt);

/**
 * Does the mouse button state express a left-click?
 * @param state Mouse button state (as given in #Window::OnMouseButtonEvent).
 * @return The state expresses a left button click down event.
 * @ingroup window_group
 */
bool IsLeftClick(uint8 state)
{
	return (state & (MB_LEFT | (MB_LEFT << MB_PREV_SHIFT))) == MB_LEFT;
}

/**
 * %Window constructor. It automatically adds the window to the window stack.
 * @param wtype %Window type Type of window.
 * @param wnumber Number of the window within the \a wtype.
 */
Window::Window(WindowTypes wtype, WindowNumber wnumber) : rect(0, 0, 0, 0), wtype(wtype), wnumber(wnumber)
{
	this->timeout = 0;
	this->higher = NULL;
	this->lower  = NULL;
	this->flags  = 0;

	_manager.AddToStack(this); // Add to window stack.
}

/**
 * Destructor.
 * Do not call directly, use #WindowManager::DeleteWindow instead.
 */
Window::~Window()
{
	assert(!_manager.HasWindow(this));
}

/**
 * Set the initial size of a window.
 * @param width Initial width of the new window.
 * @param height Initial height of the new window.
 */
/* virtual */ void Window::SetSize(uint width, uint height)
{
	this->rect.width = width;
	this->rect.height = height;
}

/**
 * Set the initial position of the top-left corner of the window.
 * @param x Initial X position.
 * @param y Initial Y position.
 */
void Window::SetPosition(int32 x, int32 y)
{
	this->rect.base.x = x;
	this->rect.base.y = y;
}

/** Compute the initial position of a window. */
class ComputeInitialPosition {
public:
	ComputeInitialPosition();

	Point32 FindPosition(Window *w);

	bool IsScreenEmpty(const Rectangle32 &rect);

protected:
	int base_pos; ///< Base position of a new window.
	Window *skip; ///< Window to skip in the search.

	static const int GAP; ///< Gap between the old and the new window.
};

const int ComputeInitialPosition::GAP = 5;


ComputeInitialPosition::ComputeInitialPosition()
{
	this->base_pos = 10;
	this->skip = NULL;
}

/**
 * Get distance of a position to the mouse.
 * @param pt Queried point.
 * @return Estimated distance.
 * @todo [easy] Estimate is pretty stupid, and can be improved. (Partly diagonal + partly horizontal or vertical suffices.)
 */
static int GetDistanceToMouse(const Point32 &pt)
{
	Point16 mouse_pos = _manager.GetMousePosition();
	return abs(mouse_pos.x - pt.x) + abs(mouse_pos.y - pt.y);
}

/**
 * Find an initial position for new window \a w.
 * @param new_w New window to position.
 * @return Best position of the window.
 * @pre Size of the window has been decided.
 */
Point32 ComputeInitialPosition::FindPosition(Window *new_w)
{
	static const Point16 test_positions[] = {
		{0, 1}, {0, 2}, {3, 1}, {3, 2},
		{1, 0}, {2, 0}, {1, 3}, {2, 3}
	};

	int32 x[4], y[4];

	this->skip = new_w;

	Point32 best;
	best.x = this->base_pos;
	best.y = this->base_pos;

	bool found_empty = false;
	for (Window *w = _manager.top; w != NULL; w = w->lower) {
		if (w == new_w) continue;
		x[0] = w->rect.base.x - new_w->rect.width - GAP;
		x[1] = w->rect.base.x;
		x[2] = w->rect.base.x + w->rect.width - new_w->rect.width;
		x[3] = w->rect.base.x + w->rect.width + GAP;

		y[0] = w->rect.base.y - new_w->rect.height - GAP;
		y[1] = w->rect.base.y;
		y[2] = w->rect.base.y + w->rect.height - new_w->rect.height;
		y[3] = w->rect.base.y + w->rect.height + GAP;

		for (uint i = 0; i < lengthof(test_positions); i++) {
			Point32 pt;
			pt.x = x[test_positions[i].x];
			pt.y = y[test_positions[i].y];
			if (this->IsScreenEmpty(Rectangle32(pt.x, pt.y, new_w->rect.width, new_w->rect.height))) {
				if (!found_empty || GetDistanceToMouse(best) > GetDistanceToMouse(pt)) {
					best = pt;
					found_empty = true;
				}
			}
		}
	}

	if (best.x == this->base_pos) {
		this->base_pos += 10;
		if (this->base_pos + 100 > _video->GetYSize()) this->base_pos = 10;
	}

	return best;
}

/**
 * Is the screen empty below the rectangle?
 * @param rect Area to test.
 * @return Tested area is clear.
 */
bool ComputeInitialPosition::IsScreenEmpty(const Rectangle32 &rect)
{
	uint new_prio = GetWindowZPriority(this->skip->wtype);

	if (rect.base.x < 0 || rect.base.y < 0
			|| rect.base.x + rect.width  > _video->GetXSize()
			|| rect.base.y + rect.height > _video->GetYSize()) return false;

	for (Window *w = _manager.top; w != NULL; w = w->lower) {
		if (w == this->skip || new_prio > GetWindowZPriority(w->wtype)) continue;
		if (w->rect.Intersects(rect)) return false;
	}
	return true;
}

/**
 * Find a nice initial position for the new window.
 * @return Initial position of the window.
 */
/* virtual */ Point32 Window::OnInitialPosition()
{
	static ComputeInitialPosition compute_pos;

	return compute_pos.FindPosition(this);
}

/**
 * Mark windows as being dirty (needing a repaint).
 * @todo Marking the whole display as needing a repaint is too crude.
 */
void Window::MarkDirty()
{
	_video->MarkDisplayDirty(this->rect);
}

/**
 * Paint the window to the screen.
 * @note The window manager already locked the surface.
 */
/* virtual */ void Window::OnDraw() { }

/**
 * Mouse moved to new position.
 * @param pos New position.
 */
/* virtual */ void Window::OnMouseMoveEvent(const Point16 &pos) { }

/**
 * Mouse buttons changed state.
 * @param state Updated state. @see MouseButtons
 * @return Action to perform as a result of the event (use #WMME_NONE if no special action needed).
 */
/* virtual */ WmMouseEvent Window::OnMouseButtonEvent(uint8 state)
{
	return WMME_NONE;
}

/**
 * Mousewheel rotated.
 * @param direction Direction of change (\c +1 or \c -1).
 */
/* virtual */ void Window::OnMouseWheelEvent(int direction) { }

/** Mouse entered window. */
/* virtual */ void Window::OnMouseEnterEvent() { }

/** Mouse left window. */
/* virtual */ void Window::OnMouseLeaveEvent() { }

/**
 * Timeout callback.
 * Called when #timeout decremented to 0.
 */
/* virtual */ void Window::TimeoutCallback() { }

/**
 * Enable or disable highlighting. Base class does nothing.
 * If enabled, the #timeout is used to automatically disable it again.
 * @param value New highlight value.
 */
/* virtual */ void Window::SetHighlight(bool value) { }

/**
 * An important (window-specific) change has happened.
 * @param code Unique identification of the change.
 * @param parameter Parameter of the \a code.
 * @note Meaning of number values are documented near this method in derived classes.
 */
/* virtual */ void Window::OnChange(ChangeCode code, uint32 parameter) { }

/**
 * Gui window constructor.
 * @param wtype %Window type (for finding a window in the stack).
 * @param wnumber Number of the window within the \a wtype.
 * @note Initialize the widget tree from the derived window class.
 */
GuiWindow::GuiWindow(WindowTypes wtype, WindowNumber wnumber) : Window(wtype, wnumber)
{
	this->mouse_pos.x = -1;
	this->mouse_pos.y = -1;
	this->tree = NULL;
	this->widgets = NULL;
	this->num_widgets = 0;
	this->SetHighlight(true);
	this->ride_type = NULL;
}

GuiWindow::~GuiWindow()
{
	if (this->tree != NULL) delete this->tree;
	free(this->widgets);
}

/**
 * Set the size of the window.
 * @param width Initial width of the new window.
 * @param height Initial height of the new window.
 */
/* virtual */ void GuiWindow::SetSize(uint width, uint height)
{
	// XXX Do nothing for now, in the future, this should cause a window resize.
}

/**
 * Set the string translation base for the generic strings of this window.
 * @param ride_type Ride type to use as base for the string translation.
 */
void GuiWindow::SetShopType(const ShopType *ride_type)
{
	this->ride_type = ride_type;
}

/**
 * Translate the string number if necessary.
 * @param str_id String number to translate.
 * @return Translated string number.
 */
StringID GuiWindow::TranslateStringNumber(StringID str_id) const
{
	assert(str_id != STR_INVALID);

	if (this->ride_type != NULL && str_id >= STR_GENERIC_SHOP_START) return this->ride_type->GetString(str_id);
	return str_id;
}

/**
 * Construct the widget tree of the window, and initialize the window with it.
 * @param parts %Widget parts describing the window.
 * @param length Number of parts.
 * @pre The tree has not been setup before.
 */
void GuiWindow::SetupWidgetTree(const WidgetPart *parts, int length)
{
	assert(_video != NULL); // Needed for font-size calculations.
	assert(this->tree == NULL && this->widgets == NULL);

	int16 biggest;
	this->tree = MakeWidgetTree(parts, length, &biggest);

	if (biggest >= 0) {
		this->num_widgets = biggest + 1;
		this->widgets = (BaseWidget **)malloc(sizeof(BaseWidget *) * (biggest + 1));
		for (int16 i = 0; i <= biggest; i++) this->widgets[i] = NULL;
	}

	this->tree->SetupMinimalSize(this, this->widgets);
	this->rect = Rectangle32(0, 0, this->tree->min_x, this->tree->min_y);

	Rectangle16 min_rect(0, 0, this->tree->min_x, this->tree->min_y);
	this->tree->SetSmallestSizePosition(min_rect);

	Point32 pt = this->OnInitialPosition();
	this->SetPosition(pt.x, pt.y);

	this->MarkDirty();
}

/**
 * Allow for last minute changes in the initial widget size. If the function does nothing, you'll get the default widgets.
 * The values BaseWidget::min_x and BaseWidget::min_y may be altered, but it may be a bad idea to make them smaller.
 * BaseWidget::fill_x, BaseWidget::fill_y, BaseWidget::resize_x, BaseWidget::resize_y may also be changed.
 * Since this function is being called while the window is being constructed, there is not much else that has a sane value at this point.
 * @param wid_num Widget number of the provided widget.
 * @param wid The widget itself.
 */
/* virtual */ void GuiWindow::UpdateWidgetSize(WidgetNumber wid_num, BaseWidget *wid)
{
	/* Do nothing by default. */
}

/**
 * Set string parameters of the data string of the widget.
 * @param wid_num Widget number of the widget.
 */
/* virtual */ void GuiWindow::SetWidgetStringParameters(WidgetNumber wid_num) const
{
	/* Do nothing by default. */
}

/**
 * Draw custom contents of the widget.
 * The code should not do anything else but render contents to the screen.
 * @param wid_num Widget number of the widget being drawn.
 * @param wid The widget itself.
 */
/* virtual */ void GuiWindow::DrawWidget(WidgetNumber wid_num, const BaseWidget *wid) const
{
	/* Do nothing by default. */
}

/* virtual */ void GuiWindow::OnDraw()
{
	this->tree->Draw(this);
	if ((this->flags & WF_HIGHLIGHT) != 0) _video->DrawRectangle(this->rect, COL_HIGHLIGHT);
}

/* virtual */ void GuiWindow::OnMouseMoveEvent(const Point16 &pos)
{
	this->mouse_pos = pos;
}

/* virtual */ WmMouseEvent GuiWindow::OnMouseButtonEvent(uint8 state)
{
	if (!IsLeftClick(state) || this->mouse_pos.x < 0) return WMME_NONE;

	BaseWidget *bw = this->tree->GetWidgetByPosition(this->mouse_pos);
	if (bw != NULL) {
		if (bw->wtype == WT_TITLEBAR) return WMME_MOVE_WINDOW;
		if (bw->wtype == WT_CLOSEBOX) return WMME_CLOSE_WINDOW;

		LeafWidget *lw = dynamic_cast<LeafWidget *>(bw);
		if (lw != NULL && lw->IsShaded()) return WMME_NONE;

		if (bw->wtype == WT_TEXT_PUSHBUTTON || bw->wtype == WT_IMAGE_PUSHBUTTON) {
			/* For mono-stable buttons, 'press' the button, and set a timeout for 'releasing' it again. */
			lw->SetPressed(true);
			this->timeout = 4;
			lw->MarkDirty(this->rect.base);
		}
		if (bw->number >= 0) this->OnClick(bw->number);
	}
	return WMME_NONE;
}

/* virtual */ void GuiWindow::OnMouseLeaveEvent()
{
	this->mouse_pos.x = -1;
	this->mouse_pos.y = -1;
}

/**
 * A click with the left button at a widget has been detected.
 * @param widget %Widget number.
 */
/* virtual */ void GuiWindow::OnClick(WidgetNumber widget)
{
}

/**
 * Set the checked state of the given widget.
 * @param widget %Widget number.
 * @param value New checked state value.
 */
void GuiWindow::SetWidgetChecked(WidgetNumber widget, bool value)
{
	LeafWidget *lw = this->GetWidget<LeafWidget>(widget);
	if (lw->IsChecked() != value) {
		lw->SetChecked(value);
		lw->MarkDirty(this->rect.base);
	}
}

/**
 * Is widget \a widget checked?
 * @param widget %Widget number to use.
 * @return Whether the widget is checked.
 */
bool GuiWindow::IsWidgetChecked(WidgetNumber widget) const
{
	return this->GetWidget<LeafWidget>(widget)->IsChecked();
}

/**
 * Set the pressed state of the given widget.
 * @param widget %Widget number.
 * @param value New pressed state value.
 */
void GuiWindow::SetWidgetPressed(WidgetNumber widget, bool value)
{
	LeafWidget *lw = this->GetWidget<LeafWidget>(widget);
	if (lw->IsPressed() != value) {
		lw->SetPressed(value);
		lw->MarkDirty(this->rect.base);
	}
}

/**
 * Is widget \a widget pressed?
 * @param widget %Widget number to use.
 * @return Whether the widget is pressed.
 */
bool GuiWindow::IsWidgetPressed(WidgetNumber widget) const
{
	return this->GetWidget<LeafWidget>(widget)->IsPressed();
}

/**
 * Set the shaded state of the given widget.
 * @param widget %Widget number.
 * @param value New shaded state value.
 */
void GuiWindow::SetWidgetShaded(WidgetNumber widget, bool value)
{
	LeafWidget *lw = this->GetWidget<LeafWidget>(widget);
	if (lw->IsShaded() != value) {
		lw->SetShaded(value);
		lw->MarkDirty(this->rect.base);
	}
}

/**
 * Is widget \a widget shaded?
 * @param widget %Widget number to use.
 * @return Whether the widget is shaded.
 */
bool GuiWindow::IsWidgetShaded(WidgetNumber widget) const
{
	return this->GetWidget<LeafWidget>(widget)->IsShaded();
}

/**
 * Change the state of a set of radio buttons (#WT_RADIOBUTTON or bi-stable button widgets).
 * @param wids Array of widgets, terminate with #INVALID_WIDGET_INDEX.
 * @param selected Current selected widget of the radio buttons.
 */
void GuiWindow::SetRadioButtonsSelected(const WidgetNumber *wids, WidgetNumber selected)
{
	while (*wids != INVALID_WIDGET_INDEX) {
		BaseWidget *wid = this->GetWidget<BaseWidget>(*wids);
		if (wid->wtype == WT_RADIOBUTTON) {
			this->SetWidgetChecked(*wids, *wids == selected);
		} else {
			this->SetWidgetPressed(*wids, *wids == selected);
		}
		wids++;
	}
}

/**
 * Find the currently selected widget from a set of radio buttons (#WT_RADIOBUTTON or bi-stable button widgets).
 * @param wids Array of widgets, terminate with #INVALID_WIDGET_INDEX.
 * @return Currently pressed widget, or #INVALID_WIDGET_INDEX if none found.
 */
WidgetNumber GuiWindow::GetSelectedRadioButton(const WidgetNumber *wids)
{
	while (*wids != INVALID_WIDGET_INDEX) {
		LeafWidget *wid = this->GetWidget<LeafWidget>(*wids);
		if (!wid->IsShaded()) {
			if ((wid->wtype == WT_RADIOBUTTON && wid->IsChecked()) ||
					(wid->wtype != WT_RADIOBUTTON && wid->IsPressed())) return *wids;
		}
		wids++;
	}
	return INVALID_WIDGET_INDEX;
}

/* virtual */ void GuiWindow::TimeoutCallback()
{
	this->tree->AutoRaiseButtons(this->rect.base);
	if ((this->flags & WF_HIGHLIGHT) != 0) this->SetHighlight(false);
}

/* virtual */ void GuiWindow::SetHighlight(bool value)
{
	if (value) {
		this->flags |= WF_HIGHLIGHT;
		this->timeout = 5;
	} else {
		this->flags &= ~WF_HIGHLIGHT;
	}
	this->MarkDirty();
}

/**
 * %Window manager default constructor.
 * @todo Move this to a separate InitWindowSystem?
 */
WindowManager::WindowManager()
{
	this->top = NULL;
	this->bottom = NULL;

	/* Mouse event handling. */
	this->mouse_pos.x = -10000; // A very unlikely position for a window.
	this->mouse_pos.y = -10000;
	this->current_window = NULL;
	this->mouse_state = 0;
	this->mouse_mode = WMMM_PASS_THROUGH;
};

/** %Window manager destructor. */
WindowManager::~WindowManager()
{
}

/** Close all windows at the display. */
void WindowManager::CloseAllWindows()
{
	while (this->top != NULL) {
		this->DeleteWindow(this->top);
	}
}

/**
 * Get the z-priority of a window type (higher number means further up in the window stack).
 * @param wt Window type.
 * @return Z-priority of the provided type.
 * @ingroup window_group
 */
static uint GetWindowZPriority(WindowTypes wt)
{
	switch (wt) {
		case WC_ERROR_MESSAGE:  return 10; // Error messages.
		default:                return 5;  // 'Normal' window.
		case WC_TOOLBAR:        return 1;  // Top toolbar.
		case WC_BOTTOM_TOOLBAR: return 1;  // Bottom toolbar.
		case WC_MAINDISPLAY:    return 0;  // Main display at the bottom of the stack.
	}
	NOT_REACHED();
}

/**
 * Add a window to the window stack.
 * @param w Window to add.
 */
void WindowManager::AddToStack(Window *w)
{
	assert(w->lower == NULL && w->higher == NULL);
	assert(!this->HasWindow(w));

	uint w_prio = GetWindowZPriority(w->wtype);
	if (this->top == NULL || w_prio >= GetWindowZPriority(this->top->wtype)) {
		/* Add to the top. */
		w->lower = this->top;
		w->higher = NULL;
		if (this->top != NULL) this->top->higher = w;
		this->top = w;
		if (this->bottom == NULL) this->bottom = w;
		return;
	}
	Window *stack = this->top;
	while (stack->lower != NULL && w_prio < GetWindowZPriority(stack->lower->wtype)) stack = stack->lower;

	w->lower = stack->lower;
	if (stack->lower != NULL) {
		stack->lower->higher = w;
	} else {
		assert(this->bottom == stack);
		this->bottom = w;
	}
	w->higher = stack;
	stack->lower = w;
}

/**
 * Remove a window from the list.
 * @param w Window to remove.
 */
void WindowManager::RemoveFromStack(Window *w)
{
	assert(this->HasWindow(w));

	if (w->higher == NULL) {
		this->top = w->lower;
	} else {
		w->higher->lower = w->lower;
	}

	if (w->lower == NULL) {
		this->bottom = w->higher;
	} else {
		w->lower->higher = w->higher;
	}

	w->higher = NULL;
	w->lower  = NULL;
}

/**
 * Delete a window.
 * @param w Window to remove.
 */
void WindowManager::DeleteWindow(Window *w)
{
	this->RemoveFromStack(w);
	w->MarkDirty();
	delete w;
}

/**
 * Raise a window.
 * @param w Window to raise.
 */
void WindowManager::RaiseWindow(Window *w)
{
	if (w != this->top && GetWindowZPriority(w->wtype) >= GetWindowZPriority(w->higher->wtype)) {
		this->RemoveFromStack(w);
		this->AddToStack(w);
		w->MarkDirty();
	}
}


/**
 * Test whether a particular window exists in the window stack.
 * @param w Window to look for.
 * @return Window exists in the window stack.
 * @note Mainly used for paranoia checking.
 */
bool WindowManager::HasWindow(Window *w)
{
	Window *v = this->top;
	while (v != NULL) {
		if (v == w) return true;
		v = v->lower;
	}
	return false;
}

/**
 * Find the window that covers a given position of the display.
 * @param pos Position of the display.
 * @return The window displayed at the given position, or \c NULL if no window.
 */
Window *WindowManager::FindWindowByPosition(const Point16 &pos) const
{
	Window *w = this->top;
	while (w != NULL) {
		if (w->rect.IsPointInside(pos)) break;
		w = w->lower;
	}
	return w;
}

/**
 * Return the current mouse position.
 * @return The last reported mouse position.
 */
Point16 WindowManager::GetMousePosition() const
{
	return this->mouse_pos;
}

/**
 * Mouse moved to new coordinates.
 * @param pos New position of the mouse.
 */
void WindowManager::MouseMoveEvent(const Point16 &pos)
{
	if (pos == this->mouse_pos) return;
	this->mouse_pos = pos;

	switch (this->mouse_mode) {
		case WMMM_PASS_THROUGH: {
			this->UpdateCurrentWindow();
			if (this->current_window == NULL) {
				return;
			}

			/* Compute position relative to window origin. */
			Point16 pos2;
			pos2.x = pos.x - this->current_window->rect.base.x;
			pos2.y = pos.y - this->current_window->rect.base.y;
			this->current_window->OnMouseMoveEvent(pos2);
			break;
		}

		case WMMM_MOVE_WINDOW: {
			if ((this->mouse_state & MB_LEFT) != MB_LEFT) {
				this->mouse_mode = WMMM_PASS_THROUGH;
				return;
			}
			this->current_window->MarkDirty();
			Point32 new_pos;
			new_pos.x = pos.x - this->move_offset.x;
			new_pos.y = pos.y - this->move_offset.y;
			assert(this->current_window->wtype != WC_MAINDISPLAY); // Cannot move the main display!
			this->current_window->rect.base = new_pos;
			this->current_window->MarkDirty();
			break;
		}

		default:
			NOT_REACHED();
	}
}

/**
 * Update the #current_window variable.
 * This may happen when the mouse has moved, but also because of a change in the window stack.
 * @todo Hook a call to this function into the window stack changing code.
 */
void WindowManager::UpdateCurrentWindow()
{
	Window *w = this->FindWindowByPosition(this->mouse_pos);
	if (w == this->current_window) return;

	/* Windows are different, send mouse leave/enter events. */
	if (this->current_window != NULL && this->HasWindow(this->current_window)) this->current_window->OnMouseLeaveEvent();

	this->current_window = w;
	if (this->current_window != NULL) this->current_window->OnMouseEnterEvent();
}

/**
 * Initiate a window movement operation.
 */
void WindowManager::StartWindowMove()
{
	if (this->current_window == NULL) {
		this->mouse_mode = WMMM_PASS_THROUGH;
		return;
	}

	if (!this->current_window->rect.IsPointInside(this->mouse_pos)) {
		this->mouse_mode = WMMM_PASS_THROUGH;
		return;
	}

	this->move_offset.x = this->mouse_pos.x - this->current_window->rect.base.x;
	this->move_offset.y = this->mouse_pos.y - this->current_window->rect.base.y;
	this->mouse_mode = WMMM_MOVE_WINDOW;
}

/**
 * A mouse button was pressed or released.
 * @param button The button that changed state.
 * @param pressed The button was pressed (\c false means it was released instead).
 */
void WindowManager::MouseButtonEvent(MouseButtons button, bool pressed)
{
	assert(button == MB_LEFT || button == MB_MIDDLE || button == MB_RIGHT);
	uint8 newstate = this->mouse_state;
	if (pressed) {
		newstate |= button;
	} else {
		newstate &= ~button;
	}

	this->UpdateCurrentWindow();
	if (this->current_window == NULL) {
		this->mouse_mode = WMMM_PASS_THROUGH;
		this->mouse_state = newstate;
		return;
	}

	if (button == MB_LEFT && pressed) this->RaiseWindow(this->current_window);

	switch (this->mouse_mode) {
		case WMMM_PASS_THROUGH:
			if (newstate != this->mouse_state) {
				WmMouseEvent me = this->current_window->OnMouseButtonEvent((this->mouse_state << 4) | newstate);
				switch (me) {
					case WMME_NONE:
						break;

					case WMME_MOVE_WINDOW:
						this->StartWindowMove();
						break;

					case WMME_CLOSE_WINDOW:
						this->DeleteWindow(this->current_window);
						break;

					default:
						NOT_REACHED();
				}
			}
			break;

		case WMMM_MOVE_WINDOW:
			this->mouse_mode = WMMM_PASS_THROUGH; // Mouse clicks stop window movement.
			break;

		default:
			NOT_REACHED();
	}
	this->mouse_state = newstate;
}

/**
 * The mouse wheel has been turned.
 * @param direction Direction of the turn (either \c +1 or \c -1).
 */
void WindowManager::MouseWheelEvent(int direction)
{
	this->UpdateCurrentWindow();
	if (this->current_window != NULL) this->current_window->OnMouseWheelEvent(direction);
}

/**
 * Redraw (parts of) the windows.
 * @todo [medium/difficult] Do this much less stupid.
 * @ingroup window_group
 */
void UpdateWindows()
{
	if (_video == NULL || !_video->DisplayNeedsRepaint()) return;

	/* Until the entire background is covered by the main display, clean the entire display to ensure deleted
	 * windows truly disappear (even if there is no other window behind it).
	 */
	Rectangle32 rect(0, 0, _video->GetXSize(), _video->GetYSize());
	_video->FillSurface(COL_BACKGROUND, rect);

	Window *w = _manager.bottom;
	while (w != NULL) {
		_video->LockSurface();
		w->OnDraw();
		_video->UnlockSurface();

		w = w->higher;
	}

	_video->FinishRepaint();
}

/** A tick has passed, update whatever must be updated. */
void WindowManager::Tick()
{
	Window *w = _manager.top;
	while (w != NULL) {
		if (w->timeout > 0) {
			w->timeout--;
			if (w->timeout == 0) w->TimeoutCallback();
		}
		w = w->lower;
	}

	UpdateWindows();
}

/**
 * Find an opened window by window type.
 * @param wtype %Window type to look for.
 * @param wnumber %Window number to look for (use #ALL_WINDOWS_OF_TYPE for any window of type \a wtype).
 * @return The window with the requested type if it is opened, else \c NULL.
 * @ingroup window_group
 */
Window *GetWindowByType(WindowTypes wtype, WindowNumber wnumber)
{
	Window *w = _manager.top;
	while (w != NULL) {
		if (w->wtype == wtype && (wnumber == ALL_WINDOWS_OF_TYPE || w->wnumber == wnumber)) return w;
		w = w->lower;
	}
	return NULL;
}

/**
 * Notify the window of the given type of the change with the specified number.
 * @param wtype %Window type to look for.
 * @param wnumber %Window number to look for (use #ALL_WINDOWS_OF_TYPE for any window of type \a wtype).
 * @param code Unique change number.
 * @param parameter Parameter of the change number
 */
void NotifyChange(WindowTypes wtype, WindowNumber wnumber, ChangeCode code, uint32 parameter)
{
	Window *w = GetWindowByType(wtype, wnumber);
	if (w != NULL) w->OnChange(code, parameter);
}

/**
 * Highlight and raise a window of a given type.
 * @param wtype %Window type to look for.
 * @param wnumber %Window number to look for (use #ALL_WINDOWS_OF_TYPE for any window of type \a wtype).
 * @return A window has been highlighted and raised.
 * @ingroup window_group
 */
bool HighlightWindowByType(WindowTypes wtype, WindowNumber wnumber)
{
	Window *w = GetWindowByType(wtype, wnumber);
	if (w != NULL) {
		_manager.RaiseWindow(w);
		w->SetHighlight(true);
	}

	return w != NULL;
}

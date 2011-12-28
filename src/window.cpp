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
#include "widget.h"
#include "video.h"
#include "palette.h"

/**
 * %Window manager.
 * @ingroup window_group
 */
WindowManager _manager;

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
 * @param wtype %Window type (for finding a window in the stack).
 */
Window::Window(WindowTypes wtype) : rect(0, 0, 0, 0), wtype(wtype)
{
	this->timeout = 0;
	this->higher = NULL;
	this->lower  = NULL;
	this->flags  = 0;

	_manager.AddTostack(this); // Add to window stack.
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
void Window::SetPosition(int x, int y)
{
	this->rect.base.x = x;
	this->rect.base.y = y;
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
/* virtual */ void Window::OnChange(int code, uint32 parameter) { }

/**
 * Gui window constructor.
 * @param wtype %Window type (for finding a window in the stack).
 * @note Initialize the widget tree from the derived window class.
 */
GuiWindow::GuiWindow(WindowTypes wtype) : Window(wtype)
{
	this->mouse_pos.x = -1;
	this->mouse_pos.y = -1;
	this->tree = NULL;
	this->widgets = NULL;
	this->num_widgets = 0;
	this->SetHighlight(true);
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

	this->tree->SetupMinimalSize(this->widgets);
	this->rect = Rectangle32(0, 0, this->tree->min_x, this->tree->min_y);

	Rectangle16 min_rect(0, 0, this->tree->min_x, this->tree->min_y);
	this->tree->SetSmallestSizePosition(min_rect);
}

/* virtual */ void GuiWindow::OnDraw()
{
	this->tree->Draw(this->rect.base);
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
/* virtual */ void GuiWindow::OnClick(int16 widget)
{
}

/* virtual */ void GuiWindow::TimeoutCallback()
{
	this->tree->RaiseButtons(this->rect.base);
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
		default:             return 5; // 'Normal' window.
		case WC_MAINDISPLAY: return 0; // Main display at the bottom of the stack.
	}
	NOT_REACHED();
}

/**
 * Add a window to the window stack.
 * @param w Window to add.
 */
void WindowManager::AddTostack(Window *w)
{
	assert(w->lower == NULL && w->higher == NULL);
	assert(!this->HasWindow(w));

	uint w_prio = GetWindowZPriority(w->wtype);
	if (this->top == NULL || w_prio > GetWindowZPriority(this->top->wtype)) {
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
 * Delete a window.
 * @param w Window to remove.
 * @return Removed window.
 */
void WindowManager::DeleteWindow(Window *w)
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
	w->MarkDirty();
	delete w;
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
 * Mouse moved to new coordinates.
 * @param pos New position of the mouse.
 */
void WindowManager::MouseMoveEvent(const Point16 &pos)
{
	if (pos == this->mouse_pos) return;
	this->mouse_pos = pos;

	this->UpdateCurrentWindow();
	if (this->current_window == NULL) {
		this->mouse_mode = WMMM_PASS_THROUGH;
		return;
	}

	switch (this->mouse_mode) {
		case WMMM_PASS_THROUGH: {
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
 * @return Window has changed.
 * @todo Hook a call to this function into the window stack changing code.
 */
bool WindowManager::UpdateCurrentWindow()
{
	Window *w = this->FindWindowByPosition(this->mouse_pos);
	if (w == this->current_window) return false;

	/* Windows are different, send mouse leave/enter events. */
	if (this->current_window != NULL && this->HasWindow(this->current_window)) this->current_window->OnMouseLeaveEvent();

	this->current_window = w;
	if (this->current_window != NULL) this->current_window->OnMouseEnterEvent();
	return true;
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
 * @return The window with the requested type if it is opened, else \c NULL.
 * @ingroup window_group
 */
Window *GetWindowByType(WindowTypes wtype)
{
	Window *w = _manager.top;
	while (w != NULL) {
		if (w->wtype == wtype) return w;
		w = w->lower;
	}
	return NULL;
}

/**
 * Notify the window of the given type of the change with the specified number.
 * @param wtype %Window type to look for.
 * @param code Unique change number.
 * @param parameter Parameter of the change number
 */
void NotifyChange(WindowTypes wtype, int code, uint32 parameter)
{
	Window *w = GetWindowByType(wtype);
	if (w != NULL) w->OnChange(code, parameter);
}

/**
 * Highlight a window of a given type.
 * @param wtype %Window type to look for.
 * @return A window has been highlighted.
 * @ingroup window_group
 */
bool HighlightWindowByType(WindowTypes wtype)
{
	Window *w = GetWindowByType(wtype);
	if (w != NULL) w->SetHighlight(true);
	return w != NULL;
}

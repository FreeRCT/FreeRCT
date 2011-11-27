/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file window.cpp %Window handling functions. */

#include "stdafx.h"
#include "window.h"
#include "video.h"

WindowManager _manager; ///< %Window manager.

/**
 * %Window constructor. It automatically adds the window to the window stack.
 * @param wtype %Window type (for finding a window in the stack).
 */
Window::Window(WindowTypes wtype) : rect(0, 0, 0, 0), wtype(wtype)
{
	this->higher = NULL;
	this->lower  = NULL;

	_manager.AddTostack(this); // Add to window stack.
}

/**
 * Destructor.
 * Do not call directly, delete the return value of #WindowManager::RemoveFromStack instead.
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
void Window::SetSize(uint width, uint height)
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
	_video->MarkDisplayDirty();
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
/* virtual */ void Window::OnMouseButtonEvent(uint8 state) { }

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
};

/** %Window manager destructor. */
WindowManager::~WindowManager()
{
	while (this->top != NULL) {
		delete this->RemoveFromStack(this->top);
	}
}

/**
 * Get the z-priority of a window type (higher number means further up in the window stack).
 * @param wt Window type.
 * @return Z-priority of the provided type.
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
 * Remove a window from the stack.
 * @param w Window to remove.
 * @return Removed window.
 */
Window *WindowManager::RemoveFromStack(Window *w)
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
	return w;
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
		if (pos.x > w->rect.base.x
				&& pos.y > w->rect.base.y
				&& pos.x < w->rect.base.x + (int)w->rect.width
				&& pos.y < w->rect.base.y + (int)w->rect.height) break;
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
	if (this->current_window != NULL) {
		/* Compute position relative to window origin. */
		Point16 pos2;
		pos2.x = pos.x - this->current_window->rect.base.x;
		pos2.y = pos.y - this->current_window->rect.base.y;
		this->current_window->OnMouseMoveEvent(pos2);
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
	if (newstate != this->mouse_state && this->current_window != NULL) {
		this->current_window->OnMouseButtonEvent((this->mouse_state << 4) | newstate);
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
 */
void UpdateWindows()
{
	if (_video == NULL || !_video->DisplayNeedsRepaint()) return;

	Window *w = _manager.bottom;
	while (w != NULL) {
		_video->LockSurface();
		w->OnDraw();
		_video->UnlockSurface();

		w = w->higher;
	}

	_video->FinishRepaint();
}

/**
 * Find an opened window by window type.
 * @param wtype %Window type to look for.
 * @return The window with the requested type if it is opened, else \c NULL.
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

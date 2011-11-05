/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file window.h %Window handling data structures. */

#ifndef WINDOW_H
#define WINDOW_H

#include "geometry.h"
#include "orientation.h"

class VideoSystem;

/** Available types of windows. */
enum WindowTypes {
	WC_MAINDISPLAY, ///< Main display of the world.
};

/** Known mouse buttons. */
enum MouseButtons {
	MB_LEFT   = 1, ///< Left button down.
	MB_MIDDLE = 2, ///< Middle button down.
	MB_RIGHT  = 4, ///< Right button down.

	MB_CURRENT  = 0x07, ///< Bitmask for current mouse state.
	MB_PREVIOUS = 0x70, ///< Bitmask for previous mouse state.
};
DECLARE_ENUM_AS_BIT_SET(MouseButtons)

/** Known mouse modes. */
enum MouseMode {
	MM_INACTIVE,       ///< Inactive mode.
	MM_TILE_TERRAFORM, ///< Terraforming tiles.

	MM_COUNT,          ///< Number of mouse modes.
};

/**
 * %Window base class, extremely simple (just a viewport).
 * @todo Make it a real window with widgets, title bar, and stuff.
 */
class Window {
public:
	Window(WindowTypes wtype);
	virtual ~Window();

	Rectangle rect;    ///< Screen area covered by the window.
	WindowTypes wtype; ///< Window type.

	Window *higher; ///< Window above this window (managed by #WindowManager).
	Window *lower;  ///< Window below this window (managed by #WindowManager).

	void SetSize(uint width, uint height);
	void SetPosition(int x, int y);

	void MarkDirty();

	virtual void OnDraw(VideoSystem *video);
	virtual void OnMouseMoveEvent(const Point16 &pos);
	virtual void OnMouseButtonEvent(uint8 state);
	virtual void OnMouseWheelEvent(int direction);
	virtual void OnMouseEnterEvent();
	virtual void OnMouseLeaveEvent();
};

/**
 * %Window manager class, manages the window stack.
 */
class WindowManager {
public:
	WindowManager();
	~WindowManager();

	bool HasWindow(Window *w);
	void AddTostack(Window *w);
	Window *RemoveFromStack(Window *w);

	void MouseMoveEvent(const Point16 &pos);
	void MouseButtonEvent(MouseButtons button, bool pressed);
	void MouseWheelEvent(int direction);

	Window *top;        ///< Top-most window in the window stack.
	Window *bottom;     ///< Lowest window in the window stack.
	VideoSystem *video; ///< Video output device.

private:
	Window *FindWindowByPosition(const Point16 &pos) const;
	bool UpdateCurrentWindow();

	Point16 mouse_pos;      ///< Last reported mouse position.
	Window *current_window; ///< 'Current' window under the mouse.
	uint8 mouse_state;      ///< Last reported mouse button state (lower 4 bits).
};

extern WindowManager _manager;

void UpdateWindows();
Window *GetWindowByType(WindowTypes wtype);

#endif

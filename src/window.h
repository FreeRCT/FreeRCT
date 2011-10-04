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
	Window(int x, int y, uint w, uint h, WindowTypes wtype);
	virtual ~Window();

	int x;             ///< X position of top-left corner.
	int y;             ///< Y position of top-left corner.
	uint width;        ///< Width of the window.
	uint height;       ///< Height of the window.
	WindowTypes wtype; ///< Window type.

	Window *higher; ///< Window above this window (managed by #WindowManager).
	Window *lower;  ///< Window below this window (managed by #WindowManager).

	void MarkDirty();

	virtual void OnDraw();
	virtual void OnMouseMoveEvent(const Point16 &pos);
	virtual void OnMouseButtonEvent(uint8 state);
	virtual void OnMouseWheelEvent(int direction);
	virtual void OnMouseEnterEvent();
	virtual void OnMouseLeaveEvent();
};

/**
 * Class for displaying parts of the world.
 * @todo Make it a widget?
 */
class Viewport : public Window {
public:
	Viewport(int x, int y, uint w, uint h);

	virtual void OnDraw();

	/**
	 * Convert 3D position to the horizontal 2D position.
	 * @param x X position in the game world.
	 * @param y Y position in the game world.
	 * @return X position in 2D.
	 */
	FORCEINLINE int32 ComputeX(int32 x, int32 y)
	{
		switch (this->orientation) {
			case VOR_NORTH: return ((y - x)  * this->tile_width / 2) >> 8;
			case VOR_WEST:  return (-(x + y) * this->tile_width / 2) >> 8;
			case VOR_SOUTH: return ((x - y)  * this->tile_width / 2) >> 8;
			case VOR_EAST:  return ((x + y)  * this->tile_width / 2) >> 8;
			default: NOT_REACHED();
		}
	}

	/**
	 * Convert 3D position to the vertical 2D position.
	 * @param x X position in the game world.
	 * @param y Y position in the game world.
	 * @param z Z position in the game world.
	 * @return Y position in 2D.
	 */
	FORCEINLINE int32 ComputeY(int32 x, int32 y, int32 z)
	{
		switch (this->orientation) {
			case VOR_NORTH: return ((x + y)  * this->tile_width / 4 - z * this->tile_height) >> 8;
			case VOR_WEST:  return ((y - x)  * this->tile_width / 4 - z * this->tile_height) >> 8;
			case VOR_SOUTH: return (-(x + y) * this->tile_width / 4 - z * this->tile_height) >> 8;
			case VOR_EAST:  return ((x - y)  * this->tile_width / 4 - z * this->tile_height) >> 8;
			default: NOT_REACHED();
		}
	}

	void Rotate(int direction);
	void MoveViewport(int dx, int dy);

	void SetMouseMode(MouseMode mode);
	virtual void OnMouseMoveEvent(const Point16 &pos);
	virtual void OnMouseButtonEvent(uint8 state);
	virtual void OnMouseEnterEvent();
	virtual void OnMouseLeaveEvent();

	int32 xview; ///< X position of the center point of the viewport.
	int32 yview; ///< Y position of the center point of the viewport.
	int32 zview; ///< Z position of the center point of the viewport.

	uint16 tile_width;           ///< Width of a tile.
	uint16 tile_height;          ///< Height of a tile.
	ViewOrientation orientation; ///< Direction of view.

private:
	MouseMode mouse_mode; ///< Mode of the mouse, decides how to react to mouse clicks, drags, etc.
	Point16 mouse_pos;    ///< Last known position of the mouse.
	uint8 mouse_state;    ///< Last known state of the mouse buttons.
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


void SetVideo(VideoSystem *vid);
VideoSystem *GetVideo();

void UpdateWindows();
Window *GetWindowByType(WindowTypes wtype);

#endif

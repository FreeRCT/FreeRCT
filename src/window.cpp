/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file window.h Window handling functions. */

#include "stdafx.h"
#include "window.h"
#include "voxel_map.h"
#include "video.h"

/**
 * Window manager.
 */
class WindowManager {
public:
	WindowManager();
	~WindowManager();

	bool HasWindow(Window *w);
	void AddTostack(Window *w);
	Window *RemoveFromStack(Window *w);

	Window *top;        ///< Top-most window in the window stack.
	Window *bottom;     ///< Lowest window in the window stack.
	VideoSystem *video; ///< Video output device.
};

static WindowManager _manager; ///< Window manager.

/**
 * Default constructor.
 * @todo Make #Viewport a derived class of #Window.
 */
Viewport::Viewport(int x, int y, uint w, uint h)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;

	this->xview = _world.GetXSize() / 2;
	this->yview = _world.GetYSize() / 2;
	this->zview = 0;
}

/**
 * Window constructor.
 * @param xp X position top-left corner.
 * @param yp Y position top-left corner.
 * @param wp Width of the window.
 * @param hp Height of the window.
 */
Window::Window(int xp, int yp, uint wp, uint hp) : x(xp), y(yp), w(wp), h(hp), vport(xp, yp, wp, hp)
{
	this->higher = NULL;
	this->lower  = NULL;

	_manager.AddTostack(this); // Add to window stack.
}

Window::~Window()
{
	assert(!_manager.HasWindow(this));
}

/**
 * Paint the window to the screen.
 */
void Window::OnDraw()
{
}

/**
 * Window manager default constructor.
 * @todo Move this to a separate InitWindowSystem, together with #SetVideo?
 */
WindowManager::WindowManager()
{
	this->top = NULL;
	this->bottom = NULL;

	this->video = NULL;
};

/** Window manager destructor. */
WindowManager::~WindowManager()
{
	while (this->top != NULL) {
		delete this->RemoveFromStack(this->top);
	}
}

/**
 * Set the video output device.
 * @param video Video output to use for drawing windows.
 */
void SetVideo(VideoSystem *vid)
{
	_manager.video = vid;
}

/**
 * Add a window to the window stack.
 * @param w Window to add.
 */
void WindowManager::AddTostack(Window *w)
{
	assert(w->lower == NULL && w->higher == NULL);
	assert(!this->HasWindow(w));

	w->lower = this->top;
	w->higher = NULL;
	if (this->bottom == NULL) this->bottom = w;
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
 * Redraw (parts of) the windows.
 * @todo Do this much less stupid.
 */
void UpdateWindows()
{
	if (_manager.video == NULL) return;

	Window *w = _manager.bottom;
	while (w != NULL) {
		w->OnDraw();
		w = w->higher;
	}
}

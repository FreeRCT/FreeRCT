/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file window.h Window handling data structures. */

#ifndef WINDOW_H
#define WINDOW_H

/**
 * Class for displaying parts of the world.
 */
class Viewport {
public:
	Viewport(int x, int y, uint w, uint h);

	int x;  ///< X position of top-left corner.
	int y;  ///< Y position of top-left corner.
	uint w; ///< Width of the viewport.
	uint h; ///< Height of the viewport.

	int xview; ///< X position of the center point of the viewport.
	int yview; ///< Y position of the center point of the viewport.
	int zview; ///< Z position of the center point of the viewport.
};


/**
 * %Window base class, extremely simple (just a viewport).
 * @todo Make it a real window with widgets, title bar, and stuff.
 */
class Window {
public:
	Window(int x, int y, uint w, uint h);
	virtual ~Window();

	int x;  ///< X position of top-left corner.
	int y;  ///< Y position of top-left corner.
	uint w; ///< Width of the window.
	uint h; ///< Height of the window.

	Viewport vport; ///< Temporary viewport member.

	Window *higher; ///< Window above this window (managed by #WindowManager).
	Window *lower;  ///< Window below this window (managed by #WindowManager).

	virtual void OnDraw();
};

class VideoSystem;

void SetVideo(VideoSystem *vid);
void UpdateWindows();

#endif

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

/** Available types of windows. */
enum WindowTypes {
	WC_MAINDISPLAY, ///< Main display of the world.
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
	 * Convert 3D position to a 2D position.
	 * @param x X position in the game world.
	 * @param y Y position in the game world.
	 * @param z Z position in the game world.
	 * @return X and Y position in 2D.
	 */
	FORCEINLINE Point ComputeXY(int32 x, int32 y, int32 z)
	{
		Point p;
		switch (this->orientation) {
			case VOR_NORTH:
				p.x = ((y - x) * this->tile_width / 2) >> 8;
				p.y = ((x + y) * this->tile_width / 4 - z * this->tile_height) >> 8;
				break;

			case VOR_EAST:
				p.x = (-(x + y) * this->tile_width / 2) >> 8;
				p.y = ((y - x) * this->tile_width / 4 - z * this->tile_height) >> 8;
				break;

			case VOR_SOUTH:
				p.x = ((x - y) * this->tile_width / 2) >> 8;
				p.y = (-(x + y) * this->tile_width / 4 - z * this->tile_height) >> 8;
				break;

			case VOR_WEST:
				p.x = ((x + y) * this->tile_width / 2) >> 8;
				p.y = ((x - y) * this->tile_width / 4 - z * this->tile_height) >> 8;
				break;

			default:
				NOT_REACHED();
		}
		return p;
	}

	void Rotate(int direction);

	int32 xview; ///< X position of the center point of the viewport.
	int32 yview; ///< Y position of the center point of the viewport.
	int32 zview; ///< Z position of the center point of the viewport.

	uint16 tile_width;           ///< Width of a tile.
	uint16 tile_height;          ///< Height of a tile.
	ViewOrientation orientation; ///< Direction of view.
};

class VideoSystem;

void SetVideo(VideoSystem *vid);
VideoSystem *GetVideo();

void UpdateWindows();
Window *GetWindowByType(WindowTypes wtype);

#endif

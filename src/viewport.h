/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file viewport.h %Main display data. */

#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "window.h"

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

#endif

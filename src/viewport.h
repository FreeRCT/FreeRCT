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
 */
class Viewport : public Window {
public:
	Viewport(int x, int y, uint w, uint h);

	virtual void OnDraw();

	void Rotate(int direction);
	void MoveViewport(int dx, int dy);

	void SetMouseMode(MouseMode mode);

	void ComputeCursorPosition();

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

	uint16 xvoxel;          ///< X position of the voxel with the mouse cursor.
	uint16 yvoxel;          ///< Y position of the voxel with the mouse cursor.
	uint8  zvoxel;          ///< Z position of the voxel with the mouse cursor.
	ViewOrientation cursor; ///< Cursor orientation (#VOR_INVALID means entire ground tile).

	virtual void OnMouseMoveEvent(const Point16 &pos);
	virtual void OnMouseButtonEvent(uint8 state);
	virtual void OnMouseEnterEvent();
	virtual void OnMouseLeaveEvent();
};

#endif

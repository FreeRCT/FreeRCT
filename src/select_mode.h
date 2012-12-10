/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file select_mode.h Mouse mode to select objects from the main display. */

#ifndef SELECT_MODE_H
#define SELECT_MODE_H

/** Mouse mode to select objects with a mouse click. */
class SelectMouseMode : public MouseMode {
public:
	SelectMouseMode();
	~SelectMouseMode();

	/* virtual */ bool MayActivateMode();
	/* virtual */ void ActivateMode(const Point16 &pos);
	/* virtual */ void LeaveMode();
	/* virtual */ void OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos);
	/* virtual */ void OnMouseButtonEvent(Viewport *vp, uint8 state);
	/* virtual */ bool EnableCursors();

	uint8 mouse_state; ///< Last known mouse button state.
};

extern SelectMouseMode _select_mousemode;

#endif

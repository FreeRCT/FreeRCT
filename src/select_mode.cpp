/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file select_mode.cpp Implementation of the select mouse mode. */

#include "stdafx.h"
#include "viewport.h"
#include "select_mode.h"

SelectMouseMode _select_mousemode; ///< Mouse select mode coordinator.


SelectMouseMode::SelectMouseMode() : MouseMode(WC_NONE, MM_OBJECT_SELECT)
{
	this->mouse_state = 0;
}

SelectMouseMode::~SelectMouseMode()
{
}

/* virtual */ bool SelectMouseMode::MayActivateMode()
{
	return true;
}

/* virtual */ void SelectMouseMode::ActivateMode(const Point16 &pos)
{
	this->mouse_state = 0;
}

/* virtual */ void SelectMouseMode::LeaveMode()
{
}

/* virtual */ void SelectMouseMode::OnMouseMoveEvent(Viewport *vp, const Point16 &old_pos, const Point16 &pos)
{
	if ((this->mouse_state & MB_RIGHT) != 0) {
		/* Drag the window if button is pressed down. */
		vp->MoveViewport(pos.x - old_pos.x, pos.y - old_pos.y);
	}
}

/* virtual */ void SelectMouseMode::OnMouseButtonEvent(Viewport *vp, uint8 state)
{
	this->mouse_state = state & MB_CURRENT;
	if (this->mouse_state != 0) {
		FinderData fdata(SO_RIDE | SO_PERSON, false);
		switch (vp->ComputeCursorPosition(&fdata)) {
			case SO_RIDE:
				ShowShopManagementGui(fdata.ride);
				break;

			case SO_PERSON:
				ShowGuestInfoGui(fdata.person);
				break;

			default: break;
		}
	}
}

/* virtual */ bool SelectMouseMode::EnableCursors()
{
	return false;
}


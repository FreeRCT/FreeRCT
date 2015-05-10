/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file mouse_mode.cpp Mouse mode handling implementation. */

#include "stdafx.h"
#include "mouse_mode.h"

MouseModeSelector::MouseModeSelector() : MouseModeSelector(CUR_TYPE_TILE)
{
}

MouseModeSelector::MouseModeSelector(CursorType cur_cursor)
{
	this->cur_cursor = cur_cursor;
}

MouseModeSelector::~MouseModeSelector()
{
}

/**
 * \fn MouseModeSelector::MarkDirty()
 *  Mark all voxels changed by the selector as dirty, so they get redrawn.
 */

/**
 * \fn CursorMouseMode MouseModeSelector::GetCursor(const XYZPoint16 &voxel_pos) const
 * Retrieve the cursor to display at the given voxel.
 * @param voxel_pos %Voxel to decorate with a cursor.
 * @return The cursor to use, or #CUR_TYPE_INVALID if no cursor should be displayed.
 */

/**
 * \fn MouseModeSelector::GetZRange(uint xpos, uint ypos)
 * Get the vertical range of voxels to render.
 * @param xpos Horizontal position of the stack.
 * @param ypos Vertical position of the stack.
 * @return \c 0 if no interest in rendering in the stack, else lowest voxel
 *         position in lower 16 bit, top voxel position in upper 16 bit.
 */

/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gui_graphics.h Declarations of functions/classes for drawing GUI elements and contents. */

#ifndef GUI_GRAPHICS_H
#define GUI_GRAPHICS_H

#include "video.h"

struct BorderSpriteData;

void DrawBorderSprites(const BorderSpriteData &bsd, bool pressed, const Rectangle32 &rect, uint8 colour);

/**
 * Get the height of the text.
 * @return Height of the text (in pixels).
 */
inline int GetTextHeight()
{
	return _video->GetTextHeight();
}

void DrawString(StringID strid, uint8 colour, int x, int y, int width = 0x7FFF, Alignment align = ALG_LEFT);
bool DrawMultilineString(StringID strid, int x, int y, int max_width, int max_height, uint8 colour);

void GetMultilineTextSize(StringID strid, int max_width, int *width, int *height);

#endif

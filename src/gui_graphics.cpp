/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file gui_graphics.cpp Code for drawing GUI elements and contents. */

#include "stdafx.h"
#include "sprite_store.h"
#include "video.h"

/**
 * Draw border sprites around some contents.
 * @param bsd Border sprites to use.
 * @param pressed Draw pressed down sprites.
 * @param rect Content rectangle to draw around.
 * @param colour Colour range to use.
 * @ingroup widget_group
 */
void DrawBorderSprites(const BorderSpriteData &bsd, bool pressed, const Rectangle32 &rect, uint8 colour)
{
	Point32 pt;
	const ImageData * const *spr_base = pressed ? bsd.pressed : bsd.normal;
	static Recolouring rc; // Only COL_RANGE_BEIGE is modified each time.
	rc.SetRecolouring(COL_RANGE_BEIGE, (ColourRange)colour);

	pt.x = rect.base.x;
	pt.y = rect.base.y;
	_video->BlitImage(pt, spr_base[WBS_TOP_LEFT], rc, 0);
	int xleft = pt.x + spr_base[WBS_TOP_LEFT]->xoffset + spr_base[WBS_TOP_LEFT]->width;
	int ytop = pt.y + spr_base[WBS_TOP_LEFT]->yoffset + spr_base[WBS_TOP_LEFT]->height;

	pt.x = rect.base.x + rect.width - 1;
	_video->BlitImage(pt, spr_base[WBS_TOP_RIGHT], rc, 0);
	int xright = pt.x + spr_base[WBS_TOP_RIGHT]->xoffset;

	uint16 numx = (xright - xleft) / spr_base[WBS_TOP_MIDDLE]->width;
	_video->BlitHorizontal(xleft, numx, pt.y, spr_base[WBS_TOP_MIDDLE], rc);

	pt.x = rect.base.x;
	pt.y = rect.base.y + rect.height - 1;
	_video->BlitImage(pt, spr_base[WBS_BOTTOM_LEFT], rc, 0);
	int ybot = pt.y + spr_base[WBS_BOTTOM_LEFT]->yoffset;

	uint16 numy = (ybot - ytop) / spr_base[WBS_MIDDLE_LEFT]->height;
	_video->BlitVertical(ytop, numy, pt.x, spr_base[WBS_MIDDLE_LEFT], rc);

	pt.x = rect.base.x + rect.width - 1;
	pt.y = rect.base.y + rect.height - 1;
	_video->BlitImage(pt, spr_base[WBS_BOTTOM_RIGHT], rc, 0);

	_video->BlitHorizontal(xleft, numx, pt.y, spr_base[WBS_BOTTOM_MIDDLE], rc);

	pt.x = rect.base.x + rect.width - 1;
	_video->BlitVertical(ytop, numy, pt.x, spr_base[WBS_MIDDLE_RIGHT], rc);

	_video->BlitImages(xleft, ytop, spr_base[WBS_MIDDLE_MIDDLE], numx, numy, rc);
}

/**
 * Draw a string to the screen.
 * @param strid String to draw.
 * @param x X position at the screen.
 * @param y Y position at the screen.
 * @param colour Colour of the text.
 */
void DrawString(StringID strid, int x, int y, uint8 colour)
{
	uint8 buffer[1024]; // Arbitrary limit.

	DrawText(strid, buffer, lengthof(buffer));
	_video->BlitText(buffer, x, y, colour);
}


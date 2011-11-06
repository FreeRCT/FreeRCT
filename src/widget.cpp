/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file widget.cpp Widget code. */

#include "stdafx.h"
#include "sprite_store.h"
#include "widget.h"
#include "window.h"
#include "video.h"

/**
 * Draw border sprites around some contents.
 * @param bsd Border sprites to use.
 * @param pressed Draw pressed down sprites.
 * @param rect Content rectangle to draw around.
 * @todo [easy] Argh, another #_manager.video why is there no _video thingie instead?
 * @todo [??] Similarly, needing to state the bounding box 'screen' seems weird and is not the right solution.
 * @todo #include "window.h" in widgets?!?
 */
static void DrawBorderSprites(VideoSystem *vid, const BorderSpriteData &bsd, bool pressed, const Rectangle &rect)
{
	Point pt;
	const Sprite * const *spr_base = pressed ? bsd.pressed : bsd.normal;

	pt.x = rect.base.x;
	pt.y = rect.base.y;
	vid->BlitImage(pt, spr_base[WBS_TOP_LEFT]);
	int xleft = pt.x + spr_base[WBS_TOP_LEFT]->xoffset + spr_base[WBS_TOP_LEFT]->img_data->width;
	int ytop = pt.y + spr_base[WBS_TOP_LEFT]->yoffset + spr_base[WBS_TOP_LEFT]->img_data->height;

	pt.x = rect.base.x + rect.width - 1;
	vid->BlitImage(pt, spr_base[WBS_TOP_RIGHT]);
	int xright = pt.x + spr_base[WBS_TOP_RIGHT]->xoffset;

	uint16 numx = (xright - xleft) / spr_base[WBS_TOP_MIDDLE]->img_data->width;
	vid->BlitHorizontal(xleft, numx, pt.y, spr_base[WBS_TOP_MIDDLE]);

	pt.x = rect.base.x;
	pt.y = rect.base.y + rect.height - 1;
	vid->BlitImage(pt, spr_base[WBS_BOTTOM_LEFT]);
	int ybot = pt.y + spr_base[WBS_BOTTOM_LEFT]->yoffset;

	uint16 numy = (ybot - ytop) / spr_base[WBS_MIDDLE_LEFT]->img_data->height;
	vid->BlitVertical(ytop, numy, pt.x, spr_base[WBS_MIDDLE_LEFT]);

	pt.x = rect.base.x + rect.width - 1;
	pt.y = rect.base.y + rect.height - 1;
	vid->BlitImage(pt, spr_base[WBS_BOTTOM_RIGHT]);

	vid->BlitHorizontal(xleft, numx, pt.y, spr_base[WBS_BOTTOM_MIDDLE]);

	pt.x = rect.base.x + rect.width - 1;
	vid->BlitVertical(ytop, numy, pt.x, spr_base[WBS_MIDDLE_RIGHT]);

	vid->BlitImages(xleft, ytop, spr_base[WBS_MIDDLE_MIDDLE], numx, numy);
}

/**
 * Draw a panel at the given coordinates.
 * @param rect Outer rectangle to draw in.
 * @todo [difficult] Outer rectangle does not sound very useful in general.
 */
void DrawPanel(VideoSystem *vid, const Rectangle &rect)
{
	const BorderSpriteData &bsd = _gui_sprites.panel;
	Rectangle rect2(rect.base.x + bsd.border_left, rect.base.y + bsd.border_top,
			rect.width - bsd.border_left -  bsd.border_right, rect.height - bsd.border_top - bsd.border_bottom);

	DrawBorderSprites(vid, bsd, false, rect2);
}


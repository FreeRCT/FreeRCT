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
#include "math_func.h"
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
 * @param colour Colour of the text.
 * @param x X position at the screen.
 * @param y Y position at the screen.
 * @param width Maximal width of the text.
 */
void DrawString(StringID strid, uint8 colour, int x, int y, int width)
{
	uint8 buffer[1024]; // Arbitrary limit.

	DrawText(strid, buffer, lengthof(buffer));
	_video->BlitText(buffer, colour, x, y, width);
}

/**
 * Get the text of a single line with an upper limit of the line length. Text may get split on SPACE. NL implies
 * a line-break, and NUL terminates. Note that in UTF-8, explicit tests on these characters works as expected, since
 * the other code points all have bit 7 set of every byte they use.
 * @param text Text to search and split.
 * @param max_width Longest allowed length of a line.
 * @param width [out]  Actual width of the text.
 * @return Pointer to the end of the line, points to either a NL or a NUL.
 */
static uint8 *GetSingleLine(uint8 *text, int max_width, int *width)
{
	uint8 *start = text;
	uint8 *best_pos = NULL;
	for (;;) {
		uint8 *current = text;
		/* Proceed to the first white-space. */
		while (*current != '\n' && *current != '\0' && *current != ' ') current++;
		uint8 orig = *current;
		*current = '\0';
		int line_width, line_height;
		_video->GetTextSize(start, &line_width, &line_height);

		if (line_width < max_width) {
			*width  = line_width;
			if (orig == '\0') return current;
			if (orig == '\n') {
				*current = '\n';
				return current;
			}

			*current = ' '; // orig was ' ' too.
			best_pos = current;
			text = current + 1;
			continue;
		}

		/* line_width >= max_width */
		if (best_pos != NULL) {
			/* There is a best position to fall back to. */
			*current = orig;  // We will revisit this position later.
			*best_pos = '\n'; // Insert line break.
			return best_pos;
		}

		/* line_width >= max_width, no fallback to a previous position. */
		*width  = line_width;
		if (orig == '\n' || orig == ' ') *current = '\n';
		return current;
	}
}

/**
 * Get the size of a text when printed in multi-line format.
 * @param strid Text to 'print'.
 * @param max_width Longest allowed length of a line.
 * @param width [out]  Actual width of the text.
 * @param height [out] Actual height of the text.
 * @note After the call, NL characters have been inserted at line break points.
 * @note Actual width (returned in \c *width) may be larger than \a max_width in case of long words.
 */
void GetMultilineTextSize(StringID strid, int max_width, int *width, int *height)
{
	uint8 buffer[1024]; // Arbitrary max size.
	DrawText(strid, buffer, lengthof(buffer));

	*width = 0;
	*height = 0;

	uint8 *text = buffer;
	for (;;) {
		int line_width;
		text = GetSingleLine(text, max_width, &line_width);
		*width = max(*width, line_width);
		*height += _video->GetTextHeight();

		if (*text == '\0') break;
		assert(*text == '\n');
		text++;
	}
}

/**
 * Draw a string to the screen using several lines.
 * @param strid String to draw.
 * @param x X position at the screen.
 * @param y Y position at the screen.
 * @param max_width Available width of the text.
 * @param max_height Available height of the text.
 * @param colour Colour of the text.
 * @return The height was sufficient to output all lines.
 */
bool DrawMultilineString(StringID strid, int x, int y, int max_width, int max_height, uint8 colour)
{
	uint8 buffer[1024]; // Arbitrary max size.
	DrawText(strid, buffer, lengthof(buffer));

	uint8 *text = buffer;
	while (*text != '\0') {
		if (max_height < _video->GetTextHeight()) return false;
		max_height -= _video->GetTextHeight();

		int line_width;
		uint8 *end = GetSingleLine(text, max_width, &line_width);
		if (*end == '\0') {
			_video->BlitText(text, colour, x, y, max_width);
			break;
		}
		*end = '\0';
		_video->BlitText(text, x, y, colour);
		y += _video->GetTextHeight();
		text = end + 1;
	}
	return true;
}


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
#include "palette.h"
#include "sprite_data.h"
#include "math_func.h"
#include "video.h"

/**
 * Draw the same sprite repeatedly over a (potentially) large area. The function recognizes a single-pixel
 * sprite as a special case, and converts it to a VideoSystem::FillRectangle call.
 * @param x_base Left edge coordinate to start drawing.
 * @param y_base Top edge coordinate to start drawing.
 * @param spr Sprite to draw.
 * @param numx Number of horizontal repeats of the sprite.
 * @param numy Number of vertical repeats of the sprite.
 * @param recolour Recolouring information.
 */
static void DrawRepeatedSprites(int32 x_base, int32 y_base, const ImageData *spr, uint16 numx, uint16 numy, const Recolouring &recolour)
{
	if (numx == 0 || numy == 0) return;
	if (!spr->IsSinglePixel()) {
		_video.BlitImages(x_base, y_base, spr, numx, numy, recolour);
		return;
	}
	uint32 colour = spr->GetPixel(0, 0, &recolour);
	_video.FillRectangle({x_base, y_base, numx, numy}, colour);
}

/**
 * Draw border sprites around some contents.
 * @param bsd Border sprites to use.
 * @param pressed Draw pressed down sprites.
 * @param rect Content rectangle to draw around.
 * @param colour Colour range to use.
 * @ingroup widget_group
 */
void DrawBorderSprites(const BorderSpriteData &bsd, bool pressed, const Rectangle32 &rect, ColourRange colour)
{
	const ImageData * const *spr_base = pressed ? bsd.pressed : bsd.normal;
	static Recolouring rc; // Only COL_RANGE_BROWN is modified each time.
	rc.Set(0, RecolourEntry(COL_RANGE_BROWN, colour));

	Point32 pt = rect.base;
	int xleft = pt.x;
	int ytop = pt.y;
	if (spr_base[WBS_TOP_LEFT] != nullptr) {
		_video.BlitImage(pt, spr_base[WBS_TOP_LEFT], rc, GS_NORMAL);
		xleft += spr_base[WBS_TOP_LEFT]->xoffset + spr_base[WBS_TOP_LEFT]->width;
		ytop += spr_base[WBS_TOP_LEFT]->yoffset + spr_base[WBS_TOP_LEFT]->height;
	}

	pt.x = rect.base.x + rect.width - 1;
	int xright = pt.x;
	if (spr_base[WBS_TOP_RIGHT] != nullptr) {
		_video.BlitImage(pt, spr_base[WBS_TOP_RIGHT], rc, GS_NORMAL);
		xright += spr_base[WBS_TOP_RIGHT]->xoffset;
	}

	uint16 numx = xright - xleft;
	if (spr_base[WBS_TOP_MIDDLE] != nullptr) {
		numx /= spr_base[WBS_TOP_MIDDLE]->width;
	} else if (spr_base[WBS_BOTTOM_MIDDLE] != nullptr) {
		numx /= spr_base[WBS_BOTTOM_MIDDLE]->width;
	} // else assume sprite width is 1.
	if (spr_base[WBS_TOP_MIDDLE] != nullptr) _video.BlitHorizontal(xleft, numx, pt.y, spr_base[WBS_TOP_MIDDLE], rc);

	pt.x = rect.base.x;
	pt.y = rect.base.y + rect.height - 1;
	int ybot = pt.y;
	if (spr_base[WBS_BOTTOM_LEFT] != nullptr) {
		_video.BlitImage(pt, spr_base[WBS_BOTTOM_LEFT], rc, GS_NORMAL);
		ybot += spr_base[WBS_BOTTOM_LEFT]->yoffset;
	}

	uint16 numy = ybot - ytop;
	if (spr_base[WBS_MIDDLE_LEFT] != nullptr) {
		numy /= spr_base[WBS_MIDDLE_LEFT]->height;
	} else if (spr_base[WBS_MIDDLE_RIGHT] != nullptr) {
		numy /= spr_base[WBS_MIDDLE_RIGHT]->height;
	} // else assume sprite height is 1.
	if (spr_base[WBS_MIDDLE_LEFT] != nullptr) _video.BlitVertical(ytop, numy, pt.x, spr_base[WBS_MIDDLE_LEFT], rc);

	pt.x = rect.base.x + rect.width - 1;
	pt.y = rect.base.y + rect.height - 1;
	if (spr_base[WBS_BOTTOM_RIGHT] != nullptr) _video.BlitImage(pt, spr_base[WBS_BOTTOM_RIGHT], rc, GS_NORMAL);

	if (spr_base[WBS_BOTTOM_MIDDLE] != nullptr) _video.BlitHorizontal(xleft, numx, pt.y, spr_base[WBS_BOTTOM_MIDDLE], rc);

	pt.x = rect.base.x + rect.width - 1;
	if (spr_base[WBS_MIDDLE_RIGHT] != nullptr) _video.BlitVertical(ytop, numy, pt.x, spr_base[WBS_MIDDLE_RIGHT], rc);

	if (spr_base[WBS_MIDDLE_MIDDLE] != nullptr) DrawRepeatedSprites(xleft, ytop, spr_base[WBS_MIDDLE_MIDDLE], numx, numy, rc);
}

/**
 * Draw overlay sprite to mark a button as being shaded.
 * @param rect Area at the screen containing the button.
 * @todo Alignment can be done much faster if we may assume a sprite size. Perhaps add some special cases for common sizes?
 */
void OverlayShaded(const Rectangle32 &rect)
{
	const ImageData *img = _gui_sprites.disabled;
	if (img == nullptr) return;

	Rectangle32 r(rect);
	r.RestrictTo(0, 0, _video.GetXSize(), _video.GetYSize());
	if (r.width == 0 || r.height == 0) return;

	/* Set clipped area to the rectangle. */
	ClippedRectangle cr(_video.GetClippedRectangle());
	ClippedRectangle new_cr(cr, r.base.x, r.base.y, r.width, r.height);
	_video.SetClippedRectangle(new_cr);

	/* Align the disabled sprite so it becomes a continuous pattern. */
	int32 base_x = -(r.base.x % img->width);
	int32 base_y = -(r.base.y % img->height);
	uint16 numx = (r.width + img->width - 1) / img->width;
	uint16 numy = (r.height + img->height - 1) / img->height;

	static const Recolouring recolour; // Fixed recolouring mapping.
	_video.BlitImages(base_x, base_y, img, numx, numy, recolour);

	_video.SetClippedRectangle(cr); // Restore clipped area.
}

/**
 * Draw a string to the screen.
 * @param strid String to draw.
 * @param colour Colour of the text.
 * @param x X position at the screen.
 * @param y Y position at the screen.
 * @param width Maximal width of the text.
 * @param align Horizontal alignment of the string.
 * @param outline Whether to make the string "bold" (default false).
 */
void DrawString(StringID strid, uint8 colour, int x, int y, int width, Alignment align, bool outline)
{
	uint8 buffer[1024]; // Arbitrary limit.

	DrawText(strid, buffer, lengthof(buffer));
	/** \todo Reduce the naiviness of this. */
	if (outline) {
		_video.BlitText(buffer, MakeRGBA(0, 0, 0, OPAQUE), x + 1, y, width, align);
		_video.BlitText(buffer, MakeRGBA(0, 0, 0, OPAQUE), x, y + 1, width, align);
		_video.BlitText(buffer, MakeRGBA(0, 0, 0, OPAQUE), x - 1, y, width, align);
		_video.BlitText(buffer, MakeRGBA(0, 0, 0, OPAQUE), x, y - 1, width, align);
	}
	_video.BlitText(buffer, _palette[colour], x, y, width, align);
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
	uint8 *best_pos = nullptr;
	for (;;) {
		uint8 *current = text;
		/* Proceed to the first white-space. */
		while (*current != '\n' && *current != '\0' && *current != ' ') current++;
		uint8 orig = *current;
		*current = '\0';
		int line_width, line_height;
		_video.GetTextSize(start, &line_width, &line_height);

		if (line_width < max_width) {
			*width = line_width;
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
		if (best_pos != nullptr) {
			/* There is a best position to fall back to. */
			*current = orig;  // We will revisit this position later.
			*best_pos = '\n'; // Insert line break.
			return best_pos;
		}

		/* line_width >= max_width, no fallback to a previous position. */
		*width = line_width;
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
		*width = std::max(*width, line_width);
		*height += _video.GetTextHeight();

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
		if (max_height < _video.GetTextHeight()) return false;
		max_height -= _video.GetTextHeight();

		int line_width;
		uint8 *end = GetSingleLine(text, max_width, &line_width);
		if (*end == '\0') {
			_video.BlitText(text, _palette[colour], x, y, max_width);
			break;
		}
		*end = '\0';
		_video.BlitText(text, _palette[colour], x, y, max_width);
		y += _video.GetTextHeight();
		text = end + 1;
	}
	return true;
}

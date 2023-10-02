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
	static Recolouring rc;  // Only COL_RANGE_BROWN is modified each time.
	rc.Set(0, RecolourEntry(COL_RANGE_BROWN, colour));

	int x1 = rect.base.x;
	int x2 = rect.base.x + rect.width;
	int y1 = rect.base.y;
	int y2 = rect.base.y + rect.height;

	if (spr_base[WBS_MIDDLE_MIDDLE] != nullptr) {
		_video.TileImage(spr_base[WBS_MIDDLE_MIDDLE], Rectangle32(x1, y1, x2 - x1, y2 - y1), true, true, rc);
	}

	if (spr_base[WBS_MIDDLE_LEFT] != nullptr) {
		_video.TileImage(spr_base[WBS_MIDDLE_LEFT], Rectangle32(
				x1 + spr_base[WBS_MIDDLE_LEFT]->xoffset,
				y1 + spr_base[WBS_MIDDLE_LEFT]->yoffset,
				spr_base[WBS_MIDDLE_LEFT]->width,
				y2 - y1), false, true, rc);
	}
	if (spr_base[WBS_MIDDLE_RIGHT] != nullptr) {
		_video.TileImage(spr_base[WBS_MIDDLE_RIGHT], Rectangle32(
				x2 + spr_base[WBS_MIDDLE_RIGHT]->xoffset,
				y1 + spr_base[WBS_MIDDLE_RIGHT]->yoffset,
				spr_base[WBS_MIDDLE_RIGHT]->width,
				y2 - y1), false, true, rc);
	}
	if (spr_base[WBS_TOP_MIDDLE] != nullptr) {
		_video.TileImage(spr_base[WBS_TOP_MIDDLE], Rectangle32(
				x1 + spr_base[WBS_TOP_MIDDLE]->xoffset,
				y1 + spr_base[WBS_TOP_MIDDLE]->yoffset,
				x2 - x1,
				spr_base[WBS_TOP_MIDDLE]->height), true, false, rc);
	}
	if (spr_base[WBS_BOTTOM_MIDDLE] != nullptr) {
		_video.TileImage(spr_base[WBS_BOTTOM_MIDDLE], Rectangle32(
				x1 + spr_base[WBS_BOTTOM_MIDDLE]->xoffset,
				y2 + spr_base[WBS_BOTTOM_MIDDLE]->yoffset,
				x2 - x1,
				spr_base[WBS_BOTTOM_MIDDLE]->height), true, false, rc);
	}

	if (spr_base[WBS_TOP_LEFT] != nullptr) {
		_video.BlitImage(Point32(x1, y1), spr_base[WBS_TOP_LEFT], rc);
	}
	if (spr_base[WBS_TOP_RIGHT] != nullptr) {
		_video.BlitImage(Point32(x2, y1), spr_base[WBS_TOP_RIGHT], rc);
	}
	if (spr_base[WBS_BOTTOM_LEFT] != nullptr) {
		_video.BlitImage(Point32(x1, y2), spr_base[WBS_BOTTOM_LEFT], rc);
	}
	if (spr_base[WBS_BOTTOM_RIGHT] != nullptr) {
		_video.BlitImage(Point32(x2, y2), spr_base[WBS_BOTTOM_RIGHT], rc);
	}
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
	_video.TileImage(img, rect, true, true);
}

/**
 * Draw a string to the screen.
 * @param buffer String to draw.
 * @param colour Colour of the text.
 * @param x X position at the screen.
 * @param y Y position at the screen.
 * @param width Maximal width of the text.
 * @param align Horizontal alignment of the string.
 * @param outline Whether to make the string "bold" (default false).
 */
void DrawString(const std::string &buffer, uint8 colour, int x, int y, int width, Alignment align, bool outline)
{
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
	DrawString(DrawText(strid), colour, x, y, width, align, outline);
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
static char *GetSingleLine(char *text, int max_width, int *width)
{
	char *start = text;
	char *best_pos = nullptr;
	for (;;) {
		char *current = text;
		/* Proceed to the first white-space. */
		while (*current != '\n' && *current != '\0' && *current != ' ') current++;
		char orig = *current;
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
	/* \todo This design is ugly. Fix the utility function GetSingleLine() to work on std::string with a start index instead of mutable char*. */
	const std::string buffer = DrawText(strid);
	std::unique_ptr<char[]> mutable_buffer(new char[buffer.size() + 1]);
	strcpy(mutable_buffer.get(), buffer.c_str());
	char *text = mutable_buffer.get();

	*width = 0;
	*height = 0;
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
	const std::string buffer = DrawText(strid);
	std::unique_ptr<char[]> mutable_buffer(new char[buffer.size() + 1]);
	strcpy(mutable_buffer.get(), buffer.c_str());
	char *text = mutable_buffer.get();

	while (*text != '\0') {
		if (max_height < _video.GetTextHeight()) return false;
		max_height -= _video.GetTextHeight();

		int line_width;
		char *end = GetSingleLine(text, max_width, &line_width);
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

/**
 * Calculate the render offset of a sprite so that it is centered in the given rectangle.
 * @param rect Rectangle to center the sprite in.
 * @param img Sprite to center.
 * @return The offset to use.
 */
Point32 CenterSprite(const Rectangle32 &rect, const ImageData *img)
{
	return Point32(
			rect.base.x + (static_cast<int32>(rect.width ) - static_cast<int32>(img->width )) / 2,
			rect.base.y + (static_cast<int32>(rect.height) - static_cast<int32>(img->height)) / 2);
}

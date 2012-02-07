/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file palette.h 8bpp palette definitions. */

#ifndef PALETTE_H
#define PALETTE_H

/** Names of colour ranges. */
enum ColourRange {
	COL_RANGE_GREY,
	COL_RANGE_GREEN_BROWN,
	COL_RANGE_BROWN,
	COL_RANGE_YELLOW,
	COL_RANGE_DARK_RED,
	COL_RANGE_DARK_GREEN,
	COL_RANGE_LIGHT_GREEN,
	COL_RANGE_GREEN,
	COL_RANGE_LIGHT_RED,
	COL_RANGE_DARK_BLUE,
	COL_RANGE_BLUE,
	COL_RANGE_LIGHT_BLUE,
	COL_RANGE_PURPLE,
	COL_RANGE_RED,
	COL_RANGE_ORANGE,
	COL_RANGE_SEA_GREEN,
	COL_RANGE_PINK,
	COL_RANGE_BEIGE,

	COL_RANGE_COUNT, ///< Number of colour ranges.
};

/** Colours. */
enum PaletteColours {
	COL_BACKGROUND = 0,     ///< Index of background behind the world display.
	COL_HIGHLIGHT  = 1,     ///< Index of window highlighting edge colour when (re)opening.

	COL_SERIES_START = 10,  ///< Index of the first series.
	COL_SERIES_LENGTH = 12, ///< Number of shades in a single run.
	COL_SERIES_END = COL_SERIES_START + COL_RANGE_COUNT * COL_SERIES_LENGTH, ///< First colour after the series.
};

/**
 * Get the index of the base colour of a colour range.
 * @param cr Colour range to use.
 * @return Palette index of the first colour of the given colour range.
 */
inline uint8 GetColourRangeBase(ColourRange cr)
{
	return COL_SERIES_START + cr * COL_SERIES_LENGTH;
}

extern const uint8 _palette[256][3]; ///< The 8bpp FreeRCT palette.

#endif

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

#include "language.h"

class Random;

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
	COL_RANGE_INVALID = COL_RANGE_COUNT, ///< Number denoting an invalid colour range.
};

/** Gui text colours. */
enum GuiTextColours {
	TEXT_BLACK = 0, ///< Black text colour.
	TEXT_WHITE = 1, ///< White text colour.
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

/**
 * Sprite recolouring information.
 * All information of a sprite recolouring. The gradient colour shift is handled separately, as it changes often.
 */
class Recolouring {
public:
	Recolouring();
	Recolouring(const Recolouring &sr);
	Recolouring &operator=(const Recolouring &sr);
	void SetRecolouring(ColourRange orig, ColourRange dest);

	/**
	 * Compute the colour of a pixel with recolouring.
	 * @param orig Original colour.
	 * @param shift Gradient shift in the target range.
	 * @return Colour value after recolouring.
	 */
	inline int Recolour(int orig, int shift) const
	{
		if (orig < COL_SERIES_START || orig >= COL_SERIES_END) return orig;
		int i = (orig - COL_SERIES_START) / COL_SERIES_LENGTH;
		int j = this->range_map[i];
		orig += (j - i) * COL_SERIES_LENGTH + shift;
		int base = GetColourRangeBase((ColourRange)j);
		if (orig < base) return base;
		if (orig > base + COL_SERIES_LENGTH - 1) return base + COL_SERIES_LENGTH - 1;
		return orig;
	}

	uint8 range_map[COL_RANGE_COUNT];   ///< Mapping of each colour range to another one.
};

/**
 * Editable recolouring.
 * Names and tooltips allow explanation of the colours that can be changed.
 */
class EditableRecolouring : public Recolouring {
public:
	EditableRecolouring();
	EditableRecolouring(const EditableRecolouring &sr);
	EditableRecolouring &operator=(const EditableRecolouring &sr);
	void SetRecolouring(ColourRange orig, ColourRange dest, StringID name = STR_NULL, StringID tooltip = STR_NULL);

	StringID name_map[COL_RANGE_COUNT]; ///< Names of the colours of the sprite.
	StringID tip_map[COL_RANGE_COUNT];  ///< Tooltips of the colours.
};

/** Definition of a random recolouring remapping. */
struct RandomRecolouringMapping {
	RandomRecolouringMapping();

	/**
	 * Set the recolour mapping, from an RCD file.
	 * @param value Value as defined in the RCD format (lower 18 bits the set, upper 8 bits the source colour range).
	 */
	void Set(uint32 value)
	{
		this->Set(value >> 24, value);
	}

	/**
	 * Set the recolour mapping.
	 * @param number Source colour range to remap.
	 * @param dest_set Bit-set of allowed destination colour ranges.
	 */
	void Set(uint8 number, uint32 dest_set)
	{
		if (number >= COL_RANGE_COUNT) number = COL_RANGE_INVALID;
		this->range_number = number;
		this->dest_set = dest_set;
	}

	uint8 range_number; ///< Colour range number being mapped.
	uint32 dest_set;    ///< Bit-set of allowed colour ranges to replace #range_number.

	ColourRange DrawRandomColour(Random *rnd) const;
};

extern const uint8 _palette[256][3]; ///< The 8bpp FreeRCT palette.

#endif

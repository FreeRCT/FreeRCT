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

static const uint8 TRANSPARENT = 0; ///< Opacity value of a fully transparent pixel.
static const uint8 OPAQUE = 255;    ///< Opacity value of a fully opaque pixel.

/**
 * Construct a 32bpp pixel value from its components.
 * @param r Intensity of the red colour component.
 * @param g Intensity of the green colour component.
 * @param b Intensity of the blue colour component.
 * @param a Opacity of the pixel.
 * @return Pixel value of the given combination of colour components.
 */
static inline uint32 MakeRGBA(uint8 r, uint8 g, uint8 b, uint8 a)
{
	return (((uint32)r) << 24) | (((uint32)g) << 16) | (((uint32)b) << 8) | a;
}

/**
 * Retrieve the opaqueness of the provided pixel value.
 * @param rgba Pixel value to examine.
 * @return Opaqueness of the examined pixel.
 */
static inline uint8 GetA(uint32 rgba)
{
	return rgba;
}

/** Names of colour ranges. */
enum ColourRange {
	COL_RANGE_GREY,
	COL_RANGE_GREEN_BROWN,
	COL_RANGE_ORANGE_BROWN,
	COL_RANGE_YELLOW,
	COL_RANGE_DARK_RED,
	COL_RANGE_DARK_GREEN,
	COL_RANGE_LIGHT_GREEN,
	COL_RANGE_GREEN,
	COL_RANGE_PINK_BROWN,
	COL_RANGE_DARK_PURPLE,
	COL_RANGE_BLUE,
	COL_RANGE_DARK_JADE_GREEN,
	COL_RANGE_PURPLE,
	COL_RANGE_RED,
	COL_RANGE_ORANGE,
	COL_RANGE_SEA_GREEN,
	COL_RANGE_PINK,
	COL_RANGE_BROWN,

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

/** Shifting of the gradient to make the sprite lighter or darker. */
enum GradientShift {
	GS_NIGHT,          ///< Shift gradient four steps darker.
	GS_VERY_DARK,      ///< Shift gradient three steps darker.
	GS_DARK,           ///< Shift gradient two steps darker.
	GS_SLIGHTLY_DARK,  ///< Shift gradient one step darker.
	GS_NORMAL,         ///< No change in gradient.
	GS_SLIGHTLY_LIGHT, ///< Shift gradient one step lighter.
	GS_LIGHT,          ///< Shift gradient two steps lighter.
	GS_VERY_LIGHT,     ///< Shift gradient three steps lighter.
	GS_DAY,            ///< Shift gradient four steps lighter.

	GS_COUNT,          ///< Number of gradient shifts.
	GS_INVALID = 0xff, ///< Invalid gradient shift.
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
	void SetRecolouring(uint8 base_series, uint8 dest_series);

	/**
	 * Compute the colour of a pixel with recolouring.
	 * @param orig Original colour.
	 * @param shift Gradient shift in the target range.
	 * @return Colour value after recolouring.
	 */
	inline int Recolour8bpp(int orig, int shift) const
	{
		return this->recolour_map[shift][orig];
	}

	/**
	 * Compute 32bpp recoloured pixels.
	 * @param intensity Intensity of the original pixel.
	 * @param shift Amount of shifting that should be performed.
	 * @param layer Recolour layer (denotes which part is being recoloured).
	 * @param opacity Opacity of the pixel.
	 * @return Recoloured pixel value.
	 * @todo Do actual recolouring.
	 */
	inline uint32 Recolour32bpp(uint8 intensity, int shift, uint8 layer, uint8 opacity) const
	{
		return MakeRGBA(intensity, intensity, intensity, opacity);
	}

	uint8 recolour_map[5][256]; ///< Mapping of each colour to its shifted, recoloured palette value.

private:
	void SetRecolourFixedParts();
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
		this->Set(static_cast<ColourRange>(value >> 24), value);
	}

	/**
	 * Set the recolour mapping.
	 * @param number Source colour range to remap.
	 * @param dest_set Bit-set of allowed destination colour ranges.
	 */
	void Set(ColourRange number, uint32 dest_set)
	{
		if (number >= COL_RANGE_COUNT) number = COL_RANGE_INVALID;
		this->range_number = number;
		this->dest_set = dest_set;
	}

	ColourRange range_number; ///< Colour range number being mapped.
	uint32 dest_set;          ///< Bit-set of allowed colour ranges to replace #range_number.

	ColourRange DrawRandomColour(Random *rnd) const;
};

extern const uint32 _palette[256]; ///< The old FreeRCT palette.

#endif

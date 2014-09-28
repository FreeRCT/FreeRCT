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

class Random;

extern const uint32 _palette[256];  ///< The 8bpp FreeRCT palette.
extern const uint32 * const _recolour_palettes[18]; ///< 32bpp recolour tables.

static const uint8 TRANSPARENT = 0; ///< Opacity value of a fully transparent pixel.
static const uint8 OPAQUE = 255;    ///< Opacity value of a fully opaque pixel.

static const int MAX_RECOLOUR = 4;  ///< Maximum number of recolourings that can be defined in a #Recolouring class.

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
 * Retrieve the redness of the provided pixel value.
 * @param rgba Pixel value to examine.
 * @return Redness of the examined pixel.
 */
static inline uint8 GetR(uint32 rgba)
{
	return rgba >> 24;
}

/**
 * Retrieve the greenness of the provided pixel value.
 * @param rgba Pixel value to examine.
 * @return Greenness of the examined pixel.
 */
static inline uint8 GetG(uint32 rgba)
{
	return rgba >> 16;
}

/**
 * Retrieve the blueness of the provided pixel value.
 * @param rgba Pixel value to examine.
 * @return Blueness of the examined pixel.
 */
static inline uint8 GetB(uint32 rgba)
{
	return rgba >> 8;
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

/**
 * Set the opaqueness of the provided colour.
 * @param rgba Colour value to change.
 * @param opacity Opaqueness to set.
 * @return The combined pixel value.
 */
static inline uint32 SetA(uint32 rgba, uint8 opacity)
{
	return rgba | opacity;
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
	COL_RANGE_INVALID = 0xff, ///< Number denoting an invalid colour range.
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

static const int STEP_SIZE = 18; ///< Amount of colour shift for each gradient step.
typedef uint8 (*ShiftFunc)(uint8); ///< Type of the gradient shift function.

/**
 * Gradient shift function for #GS_NIGHT.
 * @param col Input colour.
 * @return Shifted result colour.
 */
static inline uint8 ShiftGradientNight(uint8 col)
{
	return (col <= 4 * STEP_SIZE) ? 0 : col - 4 * STEP_SIZE;
}

/**
 * Gradient shift function for #GS_VERY_DARK.
 * @param col Input colour.
 * @return Shifted result colour.
 */
static inline uint8 ShiftGradientVeryDark(uint8 col)
{
	return (col <= 3 * STEP_SIZE) ? 0 : col - 3 * STEP_SIZE;
}

/**
 * Gradient shift function for #GS_DARK
 * @param col Input colour.
 * @return Shifted result colour.
 */
static inline uint8 ShiftGradientDark(uint8 col)
{
	return (col <= 2 * STEP_SIZE) ? 0 : col - 2 * STEP_SIZE;
}

/**
 * Gradient shift function for #GS_SLIGHTLY_DARK.
 * @param col Input colour.
 * @return Shifted result colour.
 */
static inline uint8 ShiftGradientSlightlyDark(uint8 col)
{
	return (col <= STEP_SIZE) ? 0 : col - STEP_SIZE;
}

/**
 * Gradient shift function for #GS_NORMAL.
 * @param col Input colour.
 * @return Shifted result colour.
 */
static inline uint8 ShiftGradientNormal(uint8 col)
{
	return col;
}

/**
 * Gradient shift function for #GS_SLIGHTLY_LIGHT.
 * @param col Input colour.
 * @return Shifted result colour.
 */
static inline uint8 ShiftGradientSlightlyLight(uint8 col)
{
	return (col >= 255 - STEP_SIZE) ? 255: col + STEP_SIZE;
}

/**
 * Gradient shift function for #GS_LIGHT.
 * @param col Input colour.
 * @return Shifted result colour.
 */
static inline uint8 ShiftGradientLight(uint8 col)
{
	return (col >= 255 - 2 * STEP_SIZE) ? 255: col + 2 * STEP_SIZE;
}

/**
 * Gradient shift function for #GS_VERY_LIGHT.
 * @param col Input colour.
 * @return Shifted result colour.
 */
static inline uint8 ShiftGradientVeryLight(uint8 col)
{
	return (col >= 255 - 3 * STEP_SIZE) ? 255: col + 3 * STEP_SIZE;
}

/**
 * Gradient shift function for #GS_DAY.
 * @param col Input colour.
 * @return Shifted result colour.
 */
static inline uint8 ShiftGradientDay(uint8 col)
{
	return (col >= 255 - 4 * STEP_SIZE) ? 255: col + 4 * STEP_SIZE;
}

/**
 * Select gradient shift function based on the \a shift.
 * @param shift Desired amount of gradient shift.
 * @return Recolour function implementing the shift.
 */
static inline ShiftFunc GetGradientShiftFunc(GradientShift shift)
{
	switch (shift) {
		case GS_NIGHT:          return ShiftGradientNight;
		case GS_VERY_DARK:      return ShiftGradientVeryDark;
		case GS_DARK:           return ShiftGradientDark;
		case GS_SLIGHTLY_DARK:  return ShiftGradientSlightlyDark;
		case GS_NORMAL:         return ShiftGradientNormal;
		case GS_SLIGHTLY_LIGHT: return ShiftGradientSlightlyLight;
		case GS_LIGHT:          return ShiftGradientLight;
		case GS_VERY_LIGHT:     return ShiftGradientVeryLight;
		case GS_DAY:            return ShiftGradientDay;
		default: NOT_REACHED();
	}
}

/**
 * Get the index of the base colour of a colour range.
 * @param cr Colour range to use.
 * @return Palette index of the first colour of the given colour range.
 */
inline uint8 GetColourRangeBase(ColourRange cr)
{
	return COL_SERIES_START + cr * COL_SERIES_LENGTH;
}

/** A recolour entry, defining a #source colour range to replace with the #dest colour range, where #dest must be in #dest_set. */
struct RecolourEntry {
	RecolourEntry();
	RecolourEntry(uint32 recol);
	RecolourEntry(ColourRange source, ColourRange dest);
	RecolourEntry(ColourRange source, uint32 dest_set, ColourRange dest);
	RecolourEntry(const RecolourEntry &orig);
	RecolourEntry &operator=(const RecolourEntry &orig);

	void AssignDest(ColourRange dest);

	/**
	 * Test whether the entry is filled and ready for use.
	 * @return Whether the entry is ready for use.
	 */
	bool IsValid() const
	{
		return this->source < COL_RANGE_COUNT && this->dest_set != 0;
	}

	ColourRange source; ///< Source colour range to replace. Only used for 8bpp images, the value #COL_RANGE_INVALID means the entry is not used.
	ColourRange dest;   ///< Destination colour range to use instead. The value #COL_RANGE_INVALID means no choice has been made yet.
	uint32 dest_set;    ///< Bit set of destination colour ranges to chose from.
};

/**
 * Sprite recolouring information.
 * All information of a sprite recolouring. The gradient colour shift is handled separately, as it changes often.
 */
class Recolouring {
public:
	Recolouring();
	Recolouring(const Recolouring &sr);
	Recolouring &operator=(const Recolouring &sr);

	void Reset();
	void Set(int index, const RecolourEntry &entry);
	void AssignRandomColours();

	const uint8 *GetPalette(GradientShift shift) const;

	/**
	 * Get the table with recolouring of a layer.
	 * @param layer Layer to recolour.
	 * @return Table to use for recolouring.
	 * @todo Implement recolour layer check in the 32bpp sprite loading, so it can be asserted here.
	 */
	const uint32 *GetRecolourTable(uint8 layer) const
	{
		if (layer >= MAX_RECOLOUR) return _recolour_palettes[0];
		const RecolourEntry &re = this->entries[layer];
		if (re.dest >= COL_RANGE_COUNT) return _recolour_palettes[0];
		return _recolour_palettes[re.dest];
	}

	/**
	 * Recolour entries, (one for each layer in 32bpp).
	 * Don't assign directly, use #Set instead.
	 */
	RecolourEntry entries[MAX_RECOLOUR];

private:
	/** Invalidate the colour map used last time. */
	void InvalidateColourMap()
	{
		this->shift = GS_INVALID;
	}

	ColourRange GetReplacementRange(ColourRange src) const;

	mutable uint8 colour_map[256]; ///< Colour map used last time.
	mutable GradientShift shift;   ///< Gradient shift used last time, #GS_INVALID if the #colour_map is not valid.
};

#endif

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file palette.cpp 8bpp palette definitions. */

#include "stdafx.h"
#include "bitmath.h"
#include "math_func.h"
#include "palette.h"
#include "random.h"

const Recolouring _no_recolour;  ///< Apply no recolouring.

/** Default constructor. */
RecolourEntry::RecolourEntry() : source(COL_RANGE_INVALID), dest(COL_RANGE_INVALID), dest_set(0)
{
}

/**
 * Constructor for an encoded recolour entry.
 * @param value Encoded recolouring.
 */
RecolourEntry::RecolourEntry(uint32 value)
{
	uint8 src = value >> 24;
	this->source = (src >= COL_RANGE_COUNT) ? COL_RANGE_INVALID : (ColourRange)src;
	this->dest = COL_RANGE_INVALID;
	this->dest_set = value & 0x3FFFF;
}

/**
 * Recolour entry mapping a \a source range directly to a \a dest range.
 * @param source Source colour range to convert.
 * @param dest Replacement colour range.
 */
RecolourEntry::RecolourEntry(ColourRange source, ColourRange dest)
{
	assert(source < COL_RANGE_COUNT);
	assert(dest < COL_RANGE_COUNT);

	this->source = source;
	this->dest = dest;
	this->dest_set = 1 << dest;
}

/**
 * Constructor with all data members.
 * @param source Source range of the entry.
 * @param dest_set Bit set of allowed destination ranges (bits 0 to 17 inclusive).
 * @param dest Destination range (if provided, else #COL_RANGE_INVALID).
 */
RecolourEntry::RecolourEntry(ColourRange source, uint32 dest_set, ColourRange dest) : source(source), dest(dest), dest_set(dest_set)
{
}

/**
 * Copy constructor.
 * @param orig Existing value to copy.
 */
RecolourEntry::RecolourEntry(const RecolourEntry &orig)
{
	this->source = orig.source;
	this->dest = orig.dest;
	this->dest_set = orig.dest_set;
}

/**
 * Assignment operator.
 * @param orig Existing value to copy.
 * @return The assigned value.
 */
RecolourEntry &RecolourEntry::operator=(const RecolourEntry &orig)
{
	if (this != &orig) {
		this->source = orig.source;
		this->dest = orig.dest;
		this->dest_set = orig.dest_set;
	}
	return *this;
}

/**
 * Assign a destination colour range to the entry.
 * @param dest Destination colour range to use.
 */
void RecolourEntry::AssignDest(ColourRange dest)
{
	if (dest == COL_RANGE_INVALID || (this->source != COL_RANGE_INVALID && dest < COL_RANGE_COUNT && GB(this->dest_set, dest, 1) != 0)) {
		this->dest = dest;
	}
}

/** Default constructor. */
Recolouring::Recolouring()
{
	this->Reset();
	this->InvalidateColourMap();
}

/**
 * Copy constructor.
 * @param rc Recolouring to use as template.
 */
Recolouring::Recolouring(const Recolouring &rc)
{
	std::copy(&rc.entries[0], endof(rc.entries), this->entries);
}

/**
 * Assignment operator.
 * @param rc Recolouring to use as template.
 * @return The assigned value.
 */
Recolouring &Recolouring::operator=(const Recolouring &rc)
{
	if (this != &rc) {
		std::copy(&rc.entries[0], endof(rc.entries), this->entries);
	}
	return *this;
}

/** Reset the recolouring, removing all recolour entries. */
void Recolouring::Reset()
{
	for (int i = 0; i < MAX_RECOLOUR; i++) entries[i].source = COL_RANGE_INVALID;
}

/**
 * Copy a recolour entry into the recolouring.
 * @param index Entry number to set.
 * @param entry Entry to copy.
 */
void Recolouring::Set(int index, const RecolourEntry &entry)
{
	if (index >= MAX_RECOLOUR) return;
	this->entries[index] = entry;
	this->InvalidateColourMap();
}

/** Select random destination colour ranges for the recolour entries. */
void Recolouring::AssignRandomColours()
{
	Random rnd;

	for (uint i = 0; i < lengthof(this->entries); i++) {
		RecolourEntry &re = this->entries[i];
		if (re.source != COL_RANGE_INVALID && re.dest == COL_RANGE_INVALID) {
			if (re.dest_set == 0) {
				re.source = COL_RANGE_INVALID;
				continue;
			}
			int num_bits = CountBits(re.dest_set);
			num_bits = (num_bits == 1) ? 0 : rnd.Uniform(num_bits - 1);
			for (int j = 0; j < 32; j++) {
				if (GB(re.dest_set, j, 1) != 0) {
					num_bits--;
					if (num_bits < 0) {
						re.AssignDest((ColourRange)j);
						break;
					}
				}
			}
		}
	}
}

static const uint32 CURRENT_VERSION_Recolouring = 1;   ///< Currently supported version of %Recolouring.

/**
 * Load recolour information.
 * ldr Input stream to load from.
 */
void Recolouring::Load(Loader &ldr)
{
	const uint32 version = ldr.OpenPattern("rcol");
	if (version != CURRENT_VERSION_Recolouring) ldr.VersionMismatch(version, CURRENT_VERSION_Recolouring);

	assert(MAX_RECOLOUR == 4); // Check that we are compatible with other saves.
	for (int i = 0; i < MAX_RECOLOUR; i++) {
		uint8 val = ldr.GetByte();
		this->entries[i].AssignDest((ColourRange)val);
	}
	ldr.ClosePattern();
}

/**
 * Save the recolouring information.
 * @param svr Output stream to write to.
 */
void Recolouring::Save(Saver &svr)
{
	svr.StartPattern("rcol", CURRENT_VERSION_Recolouring);
	assert(MAX_RECOLOUR == 4); // Check that we are compatible with other saves.
	for (int i = 0; i < MAX_RECOLOUR; i++) {
		const RecolourEntry &entry = this->entries[i];
		uint8 val = (entry.source == COL_RANGE_INVALID) ? COL_RANGE_INVALID : entry.dest;
		svr.PutByte(val);
	}
	svr.EndPattern();
}

/**
 * Compute the palette of the #Recolouring object from the #entries and the gradient shift.
 * @param shift Applied gradient shift.
 * @return 8bpp palette, including recolouring.
 */
const uint8 *Recolouring::GetPalette(GradientShift shift) const
{
	if (this->shift == shift) return this->colour_map;

	for (int i = 0; i < COL_SERIES_START; i++) this->colour_map[i] = i;
	if (shift == GS_SEMI_TRANSPARENT) {
		for (int i = COL_SERIES_START; i < COL_SERIES_END; i++) this->colour_map[i] = COL_SEMI_TRANSPARENT;
	} else {
		for (int rng = 0; rng < COL_RANGE_COUNT; rng++) {
			int base = GetColourRangeBase((ColourRange)rng);
			int baseval = GetColourRangeBase(this->GetReplacementRange((ColourRange)rng));
			for (int col = 0; col < COL_SERIES_LENGTH; col++) {
				this->colour_map[base + col] = baseval + Clamp(col + shift - GS_NORMAL, 0, COL_SERIES_LENGTH - 1);
			}
		}
	}
	for (int i = COL_SERIES_END; i < 256; i++) this->colour_map[i] = i;

	this->shift = shift;
	return this->colour_map;
}

/**
 * Find the coloure range to use as replacement for the \a src colour range.
 * @param src Colour range to replace.
 * @return Colour range to use instead of \a src (may be \a src again).
 */
ColourRange Recolouring::GetReplacementRange(ColourRange src) const
{
	ColourRange dest = src;
	for (int i = 0; i < MAX_RECOLOUR; i++) {
		if (this->entries[i].source == src && this->entries[i].dest != COL_RANGE_INVALID) dest = this->entries[i].dest;
	}
	return dest;
}

/**
 * Generate this recoloring's current condensed version.
 * @return The condenses information.
 */
CondensedRecolouring Recolouring::ToCondensed() const
{
	CondensedRecolouring r;
	for (const RecolourEntry &e : this->entries) r.emplace_back(e.source, e.dest);
	return r;
}

/** 8 bpp colours mapped to 32 bpp. */
const uint32 _palette[256] = {
 	MakeRGBA(  0,   0,   0, TRANSPARENT), //  0 COL_BACKGROUND (background behind world display)
	MakeRGBA(255, 255, 255, OPAQUE), //  1 COL_HIGHLIGHT (full white to highlight window edge)
	MakeRGBA(255, 255, 255, OPACITY_SEMI_TRANSPARENT), //  2 COL_SEMI_TRANSPARENT (semi-transparent white buy-ride colour)
	MakeRGBA(220, 200, 100, OPAQUE), //  3 Background for tooltips.
	MakeRGBA( 20,  20,  50, OPAQUE), //  4 Text colour for tooltips.
	MakeRGBA(  0,   0,   0, OPAQUE), //  5 Border colour for tooltips.
	MakeRGBA(130, 130, 130, OPAQUE), //  6 Grey text colour.
	MakeRGBA(  0,   0,   0,    127), //  7 Darkening overlay
	MakeRGBA(  0,   0,   0, OPAQUE), //  8 Black text.
	MakeRGBA(  0,   0,   0, OPAQUE), //  9 unused
	MakeRGBA( 23,  35,  35, OPAQUE), //  10 Start of COL_RANGE_GREY.
	MakeRGBA( 35,  51,  51, OPAQUE), //  11
	MakeRGBA( 47,  67,  67, OPAQUE), //  12
	MakeRGBA( 63,  83,  83, OPAQUE), //  13
	MakeRGBA( 75,  99,  99, OPAQUE), //  14
	MakeRGBA( 91, 115, 115, OPAQUE), //  15
	MakeRGBA(111, 131, 131, OPAQUE), //  16
	MakeRGBA(131, 151, 151, OPAQUE), //  17
	MakeRGBA(159, 175, 175, OPAQUE), //  18
	MakeRGBA(183, 195, 195, OPAQUE), //  19
	MakeRGBA(211, 219, 219, OPAQUE), //  20
	MakeRGBA(239, 243, 243, OPAQUE), //  21
	MakeRGBA( 51,  47,   0, OPAQUE), //  22 Start of COL_RANGE_GREEN_BROWN.
	MakeRGBA( 63,  59,   0, OPAQUE), //  23
	MakeRGBA( 79,  75,  11, OPAQUE), //  24
	MakeRGBA( 91,  91,  19, OPAQUE), //  25
	MakeRGBA(107, 107,  31, OPAQUE), //  26
	MakeRGBA(119, 123,  47, OPAQUE), //  27
	MakeRGBA(135, 139,  59, OPAQUE), //  28
	MakeRGBA(151, 155,  79, OPAQUE), //  29
	MakeRGBA(167, 175,  95, OPAQUE), //  30
	MakeRGBA(187, 191, 115, OPAQUE), //  31
	MakeRGBA(203, 207, 139, OPAQUE), //  32
	MakeRGBA(223, 227, 163, OPAQUE), //  33
	MakeRGBA( 67,  43,   7, OPAQUE), //  34 Start of COL_RANGE_ORANGE_BROWN
	MakeRGBA( 87,  59,  11, OPAQUE), //  35
	MakeRGBA(111,  75,  23, OPAQUE), //  36
	MakeRGBA(127,  87,  31, OPAQUE), //  37
	MakeRGBA(143,  99,  39, OPAQUE), //  38
	MakeRGBA(159, 115,  51, OPAQUE), //  39
	MakeRGBA(179, 131,  67, OPAQUE), //  40
	MakeRGBA(191, 151,  87, OPAQUE), //  41
	MakeRGBA(203, 175, 111, OPAQUE), //  42
	MakeRGBA(219, 199, 135, OPAQUE), //  43
	MakeRGBA(231, 219, 163, OPAQUE), //  44
	MakeRGBA(247, 239, 195, OPAQUE), //  45
	MakeRGBA( 71,  27,   0, OPAQUE), //  46 Start of COL_RANGE_YELLOW
	MakeRGBA( 95,  43,   0, OPAQUE), //  47
	MakeRGBA(119,  63,   0, OPAQUE), //  48
	MakeRGBA(143,  83,   7, OPAQUE), //  49
	MakeRGBA(167, 111,   7, OPAQUE), //  50
	MakeRGBA(191, 139,  15, OPAQUE), //  51
	MakeRGBA(215, 167,  19, OPAQUE), //  52
	MakeRGBA(243, 203,  27, OPAQUE), //  53
	MakeRGBA(255, 231,  47, OPAQUE), //  54
	MakeRGBA(255, 243,  95, OPAQUE), //  55
	MakeRGBA(255, 251, 143, OPAQUE), //  56
	MakeRGBA(255, 255, 195, OPAQUE), //  57
	MakeRGBA( 35,   0,   0, OPAQUE), //  58 Start of COL_RANGE_DARK_RED
	MakeRGBA( 79,   0,   0, OPAQUE), //  59
	MakeRGBA( 95,   7,   7, OPAQUE), //  60
	MakeRGBA(111,  15,  15, OPAQUE), //  61
	MakeRGBA(127,  27,  27, OPAQUE), //  62
	MakeRGBA(143,  39,  39, OPAQUE), //  63
	MakeRGBA(163,  59,  59, OPAQUE), //  64
	MakeRGBA(179,  79,  79, OPAQUE), //  65
	MakeRGBA(199, 103, 103, OPAQUE), //  66
	MakeRGBA(215, 127, 127, OPAQUE), //  67
	MakeRGBA(235, 159, 159, OPAQUE), //  68
	MakeRGBA(255, 191, 191, OPAQUE), //  69
	MakeRGBA( 27,  51,  19, OPAQUE), //  70 Start of COL_RANGE_DARK_GREEN
	MakeRGBA( 35,  63,  23, OPAQUE), //  71
	MakeRGBA( 47,  79,  31, OPAQUE), //  72
	MakeRGBA( 59,  95,  39, OPAQUE), //  73
	MakeRGBA( 71, 111,  43, OPAQUE), //  74
	MakeRGBA( 87, 127,  51, OPAQUE), //  75
	MakeRGBA( 99, 143,  59, OPAQUE), //  76
	MakeRGBA(115, 155,  67, OPAQUE), //  77
	MakeRGBA(131, 171,  75, OPAQUE), //  78
	MakeRGBA(147, 187,  83, OPAQUE), //  79
	MakeRGBA(163, 203,  95, OPAQUE), //  80
	MakeRGBA(183, 219, 103, OPAQUE), //  81
	MakeRGBA( 31,  55,  27, OPAQUE), //  82 Start of COL_RANGE_LIGHT_GREEN
	MakeRGBA( 47,  71,  35, OPAQUE), //  83
	MakeRGBA( 59,  83,  43, OPAQUE), //  84
	MakeRGBA( 75,  99,  55, OPAQUE), //  85
	MakeRGBA( 91, 111,  67, OPAQUE), //  86
	MakeRGBA(111, 135,  79, OPAQUE), //  87
	MakeRGBA(135, 159,  95, OPAQUE), //  88
	MakeRGBA(159, 183, 111, OPAQUE), //  89
	MakeRGBA(183, 207, 127, OPAQUE), //  90
	MakeRGBA(195, 219, 147, OPAQUE), //  91
	MakeRGBA(207, 231, 167, OPAQUE), //  92
	MakeRGBA(223, 247, 191, OPAQUE), //  93
	MakeRGBA( 15,  63,   0, OPAQUE), //  94 Start of COL_RANGE_GREEN
	MakeRGBA( 19,  83,   0, OPAQUE), //  95
	MakeRGBA( 23, 103,   0, OPAQUE), //  96
	MakeRGBA( 31, 123,   0, OPAQUE), //  97
	MakeRGBA( 39, 143,   7, OPAQUE), //  98
	MakeRGBA( 55, 159,  23, OPAQUE), //  99
	MakeRGBA( 71, 175,  39, OPAQUE), //  100
	MakeRGBA( 91, 191,  63, OPAQUE), //  101
	MakeRGBA(111, 207,  87, OPAQUE), //  102
	MakeRGBA(139, 223, 115, OPAQUE), //  103
	MakeRGBA(163, 239, 143, OPAQUE), //  104
	MakeRGBA(195, 255, 179, OPAQUE), //  105
	MakeRGBA( 79,  43,  19, OPAQUE), //  106 Start of COL_RANGE_PINK_BROWN
	MakeRGBA( 99,  55,  27, OPAQUE), //  107
	MakeRGBA(119,  71,  43, OPAQUE), //  108
	MakeRGBA(139,  87,  59, OPAQUE), //  109
	MakeRGBA(167,  99,  67, OPAQUE), //  110
	MakeRGBA(187, 115,  83, OPAQUE), //  111
	MakeRGBA(207, 131,  99, OPAQUE), //  112
	MakeRGBA(215, 151, 115, OPAQUE), //  113
	MakeRGBA(227, 171, 131, OPAQUE), //  114
	MakeRGBA(239, 191, 151, OPAQUE), //  115
	MakeRGBA(247, 207, 171, OPAQUE), //  116
	MakeRGBA(255, 227, 195, OPAQUE), //  117
	MakeRGBA( 15,  19,  55, OPAQUE), //  118 Start of COL_RANGE_DARK_PURPLE
	MakeRGBA( 39,  43,  87, OPAQUE), //  119
	MakeRGBA( 51,  55, 103, OPAQUE), //  120
	MakeRGBA( 63,  67, 119, OPAQUE), //  121
	MakeRGBA( 83,  83, 139, OPAQUE), //  122
	MakeRGBA( 99,  99, 155, OPAQUE), //  123
	MakeRGBA(119, 119, 175, OPAQUE), //  124
	MakeRGBA(139, 139, 191, OPAQUE), //  125
	MakeRGBA(159, 159, 207, OPAQUE), //  126
	MakeRGBA(183, 183, 223, OPAQUE), //  127
	MakeRGBA(211, 211, 239, OPAQUE), //  128
	MakeRGBA(239, 239, 255, OPAQUE), //  129
	MakeRGBA(  0,  27, 111, OPAQUE), //  130 Start of COL_RANGE_BLUE
	MakeRGBA(  0,  39, 151, OPAQUE), //  131
	MakeRGBA(  7,  51, 167, OPAQUE), //  132
	MakeRGBA( 15,  67, 187, OPAQUE), //  133
	MakeRGBA( 27,  83, 203, OPAQUE), //  134
	MakeRGBA( 43, 103, 223, OPAQUE), //  135
	MakeRGBA( 67, 135, 227, OPAQUE), //  136
	MakeRGBA( 91, 163, 231, OPAQUE), //  137
	MakeRGBA(119, 187, 239, OPAQUE), //  138
	MakeRGBA(143, 211, 243, OPAQUE), //  139
	MakeRGBA(175, 231, 251, OPAQUE), //  140
	MakeRGBA(215, 247, 255, OPAQUE), //  141
	MakeRGBA( 11,  43,  15, OPAQUE), //  142 Start of COL_RANGE_DARK_JADE_GREEN
	MakeRGBA( 15,  55,  23, OPAQUE), //  143
	MakeRGBA( 23,  71,  31, OPAQUE), //  144
	MakeRGBA( 35,  83,  43, OPAQUE), //  145
	MakeRGBA( 47,  99,  59, OPAQUE), //  146
	MakeRGBA( 59, 115,  75, OPAQUE), //  147
	MakeRGBA( 79, 135,  95, OPAQUE), //  148
	MakeRGBA( 99, 155, 119, OPAQUE), //  149
	MakeRGBA(123, 175, 139, OPAQUE), //  150
	MakeRGBA(147, 199, 167, OPAQUE), //  151
	MakeRGBA(175, 219, 195, OPAQUE), //  152
	MakeRGBA(207, 243, 223, OPAQUE), //  153
	MakeRGBA( 63,   0,  95, OPAQUE), //  154 Start of COL_RANGE_PURPLE
	MakeRGBA( 75,   7, 115, OPAQUE), //  155
	MakeRGBA( 83,  15, 127, OPAQUE), //  156
	MakeRGBA( 95,  31, 143, OPAQUE), //  157
	MakeRGBA(107,  43, 155, OPAQUE), //  158
	MakeRGBA(123,  63, 171, OPAQUE), //  159
	MakeRGBA(135,  83, 187, OPAQUE), //  160
	MakeRGBA(155, 103, 199, OPAQUE), //  161
	MakeRGBA(171, 127, 215, OPAQUE), //  162
	MakeRGBA(191, 155, 231, OPAQUE), //  163
	MakeRGBA(215, 195, 243, OPAQUE), //  164
	MakeRGBA(243, 235, 255, OPAQUE), //  165
	MakeRGBA( 63,   0,   0, OPAQUE), //  166 Start of COL_RANGE_RED
	MakeRGBA( 87,   0,   0, OPAQUE), //  167
	MakeRGBA(115,   0,   0, OPAQUE), //  168
	MakeRGBA(143,   0,   0, OPAQUE), //  169
	MakeRGBA(171,   0,   0, OPAQUE), //  170
	MakeRGBA(199,   0,   0, OPAQUE), //  171
	MakeRGBA(227,   7,   0, OPAQUE), //  172
	MakeRGBA(255,   7,   0, OPAQUE), //  173
	MakeRGBA(255,  79,  67, OPAQUE), //  174
	MakeRGBA(255, 123, 115, OPAQUE), //  175
	MakeRGBA(255, 171, 163, OPAQUE), //  176
	MakeRGBA(255, 219, 215, OPAQUE), //  177
	MakeRGBA( 79,  39,   0, OPAQUE), //  178 Start of COL_RANGE_ORANGE
	MakeRGBA(111,  51,   0, OPAQUE), //  179
	MakeRGBA(147,  63,   0, OPAQUE), //  180
	MakeRGBA(183,  71,   0, OPAQUE), //  181
	MakeRGBA(219,  79,   0, OPAQUE), //  182
	MakeRGBA(255,  83,   0, OPAQUE), //  183
	MakeRGBA(255, 111,  23, OPAQUE), //  184
	MakeRGBA(255, 139,  51, OPAQUE), //  185
	MakeRGBA(255, 163,  79, OPAQUE), //  186
	MakeRGBA(255, 183, 107, OPAQUE), //  187
	MakeRGBA(255, 203, 135, OPAQUE), //  188
	MakeRGBA(255, 219, 163, OPAQUE), //  189
	MakeRGBA(  0,  51,  47, OPAQUE), //  190 Start of COL_RANGE_SEA_GREEN
	MakeRGBA(  0,  63,  55, OPAQUE), //  191
	MakeRGBA(  0,  75,  67, OPAQUE), //  192
	MakeRGBA(  0,  87,  79, OPAQUE), //  193
	MakeRGBA(  7, 107,  99, OPAQUE), //  194
	MakeRGBA( 23, 127, 119, OPAQUE), //  195
	MakeRGBA( 43, 147, 143, OPAQUE), //  196
	MakeRGBA( 71, 167, 163, OPAQUE), //  197
	MakeRGBA( 99, 187, 187, OPAQUE), //  198
	MakeRGBA(131, 207, 207, OPAQUE), //  199
	MakeRGBA(171, 231, 231, OPAQUE), //  200
	MakeRGBA(207, 255, 255, OPAQUE), //  201
	MakeRGBA( 63,   0,  27, OPAQUE), //  202 Start of COL_RANGE_PINK
	MakeRGBA( 91,   0,  39, OPAQUE), //  203
	MakeRGBA(119,   0,  59, OPAQUE), //  204
	MakeRGBA(147,   7,  75, OPAQUE), //  205
	MakeRGBA(179,  11,  99, OPAQUE), //  206
	MakeRGBA(199,  31, 119, OPAQUE), //  207
	MakeRGBA(219,  59, 143, OPAQUE), //  208
	MakeRGBA(239,  91, 171, OPAQUE), //  209
	MakeRGBA(243, 119, 187, OPAQUE), //  210
	MakeRGBA(247, 151, 203, OPAQUE), //  211
	MakeRGBA(251, 183, 223, OPAQUE), //  212
	MakeRGBA(255, 215, 239, OPAQUE), //  213
	MakeRGBA( 39,  19,   0, OPAQUE), //  214 Start COL_RANGE_BROWN
	MakeRGBA( 55,  31,   7, OPAQUE), //  215
	MakeRGBA( 71,  47,  15, OPAQUE), //  216
	MakeRGBA( 91,  63,  31, OPAQUE), //  217
	MakeRGBA(107,  83,  51, OPAQUE), //  218
	MakeRGBA(123, 103,  75, OPAQUE), //  219
	MakeRGBA(143, 127, 107, OPAQUE), //  220
	MakeRGBA(163, 147, 127, OPAQUE), //  221
	MakeRGBA(187, 171, 147, OPAQUE), //  222
	MakeRGBA(207, 195, 171, OPAQUE), //  223
	MakeRGBA(231, 219, 195, OPAQUE), //  224
	MakeRGBA(255, 243, 223, OPAQUE), //  225
	MakeRGBA(255,   0, 255, OPAQUE), //  226 COL_RANGE_COUNT (=COL_SERIES_END)
	MakeRGBA(255, 183,   0, OPAQUE), //  227
	MakeRGBA(255, 219,   0, OPAQUE), //  228
	MakeRGBA(255, 255,   0, OPAQUE), //  229
	MakeRGBA(  7, 107,  99, OPAQUE), //  230
	MakeRGBA(  7, 107,  99, OPAQUE), //  231
	MakeRGBA(  7, 107,  99, OPAQUE), //  232
	MakeRGBA( 27, 131, 123, OPAQUE), //  233
	MakeRGBA( 39, 143, 135, OPAQUE), //  234
	MakeRGBA( 55, 155, 151, OPAQUE), //  235
	MakeRGBA( 55, 155, 151, OPAQUE), //  236
	MakeRGBA( 55, 155, 151, OPAQUE), //  237
	MakeRGBA(115, 203, 203, OPAQUE), //  238
	MakeRGBA(155, 227, 227, OPAQUE), //  239
	MakeRGBA( 47,  47,  47, OPAQUE), //  240
	MakeRGBA( 87,  71,  47, OPAQUE), //  241
	MakeRGBA( 47,  47,  47, OPAQUE), //  242
	MakeRGBA(  0,   0,  99, OPAQUE), //  243
	MakeRGBA( 27,  43, 139, OPAQUE), //  244
	MakeRGBA( 39,  59, 151, OPAQUE), //  245
	MakeRGBA(  0,   0,   0, OPAQUE), //  246
	MakeRGBA(  0,   0,   0, OPAQUE), //  247
	MakeRGBA(  0,   0,   0, OPAQUE), //  248
	MakeRGBA(  0,   0,   0, OPAQUE), //  249
	MakeRGBA(  0,   0,   0, OPAQUE), //  250
	MakeRGBA(  0,   0,   0, OPAQUE), //  251
	MakeRGBA(  0,   0,   0, OPAQUE), //  252
	MakeRGBA(  0,   0,   0, OPAQUE), //  253
	MakeRGBA(  0,   0,   0, OPAQUE), //  254
	MakeRGBA(  0,   0,   0, OPAQUE), //  255
};

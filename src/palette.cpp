/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file palette.cpp 8bpp palette definitions. */

#include "stdafx.h"
#include "palette.h"
#include "memory.h"
#include "random.h"

/** Default constructor. */
Recolouring::Recolouring()
{
	for (int i = 0; i < COL_RANGE_COUNT; i++) this->range_map[i] = i;
}

/**
 * Copy constructor.
 * @param rc Recolouring to use as template.
 */
Recolouring::Recolouring(const Recolouring &rc)
{
	MemCpy(this->range_map, rc.range_map, COL_RANGE_COUNT);
}

/**
 * Assignment operator.
 * @param rc Recolouring to use as template.
 * @return The assigned value.
 */
Recolouring &Recolouring::operator=(const Recolouring &rc)
{
	if (this != &rc) {
		MemCpy(this->range_map, rc.range_map, COL_RANGE_COUNT);
	}
	return *this;
}

/**
 * Set up recolouring of a range.
 * @param orig Colour range to recolour.
 * @param dest Colour range to recolour to.
 */
void Recolouring::SetRecolouring(ColourRange orig, ColourRange dest)
{
	assert(orig < COL_RANGE_COUNT);
	assert(dest < COL_RANGE_COUNT);

	this->range_map[orig] = dest;
}


/** Default constructor of the random recolour mapping. */
RandomRecolouringMapping::RandomRecolouringMapping() : range_number(COL_RANGE_INVALID), dest_set(0)
{
}

/**
 * Decide a colour for the random colour range.
 * @param rnd %Random number generator.
 * @return Colour range to use for #range_number.
 */
ColourRange RandomRecolouringMapping::DrawRandomColour(Random *rnd) const
{
	assert(this->range_number != COL_RANGE_INVALID);

	int ranges[COL_RANGE_COUNT];
	int count = 0;
	uint32 bit = 1;
	for (int i = 0; i < (int)lengthof(ranges); i++) {
		if ((bit & this->dest_set) != 0) ranges[count++] = i;
		bit <<= 1;
	}
	if (count == 0) return static_cast<ColourRange>(this->range_number); // No range to replace the original colour with.
	if (count == 1) return static_cast<ColourRange>(ranges[0]); // Just one colour, easy choice.
	return static_cast<ColourRange>(ranges[rnd->Uniform(count - 1)]);
}


/** Default constructor. */
EditableRecolouring::EditableRecolouring() : Recolouring()
{
	for (int i = 0; i < COL_RANGE_COUNT; i++) {
		this->name_map[i] = STR_NULL;
		this->tip_map[i] = STR_NULL;
	}
}

/**
 * Copy constructor.
 * @param er Recolouring to use as template.
 */
EditableRecolouring::EditableRecolouring(const EditableRecolouring &er) : Recolouring(er)
{
	MemCpy(this->name_map, er.name_map, COL_RANGE_COUNT);
	MemCpy(this->tip_map,  er.tip_map,  COL_RANGE_COUNT);
}

/**
 * Assignment operator.
 * @param er Recolouring to use as template.
 * @return The assigned value.
 */
EditableRecolouring &EditableRecolouring::operator=(const EditableRecolouring &er)
{
	if (this != &er) {
		MemCpy(this->range_map, er.range_map, COL_RANGE_COUNT);
		MemCpy(this->name_map,  er.name_map,  COL_RANGE_COUNT);
		MemCpy(this->tip_map,   er.tip_map,   COL_RANGE_COUNT);
	}
	return *this;
}

/**
 * Setup recolouring of a range.
 * @param orig Colour range to recolour.
 * @param dest Colour range to recolour to.
 * @param name The name of the colour (so the user knows what gets changed in the gui). May be #STR_NULL.
 * @param tooltip If \a name was not #STR_NULL, the tooltip describing the meaning.
 */
void EditableRecolouring::SetRecolouring(ColourRange orig, ColourRange dest, StringID name, StringID tooltip)
{
	assert(orig < COL_RANGE_COUNT);
	assert(dest < COL_RANGE_COUNT);

	this->range_map[orig] = dest;
	if (name != STR_NULL) {
		this->name_map[orig] = name;
		this->tip_map[orig]  = tooltip;
	}
}

const uint8 _palette[256][3] = {
	{   0,   0,   0}, //  0 COL_BACKGROUND (background behind world display)
	{ 255, 255, 255}, //  1 COL_HIGHLIGHT (full white to highlight window edge)
	{   0,   0,   0}, //  2 unused
	{   0,   0,   0}, //  3 unused
	{   0,   0,   0}, //  4 unused
	{   0,   0,   0}, //  5 unused
	{   0,   0,   0}, //  6 unused
	{   0,   0,   0}, //  7 unused
	{   0,   0,   0}, //  8 unused
	{   0,   0,   0}, //  9 unused
	{  23,  35,  35}, //  10 Start of COL_RANGE_GREY.
	{  35,  51,  51}, //  11
	{  47,  67,  67}, //  12
	{  63,  83,  83}, //  13
	{  75,  99,  99}, //  14
	{  91, 115, 115}, //  15
	{ 111, 131, 131}, //  16
	{ 131, 151, 151}, //  17
	{ 159, 175, 175}, //  18
	{ 183, 195, 195}, //  19
	{ 211, 219, 219}, //  20
	{ 239, 243, 243}, //  21
	{  51,  47,   0}, //  22 Start of COL_RANGE_GREEN_BROWN.
	{  63,  59,   0}, //  23
	{  79,  75,  11}, //  24
	{  91,  91,  19}, //  25
	{ 107, 107,  31}, //  26
	{ 119, 123,  47}, //  27
	{ 135, 139,  59}, //  28
	{ 151, 155,  79}, //  29
	{ 167, 175,  95}, //  30
	{ 187, 191, 115}, //  31
	{ 203, 207, 139}, //  32
	{ 223, 227, 163}, //  33
	{  67,  43,   7}, //  34 Start of COL_RANGE_BROWN
	{  87,  59,  11}, //  35
	{ 111,  75,  23}, //  36
	{ 127,  87,  31}, //  37
	{ 143,  99,  39}, //  38
	{ 159, 115,  51}, //  39
	{ 179, 131,  67}, //  40
	{ 191, 151,  87}, //  41
	{ 203, 175, 111}, //  42
	{ 219, 199, 135}, //  43
	{ 231, 219, 163}, //  44
	{ 247, 239, 195}, //  45
	{  71,  27,   0}, //  46 Start of COL_RANGE_YELLOW
	{  95,  43,   0}, //  47
	{ 119,  63,   0}, //  48
	{ 143,  83,   7}, //  49
	{ 167, 111,   7}, //  50
	{ 191, 139,  15}, //  51
	{ 215, 167,  19}, //  52
	{ 243, 203,  27}, //  53
	{ 255, 231,  47}, //  54
	{ 255, 243,  95}, //  55
	{ 255, 251, 143}, //  56
	{ 255, 255, 195}, //  57
	{  35,   0,   0}, //  58 Start of COL_RANGE_DARK_RED
	{  79,   0,   0}, //  59
	{  95,   7,   7}, //  60
	{ 111,  15,  15}, //  61
	{ 127,  27,  27}, //  62
	{ 143,  39,  39}, //  63
	{ 163,  59,  59}, //  64
	{ 179,  79,  79}, //  65
	{ 199, 103, 103}, //  66
	{ 215, 127, 127}, //  67
	{ 235, 159, 159}, //  68
	{ 255, 191, 191}, //  69
	{  27,  51,  19}, //  70 Start of COL_RANGE_DARK_GREEN
	{  35,  63,  23}, //  71
	{  47,  79,  31}, //  72
	{  59,  95,  39}, //  73
	{  71, 111,  43}, //  74
	{  87, 127,  51}, //  75
	{  99, 143,  59}, //  76
	{ 115, 155,  67}, //  77
	{ 131, 171,  75}, //  78
	{ 147, 187,  83}, //  79
	{ 163, 203,  95}, //  80
	{ 183, 219, 103}, //  81
	{  31,  55,  27}, //  82 Start of COL_RANGE_LIGHT_GREEN
	{  47,  71,  35}, //  83
	{  59,  83,  43}, //  84
	{  75,  99,  55}, //  85
	{  91, 111,  67}, //  86
	{ 111, 135,  79}, //  87
	{ 135, 159,  95}, //  88
	{ 159, 183, 111}, //  89
	{ 183, 207, 127}, //  90
	{ 195, 219, 147}, //  91
	{ 207, 231, 167}, //  92
	{ 223, 247, 191}, //  93
	{  15,  63,   0}, //  94 Start of COL_RANGE_GREEN
	{  19,  83,   0}, //  95
	{  23, 103,   0}, //  96
	{  31, 123,   0}, //  97
	{  39, 143,   7}, //  98
	{  55, 159,  23}, //  99
	{  71, 175,  39}, //  100
	{  91, 191,  63}, //  101
	{ 111, 207,  87}, //  102
	{ 139, 223, 115}, //  103
	{ 163, 239, 143}, //  104
	{ 195, 255, 179}, //  105
	{  79,  43,  19}, //  106 Start of COL_RANGE_LIGHT_RED
	{  99,  55,  27}, //  107
	{ 119,  71,  43}, //  108
	{ 139,  87,  59}, //  109
	{ 167,  99,  67}, //  110
	{ 187, 115,  83}, //  111
	{ 207, 131,  99}, //  112
	{ 215, 151, 115}, //  113
	{ 227, 171, 131}, //  114
	{ 239, 191, 151}, //  115
	{ 247, 207, 171}, //  116
	{ 255, 227, 195}, //  117
	{  15,  19,  55}, //  118 Start of COL_RANGE_DARK_BLUE
	{  39,  43,  87}, //  119
	{  51,  55, 103}, //  120
	{  63,  67, 119}, //  121
	{  83,  83, 139}, //  122
	{  99,  99, 155}, //  123
	{ 119, 119, 175}, //  124
	{ 139, 139, 191}, //  125
	{ 159, 159, 207}, //  126
	{ 183, 183, 223}, //  127
	{ 211, 211, 239}, //  128
	{ 239, 239, 255}, //  129
	{   0,  27, 111}, //  130 Start of COL_RANGE_BLUE
	{   0,  39, 151}, //  131
	{   7,  51, 167}, //  132
	{  15,  67, 187}, //  133
	{  27,  83, 203}, //  134
	{  43, 103, 223}, //  135
	{  67, 135, 227}, //  136
	{  91, 163, 231}, //  137
	{ 119, 187, 239}, //  138
	{ 143, 211, 243}, //  139
	{ 175, 231, 251}, //  140
	{ 215, 247, 255}, //  141
	{  11,  43,  15}, //  142 Start of COL_RANGE_LIGHT_BLUE
	{  15,  55,  23}, //  143
	{  23,  71,  31}, //  144
	{  35,  83,  43}, //  145
	{  47,  99,  59}, //  146
	{  59, 115,  75}, //  147
	{  79, 135,  95}, //  148
	{  99, 155, 119}, //  149
	{ 123, 175, 139}, //  150
	{ 147, 199, 167}, //  151
	{ 175, 219, 195}, //  152
	{ 207, 243, 223}, //  153
	{  63,   0,  95}, //  154 Start of COL_RANGE_PURPLE
	{  75,   7, 115}, //  155
	{  83,  15, 127}, //  156
	{  95,  31, 143}, //  157
	{ 107,  43, 155}, //  158
	{ 123,  63, 171}, //  159
	{ 135,  83, 187}, //  160
	{ 155, 103, 199}, //  161
	{ 171, 127, 215}, //  162
	{ 191, 155, 231}, //  163
	{ 215, 195, 243}, //  164
	{ 243, 235, 255}, //  165
	{  63,   0,   0}, //  166 Start of COL_RANGE_RED
	{  87,   0,   0}, //  167
	{ 115,   0,   0}, //  168
	{ 143,   0,   0}, //  169
	{ 171,   0,   0}, //  170
	{ 199,   0,   0}, //  171
	{ 227,   7,   0}, //  172
	{ 255,   7,   0}, //  173
	{ 255,  79,  67}, //  174
	{ 255, 123, 115}, //  175
	{ 255, 171, 163}, //  176
	{ 255, 219, 215}, //  177
	{  79,  39,   0}, //  178 Start of COL_RANGE_ORANGE
	{ 111,  51,   0}, //  179
	{ 147,  63,   0}, //  180
	{ 183,  71,   0}, //  181
	{ 219,  79,   0}, //  182
	{ 255,  83,   0}, //  183
	{ 255, 111,  23}, //  184
	{ 255, 139,  51}, //  185
	{ 255, 163,  79}, //  186
	{ 255, 183, 107}, //  187
	{ 255, 203, 135}, //  188
	{ 255, 219, 163}, //  189
	{   0,  51,  47}, //  190 Start of COL_RANGE_SEA_GREEN
	{   0,  63,  55}, //  191
	{   0,  75,  67}, //  192
	{   0,  87,  79}, //  193
	{   7, 107,  99}, //  194
	{  23, 127, 119}, //  195
	{  43, 147, 143}, //  196
	{  71, 167, 163}, //  197
	{  99, 187, 187}, //  198
	{ 131, 207, 207}, //  199
	{ 171, 231, 231}, //  200
	{ 207, 255, 255}, //  201
	{  63,   0,  27}, //  202 Start of COL_RANGE_PINK
	{  91,   0,  39}, //  203
	{ 119,   0,  59}, //  204
	{ 147,   7,  75}, //  205
	{ 179,  11,  99}, //  206
	{ 199,  31, 119}, //  207
	{ 219,  59, 143}, //  208
	{ 239,  91, 171}, //  209
	{ 243, 119, 187}, //  210
	{ 247, 151, 203}, //  211
	{ 251, 183, 223}, //  212
	{ 255, 215, 239}, //  213
	{  39,  19,   0}, //  214 Start COL_RANGE_BEIGE
	{  55,  31,   7}, //  215
	{  71,  47,  15}, //  216
	{  91,  63,  31}, //  217
	{ 107,  83,  51}, //  218
	{ 123, 103,  75}, //  219
	{ 143, 127, 107}, //  220
	{ 163, 147, 127}, //  221
	{ 187, 171, 147}, //  222
	{ 207, 195, 171}, //  223
	{ 231, 219, 195}, //  224
	{ 255, 243, 223}, //  225
	{ 255,   0, 255}, //  226 COL_RANGE_COUNT (=COL_SERIES_END)
	{ 255, 183,   0}, //  227
	{ 255, 219,   0}, //  228
	{ 255, 255,   0}, //  229
	{   7, 107,  99}, //  230
	{   7, 107,  99}, //  231
	{   7, 107,  99}, //  232
	{  27, 131, 123}, //  233
	{  39, 143, 135}, //  234
	{  55, 155, 151}, //  235
	{  55, 155, 151}, //  236
	{  55, 155, 151}, //  237
	{ 115, 203, 203}, //  238
	{ 155, 227, 227}, //  239
	{  47,  47,  47}, //  240
	{  87,  71,  47}, //  241
	{  47,  47,  47}, //  242
	{   0,   0,  99}, //  243
	{  27,  43, 139}, //  244
	{  39,  59, 151}, //  245
	{   0,   0,   0}, //  246
	{   0,   0,   0}, //  247
	{   0,   0,   0}, //  248
	{   0,   0,   0}, //  249
	{   0,   0,   0}, //  250
	{   0,   0,   0}, //  251
	{   0,   0,   0}, //  252
	{   0,   0,   0}, //  253
	{   0,   0,   0}, //  254
	{   0,   0,   0}, //  255
};

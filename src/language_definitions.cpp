/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file language_definitions.cpp Definitions of known languages. */

#include "language_definitions.h"

FontSet FONT_LATIN({{0, 0x303f}, {0xFD3E, 0xfffd}});
FontSet FONT_CJK({{0, 0x303f}, {0x4E00, 0x9FFF}, {0xFD3E, 0xfffd}});

/**
 * Get the index number of a given language.
 * @param lang_name Name of the language.
 * @return Index of the language with the provided name, or \c -1 if not recognized.
 */
int GetLanguageIndex(const std::string &lang_name)
{
	int start = 0; // Exclusive lower bound.
	int end = LANGUAGE_COUNT; // Exclusive upper bound.

	while (start + 1 < end) {
		int middle = (start + end) / 2;
		int cmp = lang_name.compare(_all_languages[middle].name);
		if (cmp == 0) return middle; // Jack pot.
		if (cmp > 0) {
			start = middle;
		} else {
			end = middle;
		}
	}
	if (lang_name == _all_languages[start].name) return start;
	return -1;
}

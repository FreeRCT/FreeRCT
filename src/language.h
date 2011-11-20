/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file language.h %Language support. */

#ifndef LANGUAGE_H
#define LANGUAGE_H

#include "table/strings.h"

/** Table of strings used in the game. */
enum StringTable {
	STR_NULL = 0,        ///< \c NULL string.
	STR_EMPTY = 1,       ///< Empty string.

	STR_GENERIC_COUNT,   ///< Number of generic strings.
};

/**
 * Class for retrieving language strings.
 * @todo Implement me.
 */
class Language {
public:
	Language();
	~Language();

	const char *Load(const char *fname);

	uint16 num_texts; ///< Number of strings in the language.
	char *text;       ///< The actual text (all of it).
	char **strings;   ///< #num_texts indices into #text.
};

void InitLanguage();
void UninitLanguage();

extern Language *_language;

#endif

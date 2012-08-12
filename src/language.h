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

extern int _current_language;

/** Table of strings used in the game. */
enum StringTable {
	STR_NULL = 0,        ///< \c NULL string.
	STR_EMPTY = 1,       ///< Empty string.

	STR_GENERIC_COUNT,   ///< Number of generic strings.
};

typedef uint16 StringID; ///< Type of a string value.

/** Languages known by the program. */
enum Languages {
	DEFAULT_LANGUAGE = 0, ///< Id of the default language.

	LANGUAGE_COUNT = 3,   ///< Number of available languages.
};

/**
 * A string with its name and its translations. Memory of the text is not owned by the class.
 */
class TextString {
public:
	TextString();

	void Clear();

	/**
	 * Get the string in the currently selected language.
	 * @return Text of this string in the currently selected language.
	 */
	const uint8 *GetString()
	{
		if (_current_language < 0 || _current_language >= LANGUAGE_COUNT ||
				this->languages[_current_language] == NULL) return (uint8 *)"";
		return this->languages[_current_language];
	}

	const char *name;                      ///< Name of the string.
	const uint8 *languages[LANGUAGE_COUNT]; ///< The string in all languages.
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
	void Clear();

	const char *GetText(StringID number);

	uint16 num_texts; ///< Number of strings in the language.
	char *text;       ///< The actual text (all of it).
	char **strings;   ///< #num_texts indices into #text.
};

int GetLanguageIndex(const char *lang_name);
void InitLanguage();
void UninitLanguage();

extern Language _language;

#endif

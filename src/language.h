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

class TextData;

extern int _current_language;

/** Table of string-parts in the game. */
enum StringTable {
	STR_NULL = 0,  ///< \c NULL string.
	STR_EMPTY = 1, ///< Empty string.

	STR_GUI_START, ///< Start of the GUI strings.

	/* After the gui strings come the other registered strings. */

	STR_END_FREE_SPACE = 0xF800,
	STR_GENERIC_SHOP_START = STR_END_FREE_SPACE,

	STR_GENERIC_END = 0xFFFF,
};

#include "table/shops_strings.h"

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
	const uint8 *GetString() const
	{
		if (_current_language < 0 || _current_language >= LANGUAGE_COUNT) return (uint8 *)"<out of bounds>";
		if (this->languages[_current_language] != NULL) return this->languages[_current_language];
		if (this->languages[0] != NULL) return this->languages[0];
		return (uint8 *)"<no-text>";
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

	uint16 RegisterStrings(const TextData &td, const char * const names[], uint16 base=STR_GENERIC_END);

	const uint8 *GetText(StringID number);

	uint16 num_texts; ///< Number of strings in the language.
	uint8 *text;       ///< The actual text (all of it).
	uint8 **strings;   ///< #num_texts indices into #text.

private:
	/** Registered strings. Entries may be \c NULL for unregistered or non-existing strings. */
	const TextString *registered[2048]; // Arbitrary size.
	uint first_free; ///< 'First' string index that is not allocated yet.
};

int GetLanguageIndex(const char *lang_name);
void InitLanguage();
void UninitLanguage();

extern Language _language;

#endif

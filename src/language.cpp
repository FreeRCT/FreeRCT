/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file language.cpp %Language support code. */

#include "stdafx.h"
#include "language.h"
#include "fileio.h"
#include "sprite_store.h"

assert_compile((int)GUI_STRING_TABLE_END < STR_END_FREE_SPACE); ///< Ensure there are not too many gui strings.
assert_compile((int)SHOPS_STRING_TABLE_END < STR_GENERIC_END);  ///< Ensure there are not too many shops strings.

Language _language; ///< Language strings.
int _current_language = 0; ///< Index of the current translation.

/** Default constructor of a #TextString object. */
TextString::TextString()
{
	this->Clear();
}

/** Drop all strings. Since the text is not owned by the class, no need to free the memory. */
void TextString::Clear()
{
	this->name = NULL;
	for (uint i = 0; i < lengthof(this->languages); i++) this->languages[i] = NULL;
}


/** Known languages. */
const char * const _lang_names[] = {
	"",      // Default language.
	"en_GB", // English (mostly empty, as it is also the default language).
	"nl_NL", // Dutch.
};

assert_compile(lengthof(_lang_names) == LANGUAGE_COUNT); ///< Ensure number of language matches with array sizes.


/**
 * Get the index number of a given language.
 * @param lang_name Name of the language.
 * @return Index of the language with the provided name, or \c -1 if not recognized.
 */
int GetLanguageIndex(const char *lang_name)
{
	int start = 0; // Exclusive lower bound.
	int end = LANGUAGE_COUNT; // Exclusive upper bound.

	while (start + 1 < end) {
		int middle = (start + end) / 2;
		int cmp = strcmp(_lang_names[middle], lang_name);
		if (cmp == 0) return middle; // Jack pot.
		if (cmp < 0) {
			start = middle;
		} else {
			end = middle;
		}
	}
	if (!strcmp(_lang_names[start], lang_name)) return start;
	return -1;
}


Language::Language()
{
	for (uint i = 0; i < lengthof(this->registered); i++) this->registered[i] = NULL;
	this->first_free = GUI_STRING_TABLE_END;
}

Language::~Language()
{
	this->Clear();
}

/** Clear all loaded data. */
void Language::Clear()
{
}

/**
 * Register loaded strings of rides etc with the language system.
 * @param td Text data wrapper containing the loaded strings.
 * @param names Array of names to register.
 * @param base Base address of the strings.
 * @return Base offset for the registered strings. Add the index value of the \a names table to get the real string number.
 */
uint16 Language::RegisterStrings(const TextData &td, const char * const names[], uint16 base)
{
	/* Count the number of strings. */
	uint num_strings = 0;
	const char * const *str = names;
	while (*str != NULL) {
		num_strings++;
		str++;
	}

	if (base == STR_GENERIC_END) {
		/* Check for enough free space, and assign a block. */
		if (this->first_free + num_strings >= STR_END_FREE_SPACE) {
			fprintf(stderr, "Not enough space to store strings.\n");
			exit(1);
		}
		base = this->first_free;
		this->first_free += num_strings;
	} else {
		/* Pre-defined strings should be completely below the free space. */
		assert(base + num_strings >= base && base + num_strings <= this->first_free);
	}

	/* Copy strings in the expected order. */
	uint16 number = base;
	str = names;
	while (*str != NULL) {
		this->registered[number] = NULL;
		for (uint i = 0; i < td.string_count; i++) {
			const TextString *ts = td.strings +i;
			if (!strcmp(*str, ts->name)) {
				this->registered[number] = ts;
				break;
			}
		}
		number++;
		str++;
	}
	return base;
}

/**
 * Get string number \a number.
 * @param number string number to get.
 * @return String corresponding to the number (not owned by the caller, so don't free it).
 */
const uint8 *Language::GetText(StringID number)
{
	if (number < STR_GUI_START) {
		switch (number) {
			case STR_NULL:  return NULL;
			case STR_EMPTY: return (const uint8 *)"";
			default: NOT_REACHED();
		}
	}

	if (number < lengthof(this->registered) && this->registered[number] != NULL) {
		const uint8 *text = this->registered[number]->GetString();
		if (*text == '\0') return (const uint8 *)"<empty text>";
		return text;
	}

	return (const uint8 *)"<Invalid string>";
}

/**
 * Initialize language support.
 */
void InitLanguage()
{
}

/** Clean up the language. */
void UninitLanguage()
{
	_language.Clear();
}

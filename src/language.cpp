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

Language _language; ///< Current language.
int _current_language = 0; ///< Index of the current language

/** Verify that the generic strings are different from the strings in the language table. */
assert_compile(STR_GENERIC_COUNT <= STRING_TABLE_START);

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
	return -1;
}


Language::Language()
{
	this->num_texts = 0;
	this->text = NULL;
	this->strings = NULL;

	for (uint i = 0; i < lengthof(this->registered); i++) this->registered[i] = NULL;
	this->first_free = STRING_TABLE_START + STRING_TABLE_LENGTH;
}

Language::~Language()
{
	this->Clear();
}

/** Clear all loaded data. */
void Language::Clear()
{
	free(this->text);
	free(this->strings);

	this->num_texts = 0;
	this->text = NULL;
	this->strings = NULL;
}

/**
 * Load a language.
 * @param fname Filename containing the language.
 * @return \c NULL if all went fine, else error message.
 */
const char *Language::Load(const char *fname)
{
	char name[8];
	name[4] = '\0';
	uint32 version, length;

	RcdFile rcd_file(fname);
	if (!rcd_file.CheckFileHeader("RCDS", 1)) return "Could not read header";

	/* Load language block (only one expected). */
	size_t remain = rcd_file.GetRemaining();
	if (remain < 12) return "Insufficient space for a block"; // Not enough for a rcd block header, abort.

	if (!rcd_file.GetBlob(name, 4)) return "Loading block name failed";
	version = rcd_file.GetUInt32();
	length = rcd_file.GetUInt32();

	if (length + 12 > remain) return "Not enough data"; // Not enough data in the file.
	if (strcmp(name, "LTXT") != 0 || version != 2) return "Unknown block";
	if (length < 2 + 8) return "LTXT block too short";

	uint16 num_texts = rcd_file.GetUInt16();
	if (num_texts != STRING_TABLE_LENGTH) return "Incorrect number of strings in the data file.";
	assert(num_texts < 2000); // Arbitrary limit which should be sufficient for some time.
	if (!rcd_file.GetBlob(name, 8)) return "Loading language name failed";
	if (name[7] != '\0') return "Language name terminator missing";

	length -= 10;
	assert(length >= num_texts && length < 100000); // Arbitrary upper limit which should be sufficient for some time.

	this->num_texts = num_texts;
	this->text = (uint8 *)malloc(length);
	this->strings = (uint8 **)malloc(sizeof(const uint8 **) * num_texts);

	if (this->text == NULL || this->strings == NULL) return "Insufficient memory to load language";
	if (!rcd_file.GetBlob(this->text, length)) return "Loading language texts failed";

	uint16 idx = 0;
	uint32 offset = 0;
	while (idx < num_texts && offset < length) {
		this->strings[idx] = this->text + offset;
		idx++;

		while (offset < length) {
			if(this->text[offset] == '\0') {
				break;
			}
			offset++;
		}
		offset++;
	}
	/* Note that if the \0 detection falls out of the 'while' on length,
	 * the 'offset++' below the while makes this check hold. */
	if (idx != num_texts || offset != length) return "Wrong language text format";

	remain = rcd_file.GetRemaining();
	if (remain != 0) return "Unexpected additional block";

	return NULL;
}

/**
 * Register loaded strings of rides etc with the language system.
 * @param td Text data wrapper containing the loaded strings.
 * @param names Array of names to register.
 * @return Base offset for the registered strings. Add the index value of the \a names table to get the real string number.
 */
uint16 Language::RegisterStrings(const TextData &td, const char * const names[])
{
	uint num_strings = 0;
	const char * const *str = names;
	while (*str != NULL && this->first_free + num_strings <= lengthof(this->registered)) {
		num_strings++;
		str++;
	}
	if (this->first_free + num_strings > lengthof(this->registered)) {
		fprintf(stderr, "Not enough space to store strings.\n");
		exit(1);
	}

	uint16 orig_free = this->first_free;
	str = names;
	while (*str != NULL) {
		this->registered[this->first_free] = NULL;
		for (uint i = 0; i < td.string_count; i++) {
			const TextString *ts = td.strings +i;
			if (!strcmp(*str, ts->name)) {
				this->registered[this->first_free] = ts;
				break;
			}
		}
		this->first_free++;
		str++;
	}
	return orig_free;
}

/**
 * Get string number \a number.
 * @param number string number to get.
 * @return String corresponding to the number (not owned by the caller, so don't free it).
 */
const uint8 *Language::GetText(StringID number)
{
	if (number < STRING_TABLE_START) {
		switch (number) {
			case STR_NULL:  return NULL;
			case STR_EMPTY: return (const uint8 *)"";
			default: NOT_REACHED();
		}
	}

	if (number < STRING_TABLE_START + this->num_texts) return this->strings[number - STRING_TABLE_START];

	if (number < lengthof(this->registered) && this->registered[number] != NULL) {
		this->registered[number]->GetString();
	}

	return (const uint8 *)"Invalid string number";
}

/**
 * Initialize language support.
 * @todo Implement me.
 */
void InitLanguage()
{
	const char *msg = _language.Load("../rcd/english.lang");
	if (msg != NULL) {
		fprintf(stderr, "Loading language failed: '%s'\n", msg);
		exit(1);
	}
}

/** Clean up the language. */
void UninitLanguage()
{
	_language.Clear();
}

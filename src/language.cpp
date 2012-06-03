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

/** Verify that the generic strings are different from the strings in the language table. */
assert_compile(STR_GENERIC_COUNT <= STRING_TABLE_START);

Language *_language; ///< Current language.

Language::Language()
{
	this->num_texts = 0;
	this->text = NULL;
	this->strings = NULL;
}

Language::~Language()
{
	free(this->text);
	free(this->strings);
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
	size_t remain = rcd_file.Remaining();
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
	this->text = (char *)malloc(length);
	this->strings = (char **)malloc(sizeof(const char **) * num_texts);

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

	remain = rcd_file.Remaining();
	if (remain != 0) return "Unexpected additional block";

	return NULL;
}

/**
 * Get string number \a number.
 * @param number string number to get.
 * @return String corresponding to the number (not owned by the caller, so don't free it).
 */
const char *Language::GetText(StringID number)
{
	if (number < STRING_TABLE_START) {
		switch (number) {
			case STR_NULL:  return NULL;
			case STR_EMPTY: return "";
			default: NOT_REACHED();
		}
	}

	number -= STRING_TABLE_START;
	if (number >= this->num_texts) return "Invalid string number";
	return this->strings[number];
}

/**
 * Initialize language support.
 * @todo Implement me.
 */
void InitLanguage()
{
	Language *lang = new Language;
	const char *msg = lang->Load("../rcd/english.lang");
	if (msg != NULL) {
		fprintf(stderr, "Loading language failed: '%s'\n", msg);
		exit(1);
	}
	_language = lang;
}

/** Clean up the language. */
void UninitLanguage()
{
	if (_language != NULL) delete _language;
	_language = NULL;
}

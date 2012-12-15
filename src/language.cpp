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
#include "video.h"
#include "string_func.h"
#include "dates.h"
#include "money.h"

assert_compile((int)GUI_STRING_TABLE_END < STR_END_FREE_SPACE); ///< Ensure there are not too many gui strings.
assert_compile((int)SHOPS_STRING_TABLE_END < STR_GENERIC_END);  ///< Ensure there are not too many shops strings.

Language _language; ///< Language strings.
StringParameters _str_params; ///< Default string parameters.
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

StringParameters::StringParameters()
{
	this->Clear();
}

/**
 * Mark string parameter \a num as 'unused'.
 * @param num Number of the parameter to set (1-based).
 */
void StringParameters::SetNone(int num)
{
	num--;
	assert(num >= 0 && (uint)num < lengthof(this->parms));
	this->parms[num].parm_type = SPT_NONE;
}

/**
 * Mark string parameter \a num to contain a parameter-less string.
 * @param num Number of the parameter to set (1-based).
 * @param strid String id of the parameter.
 */
void StringParameters::SetStrID(int num, StringID strid)
{
	if (!this->set_mode) this->Clear();

	num--;
	assert(num >= 0 && (uint)num < lengthof(this->parms));
	this->parms[num].parm_type = SPT_STRID;
	this->parms[num].u.str = strid;
}

/**
 * Mark string parameter \a num to contain a number.
 * @param num Number of the parameter to set (1-based).
 * @param number Number value of the parameter.
 */
void StringParameters::SetNumber(int num, int64 number)
{
	if (!this->set_mode) this->Clear();

	num--;
	assert(num >= 0 && (uint)num < lengthof(this->parms));
	this->parms[num].parm_type = SPT_NUMBER;
	this->parms[num].u.number = number;
}

/**
 * Mark string parameter \a num to contain an amount of money.
 * @param num Number of the parameter to set (1-based).
 * @param amount Amount of money to output.
 */
void StringParameters::SetMoney(int num, const Money &amount)
{
	if (!this->set_mode) this->Clear();

	num--;
	assert(num >= 0 && (uint)num < lengthof(this->parms));
	this->parms[num].parm_type = SPT_MONEY;
	this->parms[num].u.number = (int64)amount;
}

/**
 * Mark string parameter \a num to contain a date.
 * @param num Number of the parameter to set (1-based).
 * @param date %Date to output.
 */
void StringParameters::SetDate(int num, const Date &date)
{
	if (!this->set_mode) this->Clear();

	num--;
	assert(num >= 0 && (uint)num < lengthof(this->parms));
	this->parms[num].parm_type = SPT_DATE;
	this->parms[num].u.dmy = date.Compress();
}

/**
 * Mark string parameter \a num to contain a C-style UTF-8 string.
 * @param num Number of the parameter to set (1-based).
 * @param text UTF-8 string. Is not released after use.
 */
void StringParameters::SetUint8(int num, uint8 *text)
{
	if (!this->set_mode) this->Clear();

	num--;
	assert(num >= 0 && (uint)num < lengthof(this->parms));
	this->parms[num].parm_type = SPT_UINT8;
	this->parms[num].u.text = text;
}

/** Clear all data from the parameters. Does not free memory. */
void StringParameters::Clear()
{
	for (uint i = 0; i < lengthof(this->parms); i++) this->SetNone(i + 1);
	this->set_mode = true;
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
 * Copy a source string to the destination, if it fits.
 * @param dest Destination address.
 * @param last Last byte of the destination (not written).
 * @param src Source string.
 * @return Updated destination.
 */
static uint8 *CopyString(uint8 *dest, const uint8 *last, const uint8 *src)
{
	if (src == NULL) return dest;
	while (*src != '\0' && dest < last) {
		*dest = *src;
		dest++;
		src++;
	}
	return dest;
}

/**
 * Draw the string into the supplied buffer.
 * @param strid String number to 'draw'.
 * @param buffer Destination buffer.
 * @param length Length of \a buffer in bytes.
 * @param params String parameter values for the "%n%" patterns.
 */
void DrawText(StringID strid, uint8 *buffer, uint length, StringParameters *params)
{
	static uint8 textbuf[64];

	const uint8 *txt = _language.GetText(strid);
	const uint8 *last = buffer + ((int)length - 1);
	const uint8 *ptr = txt;
	for (;;) {
		while (*ptr != '\0' && *ptr != '%' && buffer < last) {
			*buffer = *ptr;
			buffer++;
			ptr++;
		}
		if (*ptr == '\0' || buffer >= last) break;
		ptr++;
		if (*ptr == '%') {
			*buffer = '%';
			buffer++;
			ptr++;
			continue;
		}
		int n = 0;
		while (*ptr >= '0' && *ptr <= '9') {
			n = n * 10 + *ptr - '0';
			ptr++;
		}
		if (params != NULL && n >= 1 && n <= (int)lengthof(params->parms)) {
			/* Expand parameter 'n-1'. */
			switch (params->parms[n - 1].parm_type) {
				case SPT_NONE:
					buffer = CopyString(buffer, last, (uint8 *)"NONE");
					break;

				case SPT_STRID:
					buffer = CopyString(buffer, last, _language.GetText(params->parms[n - 1].u.str));
					break;

				case SPT_UINT8:
					buffer = CopyString(buffer, last, params->parms[n - 1].u.text);
					break;

				case SPT_NUMBER:
					snprintf((char *)textbuf, lengthof(textbuf), "%lld", params->parms[n - 1].u.number);
					buffer = CopyString(buffer, last, textbuf);
					break;

				case SPT_MONEY:
					snprintf((char *)textbuf, lengthof(textbuf), "%.2f", params->parms[n - 1].u.number / 100.0);
					buffer = CopyString(buffer, last, textbuf);
					break;

				case SPT_DATE: {
					Date d(params->parms[n - 1].u.dmy);
					const uint8 *month = _language.GetText(GetMonthName(d.month));
					snprintf((char *)textbuf, lengthof(textbuf), "%d-%s-%d", d.day, month, d.year);
					buffer = CopyString(buffer, last, textbuf);
					break;
				}

				default: NOT_REACHED();
			}
		}
		while (*ptr != '\0' && *ptr != '%') ptr++; // Skip to the next '%'
		if (*ptr == '\0') break;
		ptr++;
	}
	*buffer = '\0';
	if (params != NULL) params->set_mode = false; // Clean parameters on next Set.
}

/**
 * Get the name of a month or the current month.
 * @param month Month number (1-based). Use \c 0 to get the name of the current month.
 * @return String number containing the month name.
 */
StringID GetMonthName(int month)
{
	static const uint16 month_names[] = {
		GUI_MONTH_JANUARY,
		GUI_MONTH_FEBRUARY,
		GUI_MONTH_MARCH,
		GUI_MONTH_APRIL,
		GUI_MONTH_MAY,
		GUI_MONTH_JUNE,
		GUI_MONTH_JULY,
		GUI_MONTH_AUGUST,
		GUI_MONTH_SEPTEMBER,
		GUI_MONTH_OCTOBER,
		GUI_MONTH_NOVEMBER,
		GUI_MONTH_DECEMBER,
	};

	if (month == 0) month = _date.month;
	assert(month >= 1 && month < 13);
	return month_names[month - 1];
}

/**
 * Get the text-size of a string.
 * @param strid Text to calculate.
 * @param width [out] Resulting width.
 * @param height [out] Resulting height.
 */
void GetTextSize(StringID strid, int *width, int *height)
{
	static uint8 buffer[1024]; // Arbitrary max size.
	DrawText(strid, buffer, lengthof(buffer));
	_video->GetTextSize(buffer, width, height);
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

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
#include "math_func.h"
#include "dates.h"
#include "money.h"

assert_compile((int)GUI_STRING_TABLE_END < STR_END_FREE_SPACE); ///< Ensure there are not too many GUI strings.
assert_compile((int)SHOPS_STRING_TABLE_END < STR_GENERIC_END);  ///< Ensure there are not too many shops strings.

Language _language;                 ///< Language strings.
StringParameters _str_params;       ///< Default string parameters.
int _current_language = LANG_EN_GB; ///< Index of the current translation.

/** Default constructor of a #TextString object. */
TextString::TextString()
{
	this->Clear();
}

/** Drop all strings. Since the text is not owned by the class, no need to free the memory. */
void TextString::Clear()
{
	this->name = nullptr;
	std::fill_n(this->languages, lengthof(this->languages), nullptr);
}

/** Known languages. */
const char * const _lang_names[] = {
	"da_DK", // Danish.
	"de_DE", // German.
	"en_GB", // English (Also the default language).
	"es_ES", // Spanish.
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
void StringParameters::SetUint8(int num, const uint8 *text)
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
	this->Clear();
}

/** Clear all loaded data. */
void Language::Clear()
{
	std::fill_n(this->registered, lengthof(this->registered), nullptr);
	this->first_free = GUI_STRING_TABLE_END;
}

/**
 * Register loaded strings of rides etc. with the language system.
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
	while (*str != nullptr) {
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
	while (*str != nullptr) {
		this->registered[number] = nullptr;
		for (uint i = 0; i < td.string_count; i++) {
			const TextString *ts = td.strings + i;
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
	static const uint8 *default_strings[] = {
		nullptr,                 // STR_NULL
		(const uint8 *)"",    // STR_EMPTY
		(const uint8 *)"%1%", // STR_ARG1
	};

	if (number < lengthof(default_strings)) return default_strings[number];

	if (number < lengthof(this->registered) && this->registered[number] != nullptr) {
		const uint8 *text = this->registered[number]->GetString();
		if (*text == '\0') return (const uint8 *)"<empty text>";
		return text;
	}

	return (const uint8 *)"<Invalid string>";
}

/**
 * Get the (native) name of a language.
 * @param lang_index The language to look in.
 * @return The language name.
 */
const uint8 *Language::GetLanguageName(int lang_index)
{
	assert(lang_index < LANGUAGE_COUNT);
	return this->registered[GUI_LANGUAGE_NAME]->languages[lang_index];
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
	if (src == nullptr) return dest;
	while (*src != '\0' && dest < last) {
		*dest = *src;
		dest++;
		src++;
	}
	return dest;
}

/**
 * Converts a double value into a utf-8 string with the appropriate separators.
 * @param dest a provided buffer to write the output into.
 * @param size the size of the provided buffer.
 * @param amt the double value to be converted.
 */
static void MoneyStrFmt(uint8 *dest, size_t size, double amt)
{
	const uint8 *curr_sym = _language.GetText(GUI_MONEY_CURRENCY_SYMBOL);
	size_t curr_sym_len = StrBytesLength(curr_sym);

	const uint8 *tho_sep  = _language.GetText(GUI_MONEY_THOUSANDS_SEPARATOR);
	size_t tho_sep_len  = StrBytesLength(tho_sep);

	const uint8 *dec_sep  = _language.GetText(GUI_MONEY_DECIMAL_SEPARATOR);
	size_t dec_sep_len  = StrBytesLength(dec_sep);

	char *buf = new char[size];

	/* Convert double into a numeric string (with '.' as the decimal separator). */
	uint len = snprintf(buf, size, "%.2f", amt);

	/* Compute the offset of where we should begin counting for thousand separators. */
	uint comma_start = (len - 3) % 3;

	/* What is the max number of characters we might append in a single loop?
	 * This is used to prevent a potential buffer overflow. */
	/* max_append_size = 'a "special" symbol plus a single digit [0-9]'. */
	int max_append_size = std::max(std::max(curr_sym_len, tho_sep_len), dec_sep_len) + 1;

	uint j = 0;
	uint curpos = 0;

	/* Also has the added bonus of 'removing' the
	 * automagically included '-' symbol by snprintf */
	if (amt < 0) {
		dest[j++] = '-';
		curpos++;
	}

	/* Copy currency symbol next */
	SafeStrncpy(dest + j, curr_sym, curr_sym_len + 1);
	j += curr_sym_len;
	dest[j++] = buf[curpos++];

	for (uint i = curpos; i < len && j + max_append_size < size; i++) {
		if (len - i > 3 && (int)(i - comma_start) % 3 == 0) {
			SafeStrncpy(dest + j, tho_sep, tho_sep_len + 1);
			j += tho_sep_len;
			dest[j++] = buf[i];

		} else if (buf[i] == '.') { // Decimal separator produced by 'snprintf'.
			SafeStrncpy(dest + j, dec_sep, dec_sep_len + 1);
			j += dec_sep_len;

		} else {
			dest[j++] = buf[i];
		}
	}

	assert((size_t)j < size);
	dest[j] = '\0';
	delete[] buf;
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
		if (params != nullptr && n >= 1 && n <= (int)lengthof(params->parms)) {
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
					MoneyStrFmt(textbuf, lengthof(textbuf), params->parms[n - 1].u.number / 100.0);
					buffer = CopyString(buffer, last, textbuf);
					break;

				case SPT_DATE: {
					Date d(params->parms[n - 1].u.dmy);
					buffer = CopyString(buffer, last, GetDateString(d));
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
	if (params != nullptr) params->set_mode = false; // Clean parameters on next Set.
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
 * Convert the date to a Unicode string.
 * @param d %Date to format.
 * @param format C-Style format code.
 * @return The formatted string.
 * @todo Allow variable number of format parameters, e.g. "mm-yy".
 */
const uint8 *GetDateString(const Date &d, const char *format)
{
	static uint8 textbuf[64];
	const uint8 *month = _language.GetText(GetMonthName(d.month));
	snprintf((char *)textbuf, lengthof(textbuf), format, d.day, month, d.year);
	return textbuf;
}

/**
 * Get the maximum size a formatted string can take.
 * @return A 2D size representing the maximum size of the date string.
 * @todo Allow different format of date.
 */
Point32 GetMaxDateSize()
{
	Point32 point;
	point.x = 0;
	point.y = 0;

	Date d;
	for (int i = 1; i < 13; i++) {
		d.month = i;
		int w;
		int h;
		_video->GetTextSize(GetDateString(d), &w, &h);
		point.x = std::max(point.x, w);
		point.y = std::max(point.y, h);
	}
	return point;
}

/**
 * Get size of a formatted money string.
 * @param amount Amount of money.
 * @return A 2D size representing size of the money string.
 */
Point32 GetMoneyStringSize(const Money &amount)
{
	Point32 p;
	uint8 textbuf[64];
	MoneyStrFmt(textbuf, lengthof(textbuf), (int64)amount / 100.0);
	_video->GetTextSize(textbuf, &p.x, &p.y);
	return p;
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

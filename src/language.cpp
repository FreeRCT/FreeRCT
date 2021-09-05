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
assert_compile((int)GENTLE_THRILL_RIDES_STRING_TABLE_END < STR_GENERIC_END);  ///< Ensure there are not too many shops strings.

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
const std::string _lang_names[] = {
	"da_DK", // Danish.
	"de_DE", // German.
	"en_GB", // English (Also the default language).
	"en_US", // English (US).
	"es_ES", // Spanish.
	"fr_FR", // French.
	"nds_DE", // Low German.
	"nl_NL", // Dutch.
	"sv_SE", // Swedish (Sweden)
};

assert_compile(lengthof(_lang_names) == LANGUAGE_COUNT); ///< Ensure number of language matches with array sizes.

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
		int cmp = _lang_names[middle].compare(lang_name);
		if (cmp == 0) return middle; // Jack pot.
		if (cmp < 0) {
			start = middle;
		} else {
			end = middle;
		}
	}
	if (_lang_names[start] == lang_name) return start;
	return -1;
}

/**
 * Get the name of a given language index.
 * @param index Index of the language.
 * @return Name of the language, or \c "" if the index is invalid.
 */
std::string GetLanguageName(const int index)
{
	if (index < 0 || index >= LANGUAGE_COUNT) return std::string();
	return _lang_names[index];
}

/**
 * Try to find a language whose name is similar to the provided name.
 * @param lang_name Name to compare to.
 * @return Most similar language name, or \c "" if no language has a similar name.
 */
std::string GetSimilarLanguage(const std::string& lang_name)
{
	std::string best_match;
	double score = 1.5;  // Arbitrary treshold to suppress random matches.
	for (int i = 0; i < LANGUAGE_COUNT; ++i) {
		const int common_length = std::min(_lang_names[i].size(), lang_name.size());
		double s = common_length - std::max<int>(_lang_names[i].size(), lang_name.size());
		for (int j = 0; j < common_length; ++j) {
			const char c1 = lang_name.at(j);
			const char c2 = _lang_names[i].at(j);
			if (c1 == c2) {
				s++;
			} else if (c1 >= 'a' && c1 <= 'z' && c1 + 'A' - 'a' == c2) {
				s += 0.5;
			} else if (c2 >= 'a' && c2 <= 'z' && c2 + 'A' - 'a' == c1) {
				s += 0.5;
			} else {
				s--;
			}
		}
		s *= 10.0 / common_length;  // Ensure that the name's length does not influence scoring.
		if (s > score) {
			score = s;
			best_match = _lang_names[i];
		}
	}
	return best_match;
}

StringParameters::StringParameters()
{
	this->Clear();
}

/**
 * Ensure that this Parameters object can hold at least a certain number of parameters.
 * @param num Minimum number of parameters.
 */
void StringParameters::ReserveCapacity(const int num)
{
	assert(num > 0);
	if (!this->set_mode) this->Clear();
	if (this->parms.size() < num) this->parms.resize(num);
}

/**
 * Mark string parameter \a num as 'unused'.
 * @param num Number of the parameter to set (1-based).
 */
void StringParameters::SetNone(int num)
{
	this->ReserveCapacity(num);
	num--;
	this->parms[num].parm_type = SPT_NONE;
}

/**
 * Mark string parameter \a num to contain a parameter-less string.
 * @param num Number of the parameter to set (1-based).
 * @param strid String id of the parameter.
 */
void StringParameters::SetStrID(int num, StringID strid)
{
	this->ReserveCapacity(num);
	num--;
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
	this->ReserveCapacity(num);
	num--;
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
	this->ReserveCapacity(num);
	num--;
	this->parms[num].parm_type = SPT_MONEY;
	this->parms[num].u.number = (int64)amount;
}

/**
 * Mark string parameter \a num to contain a temperature.
 * @param num Number of the parameter to set (1-based).
 * @param value Temperature value in 1/10 degrees Celcius.
 */
void StringParameters::SetTemperature(int num, int value)
{
	this->ReserveCapacity(num);
	num--;
	this->parms[num].parm_type = SPT_TEMPERATURE;
	this->parms[num].u.number = value;
}

/**
 * Mark string parameter \a num to contain a date.
 * @param num Number of the parameter to set (1-based).
 * @param date %Date to output.
 */
void StringParameters::SetDate(int num, const Date &date)
{
	this->ReserveCapacity(num);
	num--;
	this->parms[num].parm_type = SPT_DATE;
	this->parms[num].u.dmy = date.Compress();
}

/**
 * Mark string parameter \a num to contain a UTF-8 string.
 * @param num Number of the parameter to set (1-based).
 * @param text UTF-8 string.
 */
void StringParameters::SetText(int num, const std::string &text)
{
	this->ReserveCapacity(num);
	num--;
	this->parms[num].parm_type = SPT_TEXT;
	this->parms[num].text = text;
}

/** Clear all data from the parameters. Does not free memory. */
void StringParameters::Clear()
{
	this->parms.clear();
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
			const TextString *ts = td.strings.get() + i;
			if (strcmp(ts->name, *str) == 0) {
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
std::string Language::GetText(StringID number)
{
	static const std::string default_strings[] = {
		"",     // STR_NULL
		"%1%",  // STR_ARG1
	};

	if (number < lengthof(default_strings)) return default_strings[number];

	if (number < lengthof(this->registered) && this->registered[number] != nullptr) {
		const std::string &text = this->registered[number]->GetString();
		if (text.empty()) return "<empty text>";
		return text;
	}

	return "<Invalid string>";
}

/**
 * Get the (native) name of a language.
 * @param lang_index The language to look in.
 * @return The language name.
 */
std::string Language::GetLanguageName(int lang_index)
{
	assert(lang_index < LANGUAGE_COUNT);
	return this->registered[GUI_LANGUAGE_NAME]->languages[lang_index];
}

/**
 * Converts a double value into a utf-8 string with the appropriate separators.
 * @param dest [out] A provided buffer to write the output into.
 * @param size The size of the provided buffer.
 * @param amt The double value to be converted.
 */
static void MoneyStrFmt(char *dest, size_t size, double amt)
{
	const std::string curr_sym = _language.GetText(GUI_MONEY_CURRENCY_SYMBOL);
	size_t curr_sym_len = curr_sym.size();

	const std::string tho_sep  = _language.GetText(GUI_MONEY_THOUSANDS_SEPARATOR);
	size_t tho_sep_len  = tho_sep.size();

	const std::string dec_sep  = _language.GetText(GUI_MONEY_DECIMAL_SEPARATOR);
	size_t dec_sep_len  = dec_sep.size();

	std::unique_ptr<char[]> buf(new char[size]);

	/* Convert double into a numeric string (with '.' as the decimal separator). */
	uint len = snprintf(buf.get(), size, "%.2f", amt);

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
	strncpy(dest + j, curr_sym.c_str(), curr_sym_len + 1);
	j += curr_sym_len;
	dest[j++] = buf[curpos++];

	for (uint i = curpos; i < len && j + max_append_size < size; i++) {
		if (len - i > 3 && (int)(i - comma_start) % 3 == 0) {
			strncpy(dest + j, tho_sep.c_str(), tho_sep_len + 1);
			j += tho_sep_len;
			dest[j++] = buf[i];

		} else if (buf[i] == '.') { // Decimal separator produced by 'snprintf'.
			strncpy(dest + j, dec_sep.c_str(), dec_sep_len + 1);
			j += dec_sep_len;

		} else {
			dest[j++] = buf[i];
		}
	}

	assert((size_t)j < size);
	dest[j] = '\0';
}

/**
 * Convert a temperature in 1/10 degrees Celcius to text.
 * @param temp Temperature in 1/10 degrees Celcius to convert.
 * @return Formatted text.
 */
static std::string TemperatureStrFormat(int temp)
{
	temp = ((temp < 0) ? temp - 5 : temp + 5) / 10; // Round to degrees Celcius.
	static char buffer[64];
	int len = snprintf(buffer, lengthof(buffer), "%d \u2103", temp);  // " " + degrees Celcius, U+2103
	return buffer;
}

/**
 * Draw the string into the supplied buffer.
 * @param strid String number to 'draw'.
 * @param params String parameter values for the "%n%" patterns.
 * @return The formatted text.
 */
std::string DrawText(StringID strid, StringParameters *params)
{
	static char textbuf[64];
	std::string buffer;

	const std::string txt = _language.GetText(strid);
	const char *ptr = txt.c_str();
	for (;;) {
		while (*ptr != '\0' && *ptr != '%') {
			buffer.push_back(*ptr);
			ptr++;
		}
		if (*ptr == '\0') break;
		ptr++;
		if (*ptr == '%') {
			buffer.push_back(*ptr);
			ptr++;
			continue;
		}
		int n = 0;
		while (*ptr >= '0' && *ptr <= '9') {
			n = n * 10 + *ptr - '0';
			ptr++;
		}
		if (params != nullptr && n >= 1 && n <= params->parms.size()) {
			/* Expand parameter 'n-1'. */
			switch (params->parms[n - 1].parm_type) {
				case SPT_NONE:
					buffer += "NONE";
					break;

				case SPT_STRID:
					buffer += _language.GetText(params->parms[n - 1].u.str);
					break;

				case SPT_TEXT:
					buffer += params->parms[n - 1].text;
					break;

				case SPT_NUMBER:
					snprintf(textbuf, lengthof(textbuf), "%lld", params->parms[n - 1].u.number);
					buffer += textbuf;
					break;

				case SPT_MONEY:
					MoneyStrFmt(textbuf, lengthof(textbuf), params->parms[n - 1].u.number / 100.0);
					buffer += textbuf;
					break;

				case SPT_TEMPERATURE:
					buffer += TemperatureStrFormat(params->parms[n - 1].u.number);
					break;

				case SPT_DATE:
					buffer += GetDateString(Date(params->parms[n - 1].u.dmy));
					break;

				default: NOT_REACHED();
			}
		}
		while (*ptr != '\0' && *ptr != '%') ptr++; // Skip to the next '%'
		if (*ptr == '\0') break;
		ptr++;
	}
	if (params != nullptr) params->set_mode = false; // Clean parameters on next Set.
	return buffer;
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
	_video.GetTextSize(DrawText(strid), width, height);
}

/**
 * Convert the date to a Unicode string.
 * @param d %Date to format.
 * @param format C-Style format code.
 * @return The formatted string.
 * @todo Allow variable number of format parameters, e.g. "mm-yy".
 */
std::string GetDateString(const Date &d, const char *format)
{
	static char textbuf[64];
	const std::string &month = _language.GetText(GetMonthName(d.month));
	snprintf(textbuf, lengthof(textbuf), format, d.day, month.c_str(), d.year);
	return textbuf;
}

/**
 * Get the maximum size a formatted string can take.
 * @return A 2D size representing the maximum size of the date string.
 * @todo Allow different format of date.
 */
Point32 GetMaxDateSize()
{
	Point32 point(0, 0);

	Date d;
	for (int i = 1; i < 13; i++) {
		d.month = i;
		int w;
		int h;
		_video.GetTextSize(GetDateString(d), &w, &h);
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
	char textbuf[64];
	MoneyStrFmt(textbuf, lengthof(textbuf), (int64)amount / 100.0);
	Point32 p;
	_video.GetTextSize(textbuf, &p.x, &p.y);
	return p;
}

/**
 * Attempt to set FreeRCT's language according to the value of an environment variable.
 * @param lang Content of a language-related environment variable.
 * @return A supported language was detected and selected.
 */
static bool TrySetLanguage(std::string lang) {
	/* Typical system language strings may be "de_DE.UTF-8" or "nds:de_DE:en_GB:en". */
	int id = GetLanguageIndex(lang.c_str());
	if (id >= 0) {
		_current_language = id;
		return true;
	}

	for (;;) {
		size_t pos = lang.find(':');
		std::string name;
		bool single_language = false;
		if (pos == std::string::npos) {
			name = lang;
			single_language = true;
		} else {
			name = lang.substr(0, pos);
			lang = lang.substr(pos + 1);
		}

		pos = name.find('.');
		if (pos != std::string::npos) name = name.substr(0, pos);
		id = GetLanguageIndex(name.c_str());
		if (id >= 0) {
			_current_language = id;
			return true;
		}

		if (single_language) break;
	}
	return false;
}

/**
 * Initialize language support.
 */
void InitLanguage()
{
	for (auto& var : {"FREERCT_LANG", "LANG", "LANGUAGE"}) {
		const char *environment_variable = getenv(var);
		if (environment_variable != nullptr && TrySetLanguage(environment_variable)) break;
	}
}

/** Clean up the language. */
void UninitLanguage()
{
	_language.Clear();
}

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
#include "math_func.h"
#include "dates.h"
#include "money.h"

assert_compile((int)GUI_STRING_TABLE_END < STR_END_FREE_SPACE); ///< Ensure there are not too many GUI strings.
assert_compile((int)SHOPS_STRING_TABLE_END < STR_GENERIC_END);  ///< Ensure there are not too many shops strings.
assert_compile((int)GENTLE_THRILL_RIDES_STRING_TABLE_END < STR_GENERIC_END);  ///< Ensure there are not too many shops strings.

LanguageManager _language;                ///< Language manager.
StringParameters _str_params;             ///< Default string parameters.
int _current_language = SOURCE_LANGUAGE;  ///< Index of the current translation.

/**
 * Get the name of a given language index.
 * @param index Index of the language.
 * @return Name of the language, or \c "" if the index is invalid.
 */
std::string GetLanguageName(const int index)
{
	if (index < 0 || index >= LANGUAGE_COUNT) return std::string();
	return _all_languages[index].name;
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
		const int common_length = std::min(strlen(_all_languages[i].name), lang_name.size());
		double s = common_length - std::max<int>(strlen(_all_languages[i].name), lang_name.size());
		for (int j = 0; j < common_length; ++j) {
			const char c1 = lang_name.at(j);
			const char c2 = _all_languages[i].name[j];
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
			best_match = _all_languages[i].name;
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
 * @param num_params Minimum number of parameters.
 * @note Only call this function if you actually need at least \c 1 parameter.
 */
void StringParameters::ReserveCapacity(const int num_params)
{
	assert(num_params > 0 && num_params < 20);  // Arbitrary upper bound.
	if (!this->set_mode) this->Clear();
	if (static_cast<int>(this->parms.size()) < num_params) this->parms.resize(num_params);
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
 * Mark string parameter \a num to contain a number, and use this number to determine the string's plural form.
 * @param num Number of the parameter to set (1-based).
 * @param number Number value of the parameter.
 */
void StringParameters::SetNumberAndPlural(int num, int64 number)
{
	this->SetNumber(num, number);
	this->pluralize_count = number;
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
	this->pluralize_count = 1;
}

LanguageBundle::LanguageBundle()
{
	this->Clear();
}

/** Clear all loaded data. */
void LanguageBundle::Clear()
{
	this->values.clear();
	this->metadata = nullptr;
}

/**
 * Get the correct plural form for string number \a number.
 * @param number string number to get.
 * @param count The value to use for selecting the appropriate plural form.
 * @return String corresponding to the number (not owned by the caller, so don't free it).
 */
const char *LanguageBundle::GetPlural(StringID number, int64 count) const
{
	if (number >= this->values.size() || this->values.at(number).empty()) return nullptr;
	int plural = this->metadata->PluralRule(count);
	return this->values.at(number).at(plural);
}

/**
 * Get the singular form for string number \a number.
 * @param number string number to get.
 * @return String corresponding to the number (not owned by the caller, so don't free it).
 */
const char *LanguageBundle::GetSgText(StringID number) const
{
	return this->GetPlural(number, 1);
}

LanguageManager::LanguageManager()
{
	this->Clear();
}

/** Clear all loaded data. */
void LanguageManager::Clear()
{
	for (LanguageBundle &l : this->languages) l.Clear();
	this->free_index = GUI_STRING_TABLE_END;
}

/**
 * Register loaded strings of rides etc. with the language system.
 * @param td Text data wrapper containing the loaded strings.
 * @param names Array of names to register.
 * @param base Base address of the strings.
 * @return Base offset for the registered strings. Add the index value of the \a names table to get the real string number.
 */
uint16 LanguageManager::RegisterStrings(const TextData &td, const char * const names[], uint16 base)
{
	if (base == STR_GENERIC_END) {
		base = this->free_index;
		this->free_index += td.string_count;
		if (this->free_index >= STR_END_FREE_SPACE) {
			error("Not enough space to store strings.\n");
		}
	} else {
		assert(base + td.string_count < this->free_index);
	}

	const size_t old_size = this->string_names.size();
	if (base + td.string_count > old_size) {
		this->string_names.resize(base + td.string_count);
		for (LanguageBundle &l : this->languages) {
			assert(l.values.size() == old_size);
			l.values.resize(base + td.string_count);
		}
	}

	/* Names and text strings are not in the same order. */
	std::map<std::string, uint> name_textstring_lookup;
	for (uint i = 0; i < td.string_count; ++i) {
		name_textstring_lookup[td.strings[i].name] = i;
	}
	for (uint i = 0; i < td.string_count; ++i) {
		assert(names[i] != nullptr && *names[i] != '\0');
		this->string_names.at(base + i) = names[i];

		const uint text_index = name_textstring_lookup.at(names[i]);
		for (int l = 0; l < LANGUAGE_COUNT; ++l) {
			this->languages[l].values.at(base + i) = td.strings[text_index].languages[l];
			this->languages[l].values.at(base + i).shrink_to_fit();
		}
	}

	return base;
}

/**
 * Get the correct plural form for string number \a number.
 * @param number string number to get.
 * @param count The value to use for selecting the appropriate plural form.
 * @return String corresponding to the number (not owned by the caller, so don't free it).
 */
std::string LanguageManager::GetPlural(StringID number, int64 count) const
{
	static const std::string default_strings[] = {
		"",     // STR_NULL
		"%1%",  // STR_ARG1
	};

	if (number < lengthof(default_strings)) return default_strings[number];

	if (_current_language != SOURCE_LANGUAGE) {
		const char *str = languages[_current_language].GetPlural(number, count);
		if (str != nullptr) return *str != '\0' ? str : "<empty translation>";
	}

	const char *str = languages[SOURCE_LANGUAGE].GetPlural(number, count);
	if (str != nullptr) return *str != '\0' ? str : "<empty string>";

	return "<invalid string>";
}

/**
 * Get the singular form for string number \a number.
 * @param number string number to get.
 * @return String corresponding to the number (not owned by the caller, so don't free it).
 */
std::string LanguageManager::GetSgText(StringID number) const
{
	return this->GetPlural(number, 1);
}

/**
 * Get the (native) name of a language.
 * @param lang_index The language to look in.
 * @return The language name.
 */
std::string LanguageManager::GetLanguageName(int lang_index) const
{
	return this->languages[lang_index].GetSgText(GUI_LANGUAGE_NAME);
}

/**
 * Look up the name of a string.
 * @param number Number of the string to look up.
 * @return The string's name.
 */
const char *LanguageManager::GetStringName(StringID number) const
{
	return this->string_names.at(number);
}

/**
 * Check that all meta info is present and sane,
 * and that all pluralized strings match their language's specifications,
 * and cache meta info for later access.
 */
void LanguageManager::InitMetaInfo()
{
	for (int i = 0; i < LANGUAGE_COUNT; ++i) this->languages[i].InitMetaInfo(i);
}

/**
 * Check that all meta info is present and sane,
 * and that all pluralized strings match their language's specifications,
 * and cache meta info for later access.
 * @param index Language index of this language.
 */
void LanguageBundle::InitMetaInfo(int index)
{
	this->metadata = &_all_languages[index];

	const uint32 nrstrings = this->values.size();
	for (uint32 i = 0; i < nrstrings; ++i) {
		int size = this->values.at(i).size();
		if (_language.GetStringName(i) == nullptr && size != 0) {
			error("Language %s has a string at undefined index %u.\n",
					this->metadata->name, i);
		}
		if (size > 1 && size != this->metadata->nplurals) {
			error("Language %s has %d plurals, but string '%s' has %d.\n",
					this->metadata->name, this->metadata->nplurals, _language.GetStringName(i), size);
		}
	}
}

/**
 * Converts a double value into a utf-8 string with the appropriate separators.
 * @param dest [out] A provided buffer to write the output into.
 * @param size The size of the provided buffer.
 * @param amt The double value to be converted.
 */
static void MoneyStrFmt(char *dest, size_t size, double amt)
{
	const std::string curr_sym = _language.GetSgText(GUI_MONEY_CURRENCY_SYMBOL);
	size_t curr_sym_len = curr_sym.size();

	const std::string tho_sep  = _language.GetSgText(GUI_MONEY_THOUSANDS_SEPARATOR);
	size_t tho_sep_len  = tho_sep.size();

	const std::string dec_sep  = _language.GetSgText(GUI_MONEY_DECIMAL_SEPARATOR);
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
	snprintf(buffer, lengthof(buffer), "%d \u2103", temp);  // " " + degrees Celcius, U+2103
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

	const std::string txt = _language.GetPlural(strid, params == nullptr ? 1 : params->pluralize_count);
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
		if (params != nullptr && n >= 1 && n <= static_cast<int>(params->parms.size())) {
			/* Expand parameter 'n-1'. */
			switch (params->parms[n - 1].parm_type) {
				case SPT_NONE:
					buffer += "NONE";
					break;

				case SPT_STRID:
					buffer += _language.GetSgText(params->parms[n - 1].u.str);
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
	const std::string &month = _language.GetSgText(GetMonthName(d.month));
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
	_language.InitMetaInfo();
}

/** Clean up the language. */
void UninitLanguage()
{
	_language.Clear();
}

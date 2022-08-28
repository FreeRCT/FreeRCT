/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file language.h %Language support. */

#ifndef LANGUAGE_H
#define LANGUAGE_H

#include "geometry.h"
#include "language_definitions.h"
#include "string_func.h"

class TextData;
class Money;
class Date;

extern int _current_language;

/**
 * Table of string-parts in the game.
 *
 * The largest part of the string space is allocated for 'simple' strings that exist only once, mostly in the GUI.
 *
 * For rides, such as shops, this does not work since there are several shop types loaded.
 * Instead, such strings are allocated at StringTable::STR_GENERIC_SHOP_START, while the
 * strings of each type are elsewhere created. By using the
 * RideType::GetString, the real string number of the queried shop type is returned.
 */
enum StringTable {
	STR_NULL = 0, ///< \c nullptr string.
	STR_ARG1,     ///< Argument 1 \c "%1%".

	STR_GUI_START,  ///< Start of the GUI strings.

	/* After the GUI strings come the other registered strings. */

	STR_END_FREE_SPACE = 0xF800,

	/**
	 * Generic strings for ride entrances and exits.
	 */
	STR_GENERIC_ENTRANCE_EXIT_START = STR_END_FREE_SPACE,

	/**
	 * Generic strings for scenery items.
	 */
	STR_GENERIC_SCENERY_START = STR_GENERIC_ENTRANCE_EXIT_START + 64,

	/**
	 * Generic shop strings, translated to 'real' string numbers by each shop type object by means of the RideType::GetString function.
	 */
	STR_GENERIC_SHOP_START = STR_GENERIC_SCENERY_START + 256,

	/**
	 * Generic coaster strings, translated to 'real' string numbers by each coaster type object by means of the RideType::GetString function.
	 */
	STR_GENERIC_COASTER_START = STR_GENERIC_SHOP_START + 256,

	/**
	 * Generic gentle/thrill ride strings, translated to 'real' string numbers by each gentle/thrill type object by means of the RideType::GetString function.
	 */
	STR_GENERIC_GENTLE_THRILL_RIDES_START = STR_GENERIC_COASTER_START + 256,

	STR_GENERIC_END = 0xFFFF,

	STR_INVALID = STR_GENERIC_END, ///< Invalid string.
};

#include "generated/gui_strings.h"
#include "generated/shops_strings.h"
#include "generated/gentle_thrill_rides_strings.h"
#include "generated/coasters_strings.h"

typedef uint16 StringID; ///< Type of a string value.

/** Types of parameters for string parameters. */
enum StringParamType {
	SPT_NONE,   ///< Parameter contains nothing (and should not be used thus).
	SPT_STRID,  ///< Parameter is another #StringID.
	SPT_NUMBER, ///< Parameter is a number.
	SPT_MONEY,  ///< Parameter is an amount of money.
	SPT_DATE,   ///< Parameter is a date.
	SPT_TEMPERATURE, ///< Parameter is a temperature.
	SPT_TEXT,  ///< Parameter is a text string.
};

/** Data of one string parameter. */
struct StringParameterData {
	uint8 parm_type; ///< Type of the parameter. @see StringParamType
	union {
		StringID str;      ///< String number.
		uint32 dmy;        ///< Day/month/year.
		int64 number;      ///< Signed number, temperature, or money amount.
	} u; ///< Data of the parameter.
	std::string text;  ///< Text content (note: unions can't contain std::string).
};

/** All string parameters. */
struct StringParameters {
	StringParameters();

	void SetNone(int num);
	void SetStrID(int num, StringID strid);
	void SetNumber(int num, int64 number);
	void SetNumberAndPlural(int num, int64 number);
	void SetMoney(int num, const Money &amount);
	void SetDate(int num, const Date &date);
	void SetTemperature(int num, int value);
	void SetText(int num, const std::string &text);
	void ReserveCapacity(int num_params);

	void Clear();

	bool set_mode; ///< When not in set-mode, all parameters are cleared on first use of a Set function.
	std::vector<StringParameterData> parms; ///< Parameters of the string.
	int64 pluralize_count;                  ///< Value to use for selecting the plural form.
};

/** A string in one language with all its plural forms. */
using PluralizedString = std::vector<const char*>;

/** Contains all strings and the meta-information for one specific language. */
class LanguageBundle {
public:
	LanguageBundle();
	void Clear();
	void InitMetaInfo(int index);

	const char *GetSgText(StringID number) const;
	const char *GetPlural(StringID number, int64 count) const;

	std::vector<PluralizedString> values;  ///< Every known string with all its plural forms, indexed by #StringID.
	                                       ///< Strings without a translation are \c nullptr. Memory is not owned.
	const LanguageDefinition *metadata;    ///< This language's metadata.
};

/** Class for retrieving language strings. */
class LanguageManager {
public:
	LanguageManager();

	void Clear();

	uint16 RegisterStrings(const TextData &td, const char * const names[], uint16 base = STR_GENERIC_END);
	void InitMetaInfo();

	std::string GetSgText(StringID number) const;
	std::string GetPlural(StringID number, int64 count) const;
	std::string GetLanguageName(int lang_index) const;
	const char *GetStringName(StringID number) const;

private:
	LanguageBundle languages[LANGUAGE_COUNT];  ///< All registered languages.
	std::vector<const char*> string_names;     ///< Names of every string, indexed by #StringID. Memory is not owned.
	uint16 free_index;                         ///< Next free index for string storage.
};

std::string GetLanguageName(int index);
std::string GetSimilarLanguage(const std::string&);
void InitLanguage();
void UninitLanguage();

StringID GetMonthName(int month = 0);
void GetTextSize(StringID num, int *width, int *height);

extern LanguageManager _language;
extern StringParameters _str_params;

std::string DrawText(StringID num, StringParameters *params = &_str_params);

std::string GetDateString(const Date &d);
Point32 GetMaxDateSize();
Point32 GetMoneyStringSize(const Money &amount);

/**
 * Convenience wrapper around the snprintf function.
 * @param format Format string with printf-style placeholders.
 * @param args Formatting arguments.
 * @return Formatted text.
 * @note When possible, use #DrawText instead.
 */
template<typename... Args>
std::string Format(const std::string &format, Args... args)
{
	static char buffer[2048];
	snprintf(buffer, lengthof(buffer), format.c_str(), args...);
	return buffer;
}

/**
 * Convenience wrapper around the snprintf function.
 * @param format #StringID of the translatable string with printf-style placeholders.
 * @param args Formatting arguments.
 * @return Formatted text.
 * @note When possible, use #DrawText instead.
 */
template<typename... Args>
std::string Format(StringID format, Args... args)
{
	return Format(_language.GetSgText(format), args...);
}

#endif

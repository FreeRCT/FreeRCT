/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file language_definitions.h Definitions of known languages. */

#include <string>
#include <string>

#include "stdafx.h"

/** The standard plural rule for English and many other languages: 1 is singular, everything else is plural. */
inline int plural_rule_standard(int64 amount)
{
	return amount == 1 ? 0 : 1;
}

/** The french plural rule: Singular for 0 and 1, plural for everything else. */
inline int plural_rule_french(int64 amount)
{
	return amount > 1 ? 1 : 0;
}

/** Information about a language. */
struct LanguageDefinition {
	constexpr LanguageDefinition(const char *n, int p, int(*r)(int64)) : name(n), nplurals(p), PluralRule(r) {}

	const char *name;         ///< ISO name of the language, e.g. \c "en_GB".
	int nplurals;             ///< Number of plural forms in the language.
	int(*PluralRule)(int64);  ///< Plural rule functor for the language.
};

/** All languages supported by FreeRCT. */
constexpr const LanguageDefinition _all_languages[] = {
	/* This must always be in alphabetical order! */
	LanguageDefinition("da_DK", 2, &plural_rule_standard),
	LanguageDefinition("de_DE", 2, &plural_rule_standard),
	LanguageDefinition("en_GB", 2, &plural_rule_standard),
	LanguageDefinition("en_US", 2, &plural_rule_standard),
	LanguageDefinition("es_ES", 2, &plural_rule_standard),
	LanguageDefinition("fr_FR", 2, &plural_rule_french),
	LanguageDefinition("nds_DE", 2, &plural_rule_standard),
	LanguageDefinition("nl_NL", 2, &plural_rule_standard),
	LanguageDefinition("sv_SE", 2, &plural_rule_standard),
};

int GetLanguageIndex(const std::string &name);

constexpr const int LANGUAGE_COUNT = lengthof(_all_languages);  ///< Number of supported languages.
static const int SOURCE_LANGUAGE = GetLanguageIndex("en_GB");   ///< Source language of the program.

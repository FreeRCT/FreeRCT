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

/** The one plural rule: There's only one form. */
inline int plural_rule_one(int64 /*amount*/)
{
	return 0;
}

/** A font set to use to display text in a language. */
struct FontSet {
	explicit FontSet(std::vector<std::pair<uint32, uint32>> ranges) : codepoint_ranges(ranges) {}

	std::string font_path;                                    ///< File path to load the font from.
	int font_size = -1;                                       ///< Desired font size.
	std::vector<std::pair<uint32, uint32>> codepoint_ranges;  ///< Ranges of Unicode codepoints this font can display.
};
extern FontSet FONT_LATIN;  ///< Font for languages using Latin script and its derivatives.
extern FontSet FONT_CJK;    ///< Font for Chinese, Japanese, and Korean text.

/** Information about a language. */
struct LanguageDefinition {
	constexpr LanguageDefinition(const char *n, int p, int(*r)(int64), const FontSet *f) : name(n), nplurals(p), PluralRule(r), font(f) {}

	const char *name;         ///< ISO name of the language, e.g. \c "en_GB".
	int nplurals;             ///< Number of plural forms in the language.
	int(*PluralRule)(int64);  ///< Plural rule functor for the language.
	const FontSet *font;      ///< The font to use for displaying text in this language.
};

/** All languages supported by FreeRCT. */
constexpr const LanguageDefinition _all_languages[] = {
	/* This must always be in alphabetical order! */
	LanguageDefinition("da_DK", 2, &plural_rule_standard, &FONT_LATIN),
	LanguageDefinition("de_DE", 2, &plural_rule_standard, &FONT_LATIN),
	LanguageDefinition("en_GB", 2, &plural_rule_standard, &FONT_LATIN),
	LanguageDefinition("en_US", 2, &plural_rule_standard, &FONT_LATIN),
	LanguageDefinition("es_ES", 2, &plural_rule_standard, &FONT_LATIN),
	LanguageDefinition("fr_FR", 2, &plural_rule_french, &FONT_LATIN),
	LanguageDefinition("nds_DE", 2, &plural_rule_standard, &FONT_LATIN),
	LanguageDefinition("nl_NL", 2, &plural_rule_standard, &FONT_LATIN),
	LanguageDefinition("pt_BR", 2, &plural_rule_standard, &FONT_LATIN),
	LanguageDefinition("sv_SE", 2, &plural_rule_standard, &FONT_LATIN),
	/* \todo Our file format and Weblate integration currently require exactly 2 plural forms. Actually Chinese has only one. */
	LanguageDefinition("zh_Hant", 2, &plural_rule_one, &FONT_CJK),
};

int GetLanguageIndex(const std::string &name);

constexpr const int LANGUAGE_COUNT = lengthof(_all_languages);  ///< Number of supported languages.
static const int SOURCE_LANGUAGE = GetLanguageIndex("en_GB");   ///< Source language of the program.

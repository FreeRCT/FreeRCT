/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file string_storage.cpp Code of the translation storage. */

#include <fstream>
#include <set>
#include <string>

#include "../stdafx.h"
#include "../string_func.h"
#include "ast.h"
#include "nodes.h"
#include "string_storage.h"

StringsStorage _strings_storage; ///< Storage of all (translated) strings by key.

/**
 * Retrieve the stored bundle of string by key name.
 * @param key Key to retrieve.
 * @return Pointer to the bundle, or \c nullptr if not found.
 */
const StringBundle *StringsStorage::GetBundle(const std::string &key)
{
	auto iter = this->bundles.find(key);
	if (iter == this->bundles.end()) return nullptr;
	return &iter->second;
}

/**
 * Parse the provided YAML file and store its translations.
 * @param filename File to read from.
 */
void StringsStorage::ReadFromYAML(const char *filename)
{
	std::fstream stream(filename, std::fstream::in);
	if (!stream.is_open()) {
		fprintf(stderr, "Failed to read from %s\n", filename);
		exit(1);
	}

	using PluralForm = std::pair<std::string, Position>;
	using PluralizedString = std::map<std::string, PluralForm>;
	using BundleContent = std::map<std::string, PluralizedString>;
	std::map<std::string, BundleContent> results;

	std::vector<std::string> nesting;
	std::string line;
	int32 line_number = 0;

#define SYNTAX_ERROR_VL(line, msg, ...) do {  \
	fprintf(stderr, "YAML syntax error at %s:%d: " msg "\n", filename, line, __VA_ARGS__);  \
	exit(1);  \
} while (false)
#define SYNTAX_ERROR_L(line, msg) do {  \
	fprintf(stderr, "YAML syntax error at %s:%d: " msg "\n", filename, line);  \
	exit(1);  \
} while (false)
#define SYNTAX_ERROR_V(...) SYNTAX_ERROR_VL(line_number, __VA_ARGS__)
#define SYNTAX_ERROR(msg) SYNTAX_ERROR_L(line_number, msg)

	while (stream.peek() != EOF) {
		++line_number;
		std::getline(stream, line);
		uint32 nr_chars = line.size();
		int pos = 0;

		/* Skip over leading whitespace and count how deeply the string is nested. */
		uint32 nesting_depth = 0;
		for (bool endloop = false; !endloop && pos < nr_chars; ++pos) {
			switch (line[pos]) {
				case ' ':
					++nesting_depth;
					break;
				case '\t':
					nesting_depth += 8;
					break;
				default:
					--pos;
					endloop = true;
					break;
			}
		}

		if (pos >= nr_chars || line[pos] == '#') continue;  // Empty or whitespace-only line or comment.

		/* Read the YAML key in this line. */
		std::string linekey;
		for (;; ++pos) {
			if (pos >= nr_chars) SYNTAX_ERROR("Unterminated line after identifier");
			if (line[pos] == '_' || line[pos] == '-' ||
					(line[pos] >= 'A' && line[pos] <= 'Z') ||
					(line[pos] >= 'a' && line[pos] <= 'z') ||
					(line[pos] >= '0' && line[pos] <= '9')) {
				linekey += line[pos];
				continue;
			}
			if (line[pos] != ':') SYNTAX_ERROR_V("Invalid identifier character '%c'", line[pos]);
			if (linekey.empty()) SYNTAX_ERROR("Empty identifier");
			++pos;
			break;
		}

		/* Skip over whitespace. */
		for (; pos < nr_chars && (line[pos] == ' ' || line[pos] == '\t'); ++pos);

		if (pos >= nr_chars || line[pos] == '#')  {
			/* No value in this line. Store the key. */
			nesting.resize(nesting_depth + 1);
			nesting.back() = linekey;
			continue;
		}

		if (nesting.empty()) SYNTAX_ERROR("Top-level values are forbidden");
		if (nesting.at(0) != "strings") SYNTAX_ERROR("All strings must be in the 'strings' namespace");

		std::string value;
		if (line[pos] != '\"' && (line[pos] != '\'')) {
			/* Parse the line verbatim and trim it. */
			value = line.substr(pos);
			while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) value.pop_back();
		} else {
			/* Parse a quoted value. */
			const char delimiter = line[pos];
			++pos;
			for (;; ++pos) {
				if (pos >= nr_chars) SYNTAX_ERROR("Unterminated line in quoted value");
				if (line[pos] == delimiter) break;  // End of quoted string.
				if (line[pos] != '\\') {
					value.push_back(line[pos]);
					continue;
				}

				/* Backslash-escaped character. */
				++pos;
				if (pos < nr_chars) {
					switch (line[pos]) {
						case 'n':
							value.push_back('\n');
							break;
						case 't':
							value.push_back('\t');
							break;
						case ' ':
							value.push_back(' ');
							break;
						case '\"':
							value.push_back('\"');
							break;
						case '\'':
							value.push_back('\'');
							break;
						default:
							SYNTAX_ERROR_V("Invalid escape character '%c'", line[pos]);
					}
					continue;
				}

				/* The backslash escapes a line break. Load the next line and continue reading (discarding leading whitespace). */
				if (stream.peek() == EOF) SYNTAX_ERROR("Unterminated string at end of file");
				++line_number;
				std::getline(stream, line);
				nr_chars = line.size();
				pos = 0;
				for (; pos < nr_chars && (line[pos] == ' ' || line[pos] == '\t'); ++pos);
				--pos;
			}

			/* Check that the line ends after the closing quote. */
			++pos;
			for (; pos < nr_chars; ++pos) {
				if (line[pos] == '#') break;
				if (line[pos] != ' ' && line[pos] != '\t') SYNTAX_ERROR("Additional data after end of string");
			}
		}

		/* Extract the actual key inheritance chain. */
		std::vector<std::string> sanitized_key;
		size_t sanitized_key_len = 0;
		nesting_depth = std::min<size_t>(nesting_depth, nesting.size());
		for (size_t i = 1; i < nesting_depth; ++i) {
			if (!nesting.at(i).empty()) {
				sanitized_key.push_back(nesting.at(i));
				++sanitized_key_len;
			}
		}
		if (sanitized_key_len == 0) SYNTAX_ERROR("Key not nested deply enough");
		if (sanitized_key_len > 2) SYNTAX_ERROR("Key nested too deply");
		sanitized_key.push_back(linekey);

		std::string &bundle_key = sanitized_key.at(0);
		std::string &key_in_bundle = sanitized_key.at(1);
		std::pair<std::string, Position> pair_to_emplace(value, Position(filename, line_number));

		PluralizedString &plurals_map = results[bundle_key][key_in_bundle];
		if (sanitized_key_len == 1) {
			/* Ordinary string. */
			if (!plurals_map.empty()) SYNTAX_ERROR_V("Duplicate key %s.%s", bundle_key.c_str(), key_in_bundle.c_str());
			plurals_map.emplace(std::string(), pair_to_emplace);
		} else {
			/* Plural form. */
			if (plurals_map.count(linekey) > 0) SYNTAX_ERROR_V("Duplicate plural form %s.%s.%s", bundle_key.c_str(), key_in_bundle.c_str(), linekey.c_str());
			plurals_map.emplace(linekey, pair_to_emplace);
		}
	}
#undef SYNTAX_ERROR_V
#undef SYNTAX_ERROR

	stream.close();

	/* Now parse the meta info for language index and plural forms.  */
	if (results.count("meta") == 0) SYNTAX_ERROR_L(0, "Bundle 'meta' missing");
	const BundleContent &metamap = results.at("meta");

	auto extract_singular_meta_string = [&metamap, filename](std::string key) {
		const auto meta_key = metamap.find(key);
		if (meta_key == metamap.end()) SYNTAX_ERROR_VL(0, "Key 'meta'.'%s' missing", key.c_str());
		const PluralizedString &meta_key_str = meta_key->second;
		if (meta_key_str.size() != 1 || !meta_key_str.begin()->first.empty()) SYNTAX_ERROR_VL(0, "'meta'.'%s' may not be pluralized", key.c_str());
		return meta_key_str.begin()->second;
	};

	const PluralForm &lang_name = extract_singular_meta_string("lang");
	const int lang_idx = GetLanguageIndex(lang_name.first.c_str(), lang_name.second);
	if (lang_idx < 0) SYNTAX_ERROR_VL(lang_name.second.line, "Unrecognized language '%s'", lang_name.first.c_str());

	ParseEvaluateableExpression(extract_singular_meta_string("rule").first);  // Just to check that the rule exists and the syntax is valid.

	const PluralForm &nplurals_str = extract_singular_meta_string("nplurals");
	int nplurals;
	try {
		nplurals = stoi(nplurals_str.first);
	} catch (const std::exception&) {
		SYNTAX_ERROR_VL(nplurals_str.second.line, "nplurals '%s' is not a number", nplurals_str.first.c_str());
	}
	if (nplurals < 1) SYNTAX_ERROR_VL(nplurals_str.second.line, "nplurals %d must be >= 1", nplurals);

	std::vector<std::string> plural_names(nplurals);
	std::map<std::string, int> plural_name_to_index;
	for (int p = 0; p < nplurals; ++p) {
		const PluralForm &plural_name = extract_singular_meta_string(std::string("plural_") + std::to_string(p));
		if (plural_name.first.empty()) SYNTAX_ERROR_L(plural_name.second.line, "Empty plural name");
		if (plural_name_to_index.count(plural_name.first) > 0) {
			SYNTAX_ERROR_VL(plural_name.second.line, "Duplicate plural name '%s'", plural_name.first.c_str());
		}
		plural_names.at(p) = plural_name.first;
		plural_name_to_index.emplace(plural_name.first, p);
	}

	/* Finally, fill in the string storage with the parsed data. */
	for (const std::pair<std::string, BundleContent> &bundle : results) {
		StringBundle &stringbundle = this->bundles[bundle.first];
		std::shared_ptr<StringsNode> strings_node = std::make_shared<StringsNode>();

		for (const std::pair<std::string, PluralizedString> &contained_string : bundle.second) {
			/* Verify that all plural forms match. */
			const int found_nplurals = contained_string.second.size();
			if (found_nplurals != 1 && found_nplurals != nplurals) {
				SYNTAX_ERROR_VL(contained_string.second.begin()->second.second.line,
						"Wrong number of plural forms (expected %d, got %d)", nplurals, found_nplurals);
			}
			if (found_nplurals == 1) {
				if (!contained_string.second.begin()->first.empty() &&
						!(nplurals == 1 && contained_string.second.begin()->first == plural_names.at(0))) {
					SYNTAX_ERROR_L(contained_string.second.begin()->second.second.line, "Plural form for non-pluralized string");
				}
			} else {
				std::vector<bool> present(nplurals);
				for (const std::pair<std::string, PluralForm> &found_plural_forms : contained_string.second) {
					const auto plural_index = plural_name_to_index.find(found_plural_forms.first);
					if (plural_index == plural_name_to_index.end()) {
						SYNTAX_ERROR_VL(found_plural_forms.second.second.line, "Invalid plural name '%s'", found_plural_forms.first.c_str());
					}
					if (present.at(plural_index->second)) {
						SYNTAX_ERROR_VL(found_plural_forms.second.second.line, "Duplicate plural key '%s'", found_plural_forms.first.c_str());
					}
					present.at(plural_index->second) = true;
				}
			}

			/* Store the string in a form that can be parsed by the RCD generator. */
			StringNode str_node;
			str_node.name = contained_string.first;
			str_node.key = bundle.first;
			str_node.lang_index = lang_idx;
			str_node.text.resize(contained_string.second.size());
			for (const std::pair<std::string, PluralForm> &form : contained_string.second) {
				int pl_index = form.first.empty() ? 0 : plural_name_to_index.at(form.first);
				str_node.text.at(pl_index) = form.second.first;
				str_node.text_pos = form.second.second;
			}
			strings_node->Add(str_node, str_node.text_pos);
		}
		stringbundle.Fill(strings_node, Position(filename, 0));
	}
}

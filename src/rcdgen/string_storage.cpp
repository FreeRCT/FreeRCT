/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file string_storage.cpp Code of the translation storage. */

#include <fstream>
#include <regex>
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

/** Parser for a YAML language file. */
struct YAMLParser {
	explicit YAMLParser(StringsStorage *s, const char *f);
	~YAMLParser();

	void Parse();

private:
	using PluralForm = std::pair<std::string, Position>;            ///< A string, and its position in the file.
	using PluralizedString = std::map<std::string, PluralForm>;     ///< All plural forms for a string.
	using BundleContent = std::map<std::string, PluralizedString>;  ///< All strings in a bundle.

	void SyntaxError(int32 l, const char *msg);

	bool ParseIdentifier();
	std::string ParseValue();
	void StoreKeyValue();
	void ValidateMetaData();
	void SaveResults(const std::pair<std::string, BundleContent> &bundle);

	StringsStorage *storage;           ///< String storage to write results to.
	const char *filename;              ///< Name of the file being parsed.
	std::fstream stream;               ///< Stream to read from.

	int32 line_number;                 ///< Current line number.
	std::string line;                  ///< Line being parsed.
	uint nesting_depth;                ///< Depth of the current identifier nesting.
	std::vector<std::string> nesting;  ///< Current identifier nesting stack.
	std::string linekey;               ///< Identifier of the current line.

	std::map<std::string, BundleContent> results;     ///< Results from the parsing.
	int nplurals;                                     ///< Number of plural forms, according to the YAML file.
	std::vector<std::string> plural_names;            ///< Names of the plural form identifiers.
	std::map<std::string, int> plural_name_to_index;  ///< Each plural name's plural index.
	int lang_idx;                                     ///< Index of the language.
};

static const std::regex r_whitespace_comment("[ \t]*(#.*)?");      ///< Regex to match whitespace and an optional comment.
static const std::regex r_identifier("([ \t]*)([-_A-Za-z0-9]+)");  ///< Regex to match an identifier.

YAMLParser::YAMLParser(StringsStorage *s, const char *f)
: storage(s), filename(f), stream(f, std::fstream::in), line_number(-1), nesting_depth(0), nplurals(0), lang_idx(-1)
{
	if (!this->stream.is_open()) {
		fprintf(stderr, "Failed to read from '%s'\n", f);
		exit(1);
	}
}

YAMLParser::~YAMLParser()
{
	this->stream.close();
}

/**
 * Invalid syntax was encountered. Print an error and exit.
 * @param l Line where the error occurred.
 * @param msg Description of the error.
 */
void YAMLParser::SyntaxError(int32 l, const char *msg)
{
	fprintf(stderr, "YAML syntax error at %s:%d: %s\n", this->filename, l, msg);
	exit(1);
}

/**
 * Parse the linekey of the current line.
 * @return The line contains a value after the identifier.
 */
bool YAMLParser::ParseIdentifier()
{
	const size_t colon = this->line.find(':');
	if (colon == std::string::npos) SyntaxError(this->line_number, "missing identifier");

	const std::string identifier_line_part = this->line.substr(0, colon);
	std::smatch match;
	if (!std::regex_match(identifier_line_part, match, r_identifier)) SyntaxError(this->line_number, "invalid identifier");
	const std::string nesting_whitespace = match[1];
	this->linekey = match[2];
	this->nesting_depth = 0;
	for (const char *c = nesting_whitespace.c_str(); *c != '\0'; ++c) this->nesting_depth += (*c == '\t' ? 8 : 1);

	this->line = this->line.substr(colon + 1);
	if (std::regex_match(this->line, r_whitespace_comment)) {
		/* Identifier without a value. */
		this->nesting.resize(this->nesting_depth + 1);
		this->nesting.back() = this->linekey;
		return false;
	}
	return true;
}

/**
 * Parse the value of the current line.
 * @return The value.
 */
std::string YAMLParser::ParseValue()
{
	/* We have a value. This can be either an unquoted string, or a single-quoted string, or a double-quoted string.
	 * Quoted strings can additionally span multiple lines.
	 */
	this->line = this->line.substr(this->line.find_first_not_of(" \t"));
	if (this->line.at(0) != '\'' && this->line.at(0) != '\"') {
		/* Unquoted string. Read until first comment or end of line and skip trailing space. */
		this->line = this->line.substr(0, this->line.find('#'));
		while (!this->line.empty() && (this->line.back() == ' ' || this->line.back() == '\t')) this->line.pop_back();
		return this->line;
	}

	/* Quoted string. Required some manual parsing. */
	std::string value;
	const char delimiter = this->line.at(0);
	size_t pos = 1;
	size_t nr_chars = this->line.size();

	for (;; ++pos) {
		if (pos >= nr_chars) SyntaxError(this->line_number, "unterminated line in quoted value");
		if (this->line[pos] == delimiter) break;  // End of quoted string.
		if (this->line[pos] != '\\') {
			value.push_back(this->line[pos]);
			continue;
		}

		/* Backslash-escaped character. */
		++pos;
		if (pos < nr_chars) {
			switch (this->line[pos]) {
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
				case '\\':
					value.push_back('\\');
					break;
				default:
					SyntaxError(this->line_number, "invalid escape character");
			}
			continue;
		}

		/* The backslash escapes a line break. Load the next line and continue reading (discarding leading whitespace). */
		if (this->stream.peek() == EOF) SyntaxError(this->line_number, "unterminated string at end of file");
		++this->line_number;
		std::getline(this->stream, this->line);
		nr_chars = this->line.size();
		pos = 0;
		for (; pos < nr_chars && (this->line[pos] == ' ' || this->line[pos] == '\t'); ++pos);
		--pos;
	}

	if (!std::regex_match(this->line.substr(pos + 1), r_whitespace_comment)) SyntaxError(this->line_number, "junk after end of quoted string");
	return value;
}

/** Parse and store the value of the current line. */
void YAMLParser::StoreKeyValue()
{
	const std::string value = this->ParseValue();

	/* Extract the actual key inheritance chain. */
	std::vector<std::string> sanitized_key;
	size_t sanitized_key_len = 0;
	this->nesting_depth = std::min<size_t>(this->nesting_depth, this->nesting.size());
	for (size_t i = 1; i < this->nesting_depth; ++i) {
		if (!this->nesting.at(i).empty()) {
			sanitized_key.push_back(this->nesting.at(i));
			++sanitized_key_len;
		}
	}
	if (sanitized_key_len == 0) SyntaxError(this->line_number, "key not nested deply enough");
	if (sanitized_key_len > 2) SyntaxError(this->line_number, "key nested too deply");
	sanitized_key.push_back(this->linekey);

	std::string &bundle_key = sanitized_key.at(0);
	std::string &key_in_bundle = sanitized_key.at(1);
	std::pair<std::string, Position> pair_to_emplace(value, Position(this->filename, this->line_number));

	PluralizedString &plurals_map = this->results[bundle_key][key_in_bundle];
	if (sanitized_key_len == 1) {
		/* Ordinary string. */
		if (!plurals_map.empty()) SyntaxError(this->line_number, "duplicate key");
		plurals_map.emplace(std::string(), pair_to_emplace);
	} else {
		/* Plural form. */
		if (plurals_map.count(this->linekey) > 0) {
			SyntaxError(this->line_number, "duplicate plural form");
		}
		plurals_map.emplace(this->linekey, pair_to_emplace);
	}
}

/** Parse the file and store results in the %storage. */
void YAMLParser::Parse()
{
	while (this->stream.peek() != EOF) {
		++this->line_number;
		std::getline(this->stream, this->line);

		if (std::regex_match(this->line, r_whitespace_comment)) continue;

		if (!this->ParseIdentifier()) continue;  // Identifier without value.

		this->StoreKeyValue();
	}

	this->ValidateMetaData();

	for (const std::pair<std::string, BundleContent> bundle : this->results) this->SaveResults(bundle);
}

/** Check that all meta data is present and sane. */
void YAMLParser::ValidateMetaData()
{
	if (this->results.count("meta") == 0) SyntaxError(0, "Bundle 'meta' missing");
	const BundleContent &metamap = this->results.at("meta");

	auto extract_singular_meta_string = [&metamap, this](std::string key) {
		const auto meta_key = metamap.find(key);
		if (meta_key == metamap.end()) SyntaxError(0, "a meta key is missing");
		const PluralizedString &meta_key_str = meta_key->second;
		if (meta_key_str.size() != 1 || !meta_key_str.begin()->first.empty()) SyntaxError(0, "meta strings may not be pluralized");
		return meta_key_str.begin()->second;
	};

	const PluralForm &lang_name = extract_singular_meta_string("lang");
	this->lang_idx = GetLanguageIndex(lang_name.first.c_str(), lang_name.second);
	if (this->lang_idx < 0) SyntaxError(lang_name.second.line, "unrecognized language");

	const PluralForm &nplurals_str = extract_singular_meta_string("nplurals");
	try {
		this->nplurals = stoi(nplurals_str.first);
	} catch (const std::exception&) {
		SyntaxError(nplurals_str.second.line, "nplurals is not a number");
	}
	if (this->nplurals != _all_languages[this->lang_idx].nplurals) SyntaxError(nplurals_str.second.line, "wrong number of plurals for this language");

	this->plural_names.resize(this->nplurals);
	for (int p = 0; p < this->nplurals; ++p) {
		std::string key = "plural_";
		key += std::to_string(p);
		this->storage->keys_to_ignore.insert(key);
		const PluralForm &plural_name = extract_singular_meta_string(key);
		if (plural_name.first.empty()) SyntaxError(plural_name.second.line, "empty plural name");
		if (this->plural_name_to_index.count(plural_name.first) > 0) {
			SyntaxError(plural_name.second.line, "duplicate plural name");
		}
		this->plural_names.at(p) = plural_name.first;
		this->plural_name_to_index.emplace(plural_name.first, p);
	}
}

/**
 * Fill in the string storage with the parsed data.
 * @param bundle Bundle to save.
 */
void YAMLParser::SaveResults(const std::pair<std::string, BundleContent> &bundle)
{
	StringBundle &stringbundle = this->storage->bundles[bundle.first];
	std::shared_ptr<StringsNode> strings_node = std::make_shared<StringsNode>();

	for (const std::pair<std::string, PluralizedString> contained_string : bundle.second) {
		/* Verify that all plural forms match. */
		const int found_nplurals = contained_string.second.size();
		if (found_nplurals != 1 && found_nplurals != this->nplurals) {
			SyntaxError(contained_string.second.begin()->second.second.line, "wrong number of plural forms");
		}
		if (found_nplurals == 1) {
			if (!contained_string.second.begin()->first.empty() &&
					!(this->nplurals == 1 && contained_string.second.begin()->first == this->plural_names.at(0))) {
				SyntaxError(contained_string.second.begin()->second.second.line, "plural form for non-pluralized string");
			}
		} else {
			std::vector<bool> present(this->nplurals);
			for (const std::pair<std::string, PluralForm> found_plural_forms : contained_string.second) {
				const auto plural_index = this->plural_name_to_index.find(found_plural_forms.first);
				if (plural_index == this->plural_name_to_index.end()) {
					SyntaxError(found_plural_forms.second.second.line, "invalid plural name");
				}
				if (present.at(plural_index->second)) {
					SyntaxError(found_plural_forms.second.second.line, "duplicate plural key");
				}
				present.at(plural_index->second) = true;
			}
		}

		/* Store the string in a form that can be parsed by the RCD generator. */
		StringNode str_node;
		str_node.name = contained_string.first;
		str_node.key = bundle.first;
		str_node.lang_index = this->lang_idx;
		str_node.text.resize(contained_string.second.size());
		for (const std::pair<std::string, PluralForm> form : contained_string.second) {
			int pl_index = form.first.empty() ? 0 : this->plural_name_to_index.at(form.first);
			str_node.text.at(pl_index) = form.second.first;
			str_node.text_pos = form.second.second;
		}
		strings_node->Add(str_node, str_node.text_pos);
	}
	stringbundle.Fill(strings_node, Position(this->filename, 0));
}

/**
 * Parse the provided YAML file and store its translations.
 * @param filename File to read from.
 */
void StringsStorage::ReadFromYAML(const char *filename)
{
	YAMLParser p(this, filename);
	p.Parse();
}

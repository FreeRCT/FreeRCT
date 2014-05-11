/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file config_reader.cpp Reader code for .cfg files. */

#include "stdafx.h"
#include "config_reader.h"
#include "fileio.h"
#include "string_func.h"
#include <string>

/**
 * Construct a key/value item.
 * @param key Key text.
 * @param value Value text.
 * @note Key and value texts get copied during construction.
 */
ConfigItem::ConfigItem(const char *key, const char *value)
{
	this->key = StrDup(key);
	this->value = StrDup(value);
}

ConfigItem::~ConfigItem()
{
	delete[] this->key;
	delete[] this->value;
}

/**
 * Get the value of the item, as an integer.
 * @return The item value if value is valid integer, else \c -1.
 */
int ConfigItem::GetNum() const
{
	int val = 0;
	int length = 0;
	const char *p = this->value;

	while (*p >= '0' && *p <= '9') {
		val = val * 10 + (*p - '0');
		length++;
		p++;
	}
	if (*p != '\0' || length > 8) return -1; // Bad integer number
	return val;
}

/**
 * Construct a named section.
 * @param sect_name Name of the new section.
 * @note Section name gets copied during construction.
 */
ConfigSection::ConfigSection(const char *sect_name)
{
	this->sect_name = StrDup(sect_name);
	this->items.clear();
}

ConfigSection::~ConfigSection()
{
	delete[] this->sect_name;
}

/**
 * Get an item from a section if it exists.
 * @param key Value of the key to look for (case sensitive).
 * @return The associated item if it exists, else \c nullptr.
 */
const ConfigItem *ConfigSection::GetItem(const char *key) const
{
	for (const auto &iter : this->items) {
		if (!strcmp(iter->key, key)) return iter;
	}
	return nullptr;
}

ConfigFile::ConfigFile()
{
	this->Clear();
}

ConfigFile::~ConfigFile()
{
	this->Clear();
}

/** Clean the config file. */
void ConfigFile::Clear()
{
	for (auto &iter : this->sections) delete iter;
}

/**
 * Is the given character a white space character?
 * @param k Provided character.
 * @return The provided character is a white space character.
 * @ingroup fileio_group
 */
static bool IsSpace(char k)
{
	return k == ' ' || k == '\r' || k == '\n' || k == '\t';
}

/**
 * Strip white space from the start and end of the line.
 * @param first Start of the line.
 * @param last If supplied, the terminator of the string, \c nullptr means only the start is supplied.
 * @return Line with stripped front and end.
 * @note Supplied line may be modified.
 * @ingroup fileio_group
 */
static char *StripWhitespace(char *first, char *last = nullptr)
{
	if (last == nullptr) {
		last = first;
		while (*last != '\0') last++;
	}

	assert(*last == '\0');

	while (first < last && IsSpace(*first)) first++;
	while (last > first + 1 && IsSpace(last[-1])) last--;
	*last = '\0';
	return first;
}

/**
 * Try to find the configuration file from the list of directories, and load it if found.
 * @param dir_list \c nullptr terminated list of directories to try.
 * @param fname Filename to look for.
 * @return Loading succeeded.
 */
bool ConfigFile::LoadFromDirectoryList(const char **dir_list, const char *fname)
{
	while (*dir_list != nullptr) {
		std::string fpath = std::string(*dir_list) + '/' + fname;
		if (PathIsFile(fpath.c_str()) && this->Load(fpath.c_str())) {
			return true;
		}
		dir_list++;
	}
	return false;
}

/**
 * Load a config file.
 * @param fname Filename to load.
 * @return Loading succeeded.
 * @todo [easy] Eliminate duplicate config sections.
 * @todo [easy] Strip whitespace around section names.
 */
bool ConfigFile::Load(const char *fname)
{
	ConfigSection *current_sect = nullptr;

	this->Clear();

	FILE *fp = fopen(fname, "rb");
	if (fp == nullptr) return false;

	for (;;) {
		char buffer[256];
		char *line = fgets(buffer, lengthof(buffer), fp);
		if (line == nullptr) break;

		while (*line != '\0' && IsSpace(*line)) line++;
		if (*line == '\0' || *line == ';') continue; // Silently skip empty lines or comment lines.

		if (*line == '[') {
			/* New section. */
			char *line2 = line;
			while (*line2 != '\0' && *line2 != ']') line2++;
			if (*line2 == ']') {
				*line2 = '\0';
				/* XXX Look for an existing config section? */
				current_sect = new ConfigSection(StripWhitespace(line + 1, line2));
				this->sections.push_front(current_sect);
			}
			continue;
		}

		if (current_sect == nullptr) continue; // No section opened, ignore such lines.

		/* Key/value line. */
		char *line2 = line;
		while (*line2 != '\0' && *line2 != '=') line2++;
		ConfigItem *item;
		if (*line2 == '=') {
			*line2 = '\0';
			item = new ConfigItem(StripWhitespace(line, line2), StripWhitespace(line2 + 1));
		} else {
			/* No value. */
			item = new ConfigItem(StripWhitespace(line, line2), "");
		}
		current_sect->items.push_front(item);
	}
	fclose(fp);
	return true;
}

/**
 * Get a section from the configuration file.
 * @param sect_name Name of the section (case sensitive).
 * @return The section with the give name if it exists, else \c nullptr.
 */
const ConfigSection *ConfigFile::GetSection(const char *sect_name) const
{
	for (const auto &iter : this->sections) {
		if (!strcmp(iter->sect_name, sect_name)) return iter;
	}
	return nullptr;
}

/**
 * Get an item value from the configuration file.
 * @param sect_name Name of the section (case sensitive).
 * @param key Name of the key (case sensitive).
 * @return The associated value if it exists, else \c nullptr.
 */
const char *ConfigFile::GetValue(const char *sect_name, const char *key) const
{
	const ConfigSection *sect = this->GetSection(sect_name);
	if (sect == nullptr) return nullptr;

	const ConfigItem *item = sect->GetItem(key);
	if (item == nullptr) return nullptr;
	return item->value;
}

/**
 * Get a number from the configuration file.
 * @param sect_name Name of the section (case sensitive).
 * @param key Name of the key (case sensitive).
 * @return The associated number if it exists, else \c -1.
 */
int ConfigFile::GetNum(const char *sect_name, const char *key) const
{
	const ConfigSection *sect = this->GetSection(sect_name);
	if (sect == nullptr) return -1;

	const ConfigItem *item = sect->GetItem(key);
	if (item == nullptr) return -1;

	return item->GetNum();
}

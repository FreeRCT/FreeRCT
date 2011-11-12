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
#include "string_func.h"

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
	free(const_cast<char *>(this->key));
	free(const_cast<char *>(this->value));
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
	free(const_cast<char *>(this->sect_name));
	for (ConfigItemList::iterator iter = this->items.begin(); iter != this->items.end(); iter++) {
		free(*iter);
	}
}

/**
 * Get an item from a section if it exists.
 * @param key Value of the key to look for (case sensitive).
 * @return The associated item if it exists, else \c NULL.
 */
const ConfigItem *ConfigSection::GetItem(const char *key) const
{
	for (ConfigItemList::const_iterator iter = this->items.begin(); iter != this->items.end(); iter++) {
		if (!strcmp((*iter)->key, key)) return *iter;
	}
	return NULL;
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
	for (ConfigSectionList::iterator iter = this->sections.begin(); iter != this->sections.end(); iter++) {
		free(*iter);
	}
}

/**
 * Is the given character a white space character?
 * @param k Provided character.
 * @return The provided character is a white space character.
 */
static bool IsSpace(char k)
{
	return k == ' ' || k == '\r' || k == '\n' || k == '\t';
}

/**
 * Strip white space from the start and end of the line.
 * @param first Start of the line.
 * @param last If supplied, the terminator of the string, \c NULL means only the start is supplied.
 * @return Line with stripped front and end.
 * @note Supplied line may be modified.
 */
static char *StripWhitespace(char *first, char *last = NULL)
{
	if (last == NULL) {
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
 * Load a config file.
 * @param fname Filename to load.
 * @return Loading succeeded.
 * @todo [easy] Eliminate duplicate config sections.
 * @todo [easy] Strip whitespace around section names.
 */
bool ConfigFile::Load(const char *fname)
{
	ConfigSection *current_sect = NULL;

	this->Clear();

	FILE *fp = fopen(fname, "rb");
	if (fp == NULL) return false;

	for (;;) {
		char buffer[256];
		char *line = fgets(buffer, lengthof(buffer), fp);
		if (line == NULL) break;

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
				this->sections.push_back(current_sect);
			}
			continue;
		}

		if (current_sect == NULL) continue; // No section opened, ignore such lines.

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
		current_sect->items.push_back(item);
	}
	fclose(fp);
	return true;
}

/**
 * Get a section from the configuration file.
 * @param sect_name Name of the section (case sensitive).
 * @return The section with the give name if it exists, else \c NULL.
 */
const ConfigSection *ConfigFile::GetSection(const char *sect_name) const
{
	for (ConfigSectionList::const_iterator iter = this->sections.begin(); iter != this->sections.end(); iter++) {
		if (!strcmp((*iter)->sect_name, sect_name)) return *iter;
	}
	return NULL;
}

/**
 * Get an item value from the configuration file.
 * @param sect_name Name of the section (case sensitive).
 * @param key Name of the key (case sensitive).
 * @return The associated value if it exists, else \c NULL.
 */
const char *ConfigFile::GetValue(const char *sect_name, const char *key) const
{
	const ConfigSection *sect = this->GetSection(sect_name);
	if (sect == NULL) return NULL;

	const ConfigItem *item = sect->GetItem(key);
	if (item == NULL) return NULL;
	return item->value;
}

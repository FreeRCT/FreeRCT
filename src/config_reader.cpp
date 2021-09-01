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

/**
 * Construct a key/value item.
 * @param section The section this value belongs to.
 * @param key Key text.
 * @param value Value text.
 */
ConfigItem::ConfigItem(const ConfigSection &section, const std::string &key, const std::string &value) : section(section), key(key), value(value)
{
	this->used = false;
}

/** Destructor. */
ConfigItem::~ConfigItem() {
	if (!this->used) {
		printf("WARNING: Config file '%s', key '%s'='%s' in section '%s' not used (perhaps the name is misspelled?)\n",
				this->section.file.filename.c_str(), this->key.c_str(), this->value.c_str(), this->section.name.c_str());
	}
}

/**
 * Get the value of the item, as a string.
 * @return The item value.
 */
const std::string &ConfigItem::GetString() const
{
	this->used = true;
	return this->value;
}

/**
 * Get the value of the item, as an integer.
 * @return The item value if value is valid integer, else \c -1.
 */
int ConfigItem::GetNum() const
{
	this->used = true;
	try {
		size_t position;
		const int result = stoi(this->value, &position);
		if (position != this->value.size()) return -1;
		return result;
	} catch (...) {
		/* Not a valid integer string. */
		return -1;
	}
}

/**
 * Construct a section.
 * @param file The config file this section belongs to.
 * @param name Section name.
 */
ConfigSection::ConfigSection(const ConfigFile &file, const std::string &name) : file(file), name(name)
{
	this->used = false;
}

/** Destructor. */
ConfigSection::~ConfigSection() {
	if (!this->used) {
		printf("WARNING: Config file '%s', section '%s' not used (perhaps the name is misspelled?)\n",
				this->file.filename.c_str(), this->name.c_str());
		/* If a section is unused, suppress warnings about all keys therein. */
		for (auto &pair : this->items) pair.second->GetString();
	}
}

/**
 * Get an item from a section if it exists.
 * @param key Value of the key to look for (case sensitive).
 * @return The associated item if it exists, else \c nullptr.
 */
const ConfigItem *ConfigSection::GetItem(const std::string &key) const
{
	this->used = true;
	const auto it = this->items.find(key);
	return it == this->items.end() ? nullptr : it->second.get();
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

	while (first < last && isspace(*first)) first++;
	while (last > first + 1 && isspace(last[-1])) last--;
	*last = '\0';
	return first;
}

/**
 * Load a config file.
 * @param fname Filename to load.
 * @todo [easy] Eliminate duplicate config sections.
 * @todo [easy] Strip whitespace around section names.
 */
ConfigFile::ConfigFile(const std::string &fname) : filename(fname)
{
	ConfigSection *current_sect = nullptr;

	FILE *fp = fopen(fname.c_str(), "rb");
	if (fp == nullptr) return;

	for (;;) {
		char buffer[256];
		char *line = fgets(buffer, lengthof(buffer), fp);
		if (line == nullptr) break;

		while (*line != '\0' && isspace(*line)) line++;
		if (*line == '\0' || *line == ';' || *line == '#') continue; // Silently skip empty lines or comment lines.

		if (*line == '[') {
			/* New section. */
			char *line2 = line;
			while (*line2 != '\0' && *line2 != ']') line2++;
			if (*line2 == ']') {
				*line2 = '\0';
				const std::string sect_name(StripWhitespace(line + 1, line2));
				if (this->sections.count(sect_name) == 0) this->sections.emplace(sect_name,
						std::unique_ptr<ConfigSection>(new ConfigSection(*this, sect_name)));
				current_sect = this->sections.at(sect_name).get();
			}
			continue;
		}

		if (current_sect == nullptr) continue; // No section opened, ignore such lines.

		/* Key/value line. */
		char *line2 = line;
		while (*line2 != '\0' && *line2 != '=') line2++;
		std::string key, value;
		if (*line2 == '=') {
			*line2 = '\0';
			key = StripWhitespace(line, line2);
			value = StripWhitespace(line2 + 1);
		} else {
			/* No value. */
			key = StripWhitespace(line, line2);
		}
		current_sect->items.emplace(key, std::unique_ptr<ConfigItem>(new ConfigItem(*current_sect, key, value)));
	}
	fclose(fp);
}

/**
 * Get a section from the configuration file.
 * @param sect_name Name of the section (case sensitive).
 * @return The section with the give name if it exists, else \c nullptr.
 */
const ConfigSection *ConfigFile::GetSection(const std::string &sect_name) const
{
	const auto it = this->sections.find(sect_name);
	return it == this->sections.end() ? nullptr : it->second.get();
}

/**
 * Get an item value from the configuration file.
 * @param sect_name Name of the section (case sensitive).
 * @param key Name of the key (case sensitive).
 * @return The associated value if it exists, else \c nullptr.
 */
std::string ConfigFile::GetValue(const std::string &sect_name, const std::string &key) const
{
	const ConfigSection *sect = this->GetSection(sect_name);
	if (sect == nullptr) return std::string();

	const ConfigItem *item = sect->GetItem(key);
	if (item == nullptr) return std::string();
	return item->GetString();
}

/**
 * Get a number from the configuration file.
 * @param sect_name Name of the section (case sensitive).
 * @param key Name of the key (case sensitive).
 * @return The associated number if it exists, else \c -1.
 */
int ConfigFile::GetNum(const std::string &sect_name, const std::string &key) const
{
	const ConfigSection *sect = this->GetSection(sect_name);
	if (sect == nullptr) return -1;

	const ConfigItem *item = sect->GetItem(key);
	if (item == nullptr) return -1;

	return item->GetNum();
}

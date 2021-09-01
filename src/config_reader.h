/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file config_reader.h Reader for .cfg files. */

#ifndef CONFIG_READER_H
#define CONFIG_READER_H

#include <map>
#include <memory>
#include <string>

/**
 * Item in a configuration file (a key/value pair).
 * @ingroup fileio_group
 */
class ConfigItem {
public:
	explicit ConfigItem(const std::string &value);

	int GetNum() const;

	std::string value; ///< Value text.
};

/**
 * Section in a configuration file (a set of related items).
 * @ingroup fileio_group
 */
class ConfigSection {
public:
	const ConfigItem *GetItem(const std::string &key) const;
	std::map<std::string, std::unique_ptr<ConfigItem>> items;  ///< Items of the section.
};

/**
 * A configuration file.
 * @ingroup fileio_group
 */
class ConfigFile {
public:
	explicit ConfigFile(const std::string &fname);

	const ConfigSection *GetSection(const std::string &name) const;
	std::string GetValue(const std::string &sect_name, const std::string &key) const;
	int GetNum(const std::string &sect_name, const std::string &key) const;

	std::map<std::string, std::unique_ptr<ConfigSection>> sections; ///< Sections of the configuration file.
};

#endif

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

class ConfigSection;
class ConfigFile;

/**
 * Item in a configuration file (a key/value pair).
 * @ingroup fileio_group
 */
class ConfigItem {
public:
	explicit ConfigItem(const ConfigSection &section, const std::string &key, const std::string &value);
	~ConfigItem();

	int GetNum() const;
	const std::string &GetString() const;

private:
	const ConfigSection &section;  ///< Section backlink.
	const std::string key;         ///< Key text.
	const std::string value;       ///< Value text.
	mutable bool used;             ///< Whether this value has ever been read from.
};

/**
 * Section in a configuration file (a set of related items).
 * @ingroup fileio_group
 */
class ConfigSection {
public:
	explicit ConfigSection(const ConfigFile &file, const std::string &name);
	~ConfigSection();

	const ConfigItem *GetItem(const std::string &key) const;

	const ConfigFile &file;  ///< Config file backlink.
	const std::string name;  ///< Section name.

private:
	friend class ConfigFile;
	mutable bool used;                                         ///< Whether this section has ever been accessed.
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

	const std::string filename;                                      ///< Name of the config file.
	std::map<std::string, std::unique_ptr<ConfigSection>> sections;  ///< Sections of the configuration file.
};

#endif

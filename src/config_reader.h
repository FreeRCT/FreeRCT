/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file config_reader.h Reader for .cfg files. */

#ifndef CONFIG_READER_H
#define CONFIG_READER_H

#include <forward_list>

/**
 * Item in a configuration file (a key/value pair).
 * @ingroup fileio_group
 */
class ConfigItem {
public:
	ConfigItem(const char *key, const char *value);
	~ConfigItem();

	const char *key;   ///< Key text.
	const char *value; ///< Value text.
};

/**
 * A list of items.
 * @ingroup fileio_group
 */
typedef std::forward_list<ConfigItem *> ConfigItemList;

/**
 * Section in a configuration file (a set of related items).
 * @ingroup fileio_group
 */
class ConfigSection {
public:
	ConfigSection(const char *sect_name);
	~ConfigSection();

	const ConfigItem *GetItem(const char *key) const;

	const char *sect_name; ///< Name of the section.
	ConfigItemList items; ///< Items of the section.
};

/**
 * A list of sections.
 * @ingroup fileio_group
 */
typedef std::forward_list<ConfigSection *> ConfigSectionList;

/**
 * A configuration file.
 * @ingroup fileio_group
 */
class ConfigFile {
public:
	ConfigFile();
	~ConfigFile();

	void Clear();
	bool Load(const char *fname);

	const ConfigSection *GetSection(const char *name) const;
	const char *GetValue(const char *sect_name, const char *key) const;

	ConfigSectionList sections; ///< Sections of the configuration file.
};

#endif

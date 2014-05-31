/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file string_storage.h Storage of (translated) strings. */

#ifndef STRING_STORAGE_H
#define STRING_STORAGE_H

class StringsStorage {
public:
	StringsStorage();

	void AddStrings(std::shared_ptr<StringsNode> strs, const Position &pos);
	void AddToBundle(std::shared_ptr<StringsNode> strs, const Position &pos);
	const StringBundle *GetBundle(const std::string &key);
	
	std::map<std::string, StringBundle> bundles; ///< Available bundles, ordered by key.
};

extern StringsStorage _strings_storage;

#endif

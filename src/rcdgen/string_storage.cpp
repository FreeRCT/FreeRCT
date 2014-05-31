/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file string_storage.cpp Code of the translation storage. */

#include "../stdafx.h"
#include "ast.h"
#include "nodes.h"
#include "string_storage.h"

StringsStorage _strings_storage; ///< Storage of all (translated) strings by key.

StringsStorage::StringsStorage()
{
}

/**
 * Examine the still available strings, and decide whether it needs to be split on key.
 * @param strs Strings to examine.
 * @return Whether there are at least two keys in the collection, and it needs to be split.
 */
static bool NeedsSplit(std::shared_ptr<StringsNode> strs)
{
	std::string key;
	int num_keys = 0;
	for (auto &str : strs->strings) {
		if (num_keys == 0) {
			key = str.key;
			num_keys++;
			continue;
		}
		if (str.key == key) continue;
		return true;
	}
	return false;
}

/**
 * Add a collection of strings to the storage.
 * @param strs String collection to add.
 * @param pos Position of the collection.
 */
void StringsStorage::AddStrings(std::shared_ptr<StringsNode> strs, const Position &pos)
{
	/* Split the strings into lists with the same key, and push them into a bundle. */
	while (strs->strings.size() > 0) {
		if (!NeedsSplit(strs)) {
			this->AddToBundle(strs, pos);
			break;
		}
		auto new_strs = std::make_shared<StringsNode>();
		std::string new_key = "";
		std::list<StringNode>::iterator iter = strs->strings.begin();
		while (iter != strs->strings.end()) {
			if (new_key == "") {
				if (iter->key != "") {
					new_key = iter->key;
					new_strs->Add(*iter, pos);
					std::list<StringNode>::iterator iter2 = iter; iter++;
					strs->strings.erase(iter2);
					continue;
				}
				iter++;
				continue;
			}
			if (new_key == iter->key) {
				new_strs->Add(*iter, pos);
				std::list<StringNode>::iterator iter2 = iter; iter++;
				strs->strings.erase(iter2);
				continue;
			}
			iter++;
		}
		this->AddToBundle(new_strs, pos);
	}
}

/**
 * Add strings belonging to one bundle to storage.
 * @param strs Strings to add
 * @param pos Position of the collection.
 */
void StringsStorage::AddToBundle(std::shared_ptr<StringsNode> strs, const Position &pos)
{
	std::string strs_key = strs->GetKey();
	if (strs_key == "") {
		fprintf(stderr, "Error at %s: \"strings\" node does not have a key.\n", pos.ToString());
		exit(1);
	}
	auto iter = this->bundles.find(strs_key);
	if (iter == this->bundles.end()) {
		std::pair<std::string, StringBundle> p(strs_key, StringBundle());
		iter = this->bundles.insert(p).first;
	}
	StringBundle &sb = iter->second;
	sb.Fill(strs, pos);
}

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

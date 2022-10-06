/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file loadsave.h Load and save functions and classes. */

#ifndef LOADSAVE_H
#define LOADSAVE_H

#include <ctime>
#include <memory>
#include <vector>
#include "fileio.h"

struct Scenario;

static const std::string SAVEGAME_DIRECTORY("save");  ///< The directory where savegames are stored, relative to the user data directory.
static const std::string TRACK_DESIGN_DIRECTORY("tracks");  ///< The directory where track designs are stored, relative to the user data directory.

/** Class for loading a save game. */
class Loader {
public:
	Loader(FILE *fp);

	uint32 OpenPattern(const char *name, bool may_fail = false, bool name_only = false);
	void ClosePattern();

	uint8 GetByte();
	uint16 GetWord();
	uint32 GetLong();
	uint64 GetLongLong();
	std::string GetText();

	void VersionMismatch(uint saved_version, uint current_version);

private:
	void PutByte(uint8 val);

	std::vector<std::string> pattern_names; ///< Stack of the currently loaded pattern.

	FILE *fp;             ///< Data stream being loaded.
	int cache_count;      ///< Number of values in #cache.
	uint8 cache[8];       ///< Stack with temporary values to return on next read.
};

/** Class for saving a savegame. */
class Saver {
public:
	Saver(FILE *fp);

	void StartPattern(const char *name);
	void StartPattern(const char *name, uint32 version);
	void EndPattern();

	void PutByte(uint8 val);
	void PutWord(uint16 val);
	void PutLong(uint32 val);
	void PutLongLong(uint64 val);
	void PutText(const std::string &str, int length = -1);

	void CheckNoOpenPattern() const;

private:
	FILE *fp; ///< Output file stream.
	std::vector<std::string> pattern_names; ///< Stack of the current pattern names.
};

/** Holds basic data about a savegame file. */
struct PreloadData {
	int fcts_version;           ///< Version number of the FCTS block.
	bool load_success = false;  ///< Whether the header was loaded correctly. If this is \c false, all other data fields are invalid.
	std::string filename;       ///< Name of the savegame file, without file path, with file extension.
	time_t timestamp = 0;       ///< Timestamp when the savegame was created.
	std::string revision;       ///< Program version with which the savegame was created.
	std::unique_ptr<Scenario> scenario;  ///< Scenario parameters.

	/**
	 * Sorting operator to allow using PreloadData in ordered STL containers.
	 * @param pd Object to compare to.
	 * @return This object should sort before the other object.
	 */
	inline bool operator<(const PreloadData &pd) const
	{
		return this->filename < pd.filename;
	}
};

bool LoadGameFile(const char *fname);
bool SaveGameFile(const char *fname);
PreloadData PreloadGameFile(const char *fname);

extern bool _automatically_resave_files;

#endif

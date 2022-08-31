/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fileio.h File IO declarations. */

#ifndef FILEIO_H
#define FILEIO_H

#include "string_func.h"

/** An error that occurs while loading a data file. */
struct LoadingError : public std::exception {
	explicit LoadingError(const char *fmt, ...);

	const char* what() const noexcept override;

private:
	std::string message;
};

/**
 * Base class for reading the contents of a directory.
 * Intended use:
 * - After creation (with the #MakeDirectoryReader function), call #OpenPath to open the directory.
 * - A new entry of the directory is returned with every call to #NextEntry.
 * - At the end, call #ClosePath to allow the object to clean up, and get ready for a next use.
 *
 * Alternatively, use #MakePath to construct a path you want to examine.
 * @ingroup fileio_group
 */
class DirectoryReader {
public:
	DirectoryReader() = default;
	virtual ~DirectoryReader() = default;

	virtual void OpenPath(const char *path) = 0;
	virtual const char *NextEntry() = 0;
	virtual void ClosePath() = 0;

	const char *NextFile();
	virtual const char *MakePath(const char *directory, const char *fname) = 0;

	virtual bool EntryIsFile() = 0;
	virtual bool EntryIsDirectory() = 0;
};

extern const char *DIR_SEP;  ///< Directory separator character.

/**
 * Class for reading an RCD file.
 * @ingroup fileio_group
 */
class RcdFileReader {
public:
	RcdFileReader(const char *fname);
	~RcdFileReader();

	bool CheckFileHeader(const char *hdr_name, uint32 version);
	bool ReadBlockHeader();
	bool SkipBytes(uint32 count);

	bool GetBlob(void *address, size_t length);

	uint8  GetUInt8();
	uint16 GetUInt16();
	uint32 GetUInt32();
	int8  GetInt8();
	int16 GetInt16();
	int32 GetInt32();
	std::string GetText();

	size_t GetRemaining();

	/**
	 * An error occurred during loading.
	 * @param fmt Format message.
	 * @param args Format arguments.
	 * @pre Must be inside a block.
	 */
	template<typename... Args>
	void Error(const char *fmt, Args... args)
	{
		throw LoadingError("Error while loading '%s' block from %s: %s", this->name, this->filename.c_str(), Format(fmt, args...).c_str());
	}

	void CheckVersion(uint32 current_version);
	void CheckMinLength(int length, int required, const char *what);
	void CheckExactLength(int length, int required, const char *what);

	std::string filename;  ///< Name of the RCD file.
	char name[5];   ///< Name of the last found block (with #ReadBlockHeader).
	uint32 version; ///< Version number of the last found block (with #ReadBlockHeader).
	uint32 size;    ///< Data size of the last found block (with #ReadBlockHeader).

private:
	FILE *fp;         ///< File handle of the opened file.
	size_t file_pos;  ///< Position in the opened file.
	size_t file_size; ///< Size of the opened file.
};

bool PathIsFile(const char *path);
bool PathIsDirectory(const char *path);

DirectoryReader *MakeDirectoryReader();

void MakeDirectory(std::string path);
void CopyBinaryFile(const char *src, const char *dest);
void RemoveFile(const char *path);

const std::string &GetUserHomeDirectory();

std::string FindDataFile(const std::string &name);
const std::string &SavegameDirectory();

#endif

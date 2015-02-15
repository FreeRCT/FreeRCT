/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fileio.h File IO declarations. */

#ifndef FILEIO_H
#define FILEIO_H

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
	DirectoryReader(const char dir_sep);
	virtual ~DirectoryReader();

	virtual void OpenPath(const char *path) = 0;
	virtual const char *NextEntry() = 0;
	virtual void ClosePath() = 0;

	const char *NextFile();
	virtual const char *MakePath(const char *directory, const char *fname) = 0;

	virtual bool EntryIsFile() = 0;
	virtual bool EntryIsDirectory() = 0;

	const char dir_sep; ///< Directory separator character.
};

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

	size_t GetRemaining();

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

bool ChangeWorkingDirectoryToExecutable(const char *exe);

#endif

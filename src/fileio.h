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
 * - At the end, call #ClosePath to allow the object to cleanup, and get ready for a next use.
 * @ingroup fileio_group
 */
class DirectoryReader {
public:
	DirectoryReader();
	virtual ~DirectoryReader();

	virtual void OpenPath(const char *path) = 0;
	virtual const char *NextEntry() = 0;
	virtual void ClosePath() = 0;

	const char *NextFile();

	virtual bool EntryIsFile() = 0;
	virtual bool EntryIsDirectory() = 0;
};

/**
 * Class representing a RCD file.
 * @ingroup fileio_group
 */
class RcdFile {
public:
	RcdFile(const char *fname);
	~RcdFile();

	bool CheckFileHeader(const char *hdr_name, uint32 version);

	bool GetBlob(void *address, size_t length);

	uint8 GetUInt8();
	uint16 GetUInt16();
	uint32 GetUInt32();
	int16 GetInt16();

	size_t GetRemaining();

private:
	FILE *fp;         ///< File handle of the opened file.
	size_t file_pos;  ///< Position in the opened file.
	size_t file_size; ///< Size of the opened file.
};


DirectoryReader *MakeDirectoryReader();

#endif

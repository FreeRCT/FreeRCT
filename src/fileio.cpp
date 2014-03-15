/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fileio.cpp File IO functions. */

/**
 * @defgroup fileio_group File IO
 */

#include "stdafx.h"
#include "fileio.h"
#include "string_func.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

/**
 * Base class implementation of a directory reader, never returning any content.
 * Derive a new class for your operating system with more functionality.
 */
DirectoryReader::DirectoryReader()
{
}

/** Destructor. */
DirectoryReader::~DirectoryReader()
{
}

/**
 * @fn void DirectoryReader::OpenPath(const char *path)
 * Set up the directory reader object for reading a directory.
 * @param path Path to the directory.
 * @note Failure is not reported here, but #NextEntry will not return anything useful.
 */

/**
 * @fn const char *DirectoryReader::NextEntry()
 * Get next entry of the directory contents.
 * @return Pointer to name of next entry (as a file path suitable for opening a file). Returns \c nullptr if not next entry exists.
 * @note The memory returned is owned by the #DirectoryReader object, and should not be released.
 */

/**
 * @fn void DirectoryReader::ClosePath()
 * Denote no further interest in reading the contents of the current directory.
 */

/**
 * @fn bool DirectoryReader::EntryIsFile()
 * Test whether the last returned entry from #NextEntry is a file.
 * @return Whether the entry is a file.
 */

/**
 * @fn bool DirectoryReader::EntryIsDirectory()
 * Test whether the last returned entry from #NextEntry is a directory.
 * @return Whether the entry is a directory.
 */

/**
 * Get the next file entry.
 * Pulls entries from #NextEntry, until the end is reached or until a file is returned.
 * @return Next file entry, if available.
 */
const char *DirectoryReader::NextFile()
{
	const char *entry;

	do {
		entry = this->NextEntry();
	} while (entry != nullptr && !this->EntryIsFile());
	return entry;
}

/**
 * Directory reader for a Unix system.
 * @ingroup fileio_group
 */
class UnixDirectoryReader : public DirectoryReader {
public:
	UnixDirectoryReader();
	virtual ~UnixDirectoryReader();

	virtual void OpenPath(const char *path) override;
	virtual const char *NextEntry() override;
	virtual void ClosePath() override;

	virtual bool EntryIsFile() override;
	virtual bool EntryIsDirectory() override;

private:
	DIR *dirfp;           ///< Directory stream if not \c nullptr.
	dirent *entry;        ///< Pointer to current directory entry (or \c nullptr).
	char dpath[MAX_PATH]; ///< Directory path.
	char fpath[MAX_PATH]; ///< File path returned by #NextEntry.
};

UnixDirectoryReader::UnixDirectoryReader() : DirectoryReader()
{
	this->dirfp = nullptr;
}

UnixDirectoryReader::~UnixDirectoryReader()
{
	this->ClosePath();
}

void UnixDirectoryReader::OpenPath(const char *path)
{
	if (this->dirfp != nullptr) this->ClosePath();

	SafeStrncpy(this->dpath, path, MAX_PATH);
	this->dirfp = opendir(this->dpath);
}

const char *UnixDirectoryReader::NextEntry()
{
	if (this->dirfp == nullptr) return nullptr;

	this->entry = readdir(this->dirfp);
	if (this->entry == nullptr) {
		this->ClosePath();
		return nullptr;
	}

	snprintf(this->fpath, MAX_PATH, "%s/%s", this->dpath, this->entry->d_name);
	this->fpath[MAX_PATH - 1] = '\0'; // Better safe than sorry.
	return this->fpath;
}

void UnixDirectoryReader::ClosePath()
{
	if (this->dirfp != nullptr) closedir(this->dirfp);
	this->dirfp = nullptr;
}

bool UnixDirectoryReader::EntryIsFile()
{
	struct stat st;

	if (stat(this->fpath, &st) != 0) return false;
	return S_ISREG(st.st_mode);
}

bool UnixDirectoryReader::EntryIsDirectory()
{
	struct stat st;

	if (stat(this->fpath, &st) != 0) return false;
	return S_ISDIR(st.st_mode);
}

/**
 * Construct a directory reader object (specific for the operating system).
 * @return A directory reader.
 * @ingroup fileio_group
 */
DirectoryReader *MakeDirectoryReader()
{
	return new UnixDirectoryReader();
}

/**
 * RCD file constructor, loading data from a file.
 * @param fname Name of the file to load.
 */
RcdFile::RcdFile(const char *fname)
{
	this->file_pos = 0;
	this->file_size = 0;
	this->fp = fopen(fname, "rb");
	if (this->fp == nullptr) return;

	fseek(this->fp, 0L, SEEK_END);
	this->file_size = ftell(this->fp);
	fseek(this->fp, 0L, SEEK_SET);
}

/** Destructor. */
RcdFile::~RcdFile()
{
	if (this->fp != nullptr) fclose(fp);
}

/**
 * Get length of data not yet read.
 * @return Count of remaining data.
 */
size_t RcdFile::GetRemaining()
{
	return (this->file_size >= this->file_pos) ? this->file_size - this->file_pos : 0;
}

/**
 * Read an 8 bits unsigned number.
 * @return Loaded number.
 * @pre File must be open, data must be available.
 */
uint8 RcdFile::GetUInt8()
{
	this->file_pos++;
	return fgetc(this->fp);
}

/**
 * Read an 8 bits signed number.
 * @return Loaded number.
 * @pre File must be open, data must be available.
 */
int8 RcdFile::GetInt8()
{
	return this->GetUInt8();
}

/**
 * Read an 16 bits unsigned number.
 * @return Loaded number.
 * @pre File must be open, data must be available.
 */
uint16 RcdFile::GetUInt16()
{
	uint16 val = this->GetUInt8();
	return val | (this->GetUInt8() << 8);
}

/**
 * Read an 16 bits signed number.
 * @return Loaded number.
 * @pre File must be open, data must be available.
 */
int16 RcdFile::GetInt16()
{
	uint16 val = this->GetUInt8();
	return val | (this->GetUInt8() << 8);
}

/**
 * Read an 32 bits unsigned number.
 * @return Loaded number.
 * @pre File must be open, data must be available.
 */
uint32 RcdFile::GetUInt32()
{
	uint32 val = this->GetUInt16();
	return val | (this->GetUInt16() << 16);
}

/**
 * Read an 32 bits signed number.
 * @return Loaded number.
 * @pre File must be open, data must be available.
 */
int32 RcdFile::GetInt32()
{
	uint32 val = this->GetUInt16();
	return val | (this->GetUInt16() << 16);
}

/**
 * Check whether the file header makes sense, and has the right version.
 * @param hdr_name Header name (should be 4 chars long).
 * @param version Header version.
 * @return The header seems correct.
 */
bool RcdFile::CheckFileHeader(const char *hdr_name, uint32 version)
{
	if (this->fp == nullptr) return false;
	if (this->GetRemaining() < 8) return false;

	char name[5];
	this->GetBlob(name, 4);
	name[4] = '\0';
	if (strcmp(name, hdr_name) != 0) return false;
	uint32 val = this->GetUInt32();
	return val == version;
}

/**
 * Get a blob of data from the file.
 * @param address Address to load into.
 * @param length Length of the data.
 * @return Loading was successful.
 */
bool RcdFile::GetBlob(void *address, size_t length)
{
	this->file_pos += length;
	return fread(address, length, 1, this->fp) == 1;
}

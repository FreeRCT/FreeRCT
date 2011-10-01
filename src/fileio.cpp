/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fileio.cpp File IO functions. */

#include "stdafx.h"
#include "fileio.h"
#include "string_func.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

static const int MAX_PATH = 512; ///< Max length of a file system path.

/**
 * Base class implementation of a directory reader, never returning any content.
 * Derive a new class for your operating system with more functionality.
 */
DirectoryReader::DirectoryReader()
{
}

/** Destructor. */
/* virtual */ DirectoryReader::~DirectoryReader()
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
 * @return Pointer to name of next entry (as a file path suitable for opening a file). Returns \c NULL if not next entry exists.
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
	} while(entry != NULL && !this->EntryIsFile());
	return entry;
}


/** Directory reader for a unix system. */
class UnixDirectoryReader : public DirectoryReader {
public:
	UnixDirectoryReader();
	virtual ~UnixDirectoryReader();

	virtual void OpenPath(const char *path);
	virtual const char *NextEntry();
	virtual void ClosePath();

	virtual bool EntryIsFile();
	virtual bool EntryIsDirectory();

private:
	DIR *dirfp;           ///< Directory stream if not \c NULL.
	dirent *entry;        ///< Pointer to current directory entry (or \c NULL).
	char dpath[MAX_PATH]; ///< Directorypath.
	char fpath[MAX_PATH]; ///< Filepath returned by #NextEntry.
};

UnixDirectoryReader::UnixDirectoryReader() : DirectoryReader()
{
	this->dirfp = NULL;
}

UnixDirectoryReader::~UnixDirectoryReader()
{
	this->ClosePath();
}

/* virtual */ void UnixDirectoryReader::OpenPath(const char *path)
{
	if (this->dirfp != NULL) this->ClosePath();

	SafeStrncpy(this->dpath, path, MAX_PATH);
	this->dirfp = opendir(this->dpath);
}

/* virtual */ const char *UnixDirectoryReader::NextEntry()
{
	if (this->dirfp == NULL) return NULL;

	this->entry = readdir(this->dirfp);
	if (this->entry == NULL) {
		this->ClosePath();
		return NULL;
	}

	snprintf(this->fpath, MAX_PATH, "%s/%s", this->dpath,  this->entry->d_name);
	this->fpath[MAX_PATH - 1] = '\0'; // Better safe than sorry.
	return this->fpath;
}

/* virtual */ void UnixDirectoryReader::ClosePath()
{
	if (this->dirfp != NULL) closedir(this->dirfp);
	this->dirfp = NULL;
}

/* virtual */ bool UnixDirectoryReader::EntryIsFile()
{
	struct stat st;

	if (stat(this->fpath, &st) != 0) return false;
	return S_ISREG(st.st_mode);
}

/* virtual */ bool UnixDirectoryReader::EntryIsDirectory()
{
	struct stat st;

	if (stat(this->fpath, &st) != 0) return false;
	return S_ISDIR(st.st_mode);
}

/**
 * Construct a directory reader object (specific for the operating system).
 * @return A directory reader.
 */
DirectoryReader *MakeDirectoryReader()
{
	return new UnixDirectoryReader();
}


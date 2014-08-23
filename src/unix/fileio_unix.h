/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fileio_unix.h File IO declarations for unix. */

#ifndef FILEIO_UNIX_H
#define FILEIO_UNIX_H

#include "../fileio.h"
#include "../stdafx.h"
#include <dirent.h>

/**
 * Directory reader for a Unix system.
 * @ingroup fileio_group
 */
class UnixDirectoryReader : public DirectoryReader {
public:
	UnixDirectoryReader();
	virtual ~UnixDirectoryReader();

	void OpenPath(const char *path) override;
	const char *NextEntry() override;
	void ClosePath() override;
	const char *MakePath(const char *directory, const char *fname) override;

	bool EntryIsFile() override;
	bool EntryIsDirectory() override;

private:
	DIR *dirfp;           ///< Directory stream if not \c nullptr.
	dirent *entry;        ///< Pointer to current directory entry (or \c nullptr).
	char dpath[MAX_PATH]; ///< Directory path.
	char fpath[MAX_PATH]; ///< File path returned by #NextEntry.
};

#endif

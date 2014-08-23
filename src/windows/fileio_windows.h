/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fileio_windows.h File IO declarations for windows. */

#ifndef FILEIO_WINDOWS_H
#define FILEIO_WINDOWS_H

#include "../fileio.h"
#include "../stdafx.h"
#include <string>
#include <windows.h>

/**
 * Directory reader for a Windows system.
 * @ingroup fileio_group
 */
class WindowsDirectoryReader : public DirectoryReader {
public:
	WindowsDirectoryReader();
	virtual ~WindowsDirectoryReader();

	void OpenPath(const char *path) override;
	const char *NextEntry() override;
	void ClosePath() override;
	const char *MakePath(const char *directory, const char *fname) override;

	bool EntryIsFile() override;
	bool EntryIsDirectory() override;

private:
	WIN32_FIND_DATA find_file_data; ///< Current directory entry
	HANDLE hfind; ///< Handle from FirstFindFile
	std::string dpath; ///< Directory path.
	char fpath[MAX_PATH]; ///< File path returned by #NextEntry.
};

#endif

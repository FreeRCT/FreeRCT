/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fileio_windows.cpp File IO Windows specific functions. */

/**
 * @defgroup fileio_group File IO
 */

#include "../stdafx.h"
#include "fileio_windows.h"
#include "../string_func.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <windows.h>
#include <string>

WindowsDirectoryReader::WindowsDirectoryReader() : DirectoryReader('\\')
{
	hfind = INVALID_HANDLE_VALUE;
}

WindowsDirectoryReader::~WindowsDirectoryReader()
{
	this->ClosePath();
}

void WindowsDirectoryReader::OpenPath(const char *path)
{
	if (this->hfind != INVALID_HANDLE_VALUE) this->ClosePath();

	this->dpath = path;
	this->hfind = FindFirstFile((this->dpath + "\\*").c_str(), &this->find_file_data);
}

const char *WindowsDirectoryReader::NextEntry()
{
	if (this->hfind == INVALID_HANDLE_VALUE) return nullptr;

	if (!FindNextFile(hfind, &find_file_data)) {
		this->ClosePath();
		return nullptr;
	}

	return this->MakePath(this->dpath.c_str(), this->find_file_data.cFileName);
}

const char *WindowsDirectoryReader::MakePath(const char *directory, const char *fname)
{	
	snprintf(this->fpath, MAX_PATH, "%s%c%s", directory, this->dir_sep, fname);
	this->fpath[MAX_PATH - 1] = '\0'; // Better safe than sorry.
	return this->fpath;
}

void WindowsDirectoryReader::ClosePath()
{
	if (this->hfind != INVALID_HANDLE_VALUE) {
		FindClose(this->hfind);
		this->hfind = INVALID_HANDLE_VALUE;
	}
}

bool WindowsDirectoryReader::EntryIsFile()
{
	return (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool WindowsDirectoryReader::EntryIsDirectory()
{
	return (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

/**
 * Test whether the given path points to a normal file.
 * @param path Path to investigate.
 * @return Whether the given path points to a normal file.
 */
bool PathIsFile(const char *path)
{
	DWORD attr = GetFileAttributes(path);
	return (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

/**
 * Test whether the given path points to a directory.
 * @param path Path to investigate.
 * @return Whether the given path points to a directory.
 */
bool PathIsDirectory(const char *path)
{
	DWORD attr = GetFileAttributes(path);
	return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}


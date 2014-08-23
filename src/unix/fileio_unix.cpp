/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fileio.cpp File IO Unix specific functions. */

/**
 * @defgroup fileio_group File IO
 */

#include "../stdafx.h"
#include "fileio_unix.h"
#include "../string_func.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

UnixDirectoryReader::UnixDirectoryReader() : DirectoryReader('/')
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

	return this->MakePath(this->dpath, this->entry->d_name);
}

const char *UnixDirectoryReader::MakePath(const char *directory, const char *fname)
{
	snprintf(this->fpath, MAX_PATH, "%s%c%s", directory, this->dir_sep, fname);
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
	return PathIsFile(this->fpath);
}

bool UnixDirectoryReader::EntryIsDirectory()
{
	return PathIsDirectory(this->fpath);
}

/**
 * Test whether the given path points to a normal file.
 * @param path Path to investigate.
 * @return Whether the given path points to a normal file.
 */
bool PathIsFile(const char *path)
{
	struct stat st;

	if (stat(path, &st) != 0) return false;
	return S_ISREG(st.st_mode);
}

/**
 * Test whether the given path points to a directory.
 * @param path Path to investigate.
 * @return Whether the given path points to a directory.
 */
bool PathIsDirectory(const char *path)
{
	struct stat st;

	if (stat(path, &st) != 0) return false;
	return S_ISDIR(st.st_mode);
}


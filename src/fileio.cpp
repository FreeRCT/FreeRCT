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

#include <cstdarg>
#include "stdafx.h"
#include "fileio.h"
#include "rev.h"
#ifdef LINUX
	#include <dirent.h>
	#include <errno.h>
	#include <unistd.h>
#elif WINDOWS
	#include <direct.h> // contains chdir in windows
#endif
#include <sys/types.h>

/**
 * Constructor.
 * @param fmt Error message (may use printf-style placeholders).
 */
LoadingError::LoadingError(const char *fmt, ...)
{
	char buffer[1024];
	va_list va;
	va_start(va, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);
	this->message = buffer;
}

/**
 * Retrieve the description of the error.
 * @return The error message.
 */
const char* LoadingError::what() const noexcept
{
	return this->message.c_str();
}

/**
 * Scan a given directory and return all entries.
 * @param path Directory to scan.
 */
std::vector<std::string> GetAllEntries(const std::string &path)
{
	std::vector<std::string> entries;
	if (!PathIsDirectory(path)) return entries;

	for (const auto &entry : std::filesystem::directory_iterator(path)) {
		std::string entryName = entry.path().string();
		entries.push_back(entryName);
	}
	return entries;
}

/**
 * Scan a given directory and return all file entries.
 * @param path Directory to scan.
 */
std::vector<std::string> GetAllFileEntries(const std::string &path)
{
	std::vector<std::string> entries;
	if (!PathIsDirectory(path)) return entries;

	for (const auto &entry : std::filesystem::directory_iterator(path)) {
		if (entry.is_regular_file()) {
			std::string entryName = entry.path().string();
			entries.push_back(entryName);
		}
	}
	return entries;
}


/**
 * Delete the given file.
 * @param path Path to delete.
 */
void RemoveFile(const std::string &path)
{

	if (!PathIsFile(path)) return;

	std::filesystem::remove(path);
}

/**
 * Test whether the given path points to a normal file.
 * @param path Path to investigate.
 * @return Whether the given path points to a normal file.
 */
bool PathIsFile(const std::string &path)
{
	std::filesystem::path fs(path);

	return std::filesystem::exists(fs) && std::filesystem::is_regular_file(fs);
}

/**
 * Test whether the given path points to a directory.
 * @param path Path to investigate.
 * @return Whether the given path points to a directory.
 */
bool PathIsDirectory(const std::string &path)
{
	std::filesystem::path fs(path);

	return std::filesystem::exists(fs) && std::filesystem::is_directory(fs);

}

/**
 * RCD file reader constructor, loading data from a file.
 * @param fname Name of the file to load.
 */
RcdFileReader::RcdFileReader(const std::string &fname)
: filename(fname), file_pos(0), file_size(0)
{
	this->name[4] = '\0';
	this->fp = fopen(fname.c_str(), "rb");
	if (this->fp == nullptr) return;

	if (fseek(this->fp, 0L, SEEK_END) != 0) {
		fclose(this->fp);
		return;
	}
	this->file_size = ftell(this->fp);
	if (fseek(this->fp, 0L, SEEK_SET) != 0) {
		fclose(this->fp);
		return;
	}
}

/** Destructor. */
RcdFileReader::~RcdFileReader()
{
	if (this->fp != nullptr) fclose(fp);
}

/**
 * Check whether the version of the current block is supported by this revision of FreeRCT, and throw an exception if this is not the case.
 * @param current_version The currently supported version.
 * @pre Must be inside a block.
 */
void RcdFileReader::CheckVersion(uint32 current_version)
{
	if (this->version != current_version) this->Error("Version mismatch: Found version %u, supported version is %u", this->version, current_version);
}

/**
 * Check whether the remaining length of the block is greater than or equal to a given minimum length, and throw an exception if this is not the case.
 * @param length Remaining length in the block.
 * @param required Required minimum length.
 * @param what Error description if the check fails.
 * @pre Must be inside a block.
 */
void RcdFileReader::CheckMinLength(int length, int required, const char *what)
{
	if (length < required) this->Error("Length too short for %s (at least %d bytes missing)", what, required - length);
}

/**
 * Check whether the remaining length of the block is equal to a given expected length, and throw an exception if this is not the case.
 * @param length Remaining length in the block.
 * @param required Expected remaining length.
 * @param what Error description if the check fails.
 * @pre Must be inside a block.
 */
void RcdFileReader::CheckExactLength(int length, int required, const char *what)
{
	if (length < required) this->Error("Length mismatch at %s (%d bytes missing)", what, required - length);
	if (length > required) this->Error("Length mismatch at %s (%d trailing bytes)", what, length - required);
}


/**
 * Get length of data not yet read.
 * @return Count of remaining data.
 */
size_t RcdFileReader::GetRemaining()
{
	return (this->file_size >= this->file_pos) ? this->file_size - this->file_pos : 0;
}

/**
 * Read an 8 bits unsigned number.
 * @return Loaded number.
 * @pre File must be open, data must be available.
 */
uint8 RcdFileReader::GetUInt8()
{
	this->file_pos++;
	return fgetc(this->fp);
}

/**
 * Read an 8 bits signed number.
 * @return Loaded number.
 * @pre File must be open, data must be available.
 */
int8 RcdFileReader::GetInt8()
{
	return this->GetUInt8();
}

/**
 * Read an 16 bits unsigned number.
 * @return Loaded number.
 * @pre File must be open, data must be available.
 */
uint16 RcdFileReader::GetUInt16()
{
	uint16 val = this->GetUInt8();
	return val | (this->GetUInt8() << 8);
}

/**
 * Read an 16 bits signed number.
 * @return Loaded number.
 * @pre File must be open, data must be available.
 */
int16 RcdFileReader::GetInt16()
{
	uint16 val = this->GetUInt8();
	return val | (this->GetUInt8() << 8);
}

/**
 * Read an 32 bits unsigned number.
 * @return Loaded number.
 * @pre File must be open, data must be available.
 */
uint32 RcdFileReader::GetUInt32()
{
	uint32 val = this->GetUInt16();
	return val | (this->GetUInt16() << 16);
}

/**
 * Read an 32 bits signed number.
 * @return Loaded number.
 * @pre File must be open, data must be available.
 */
int32 RcdFileReader::GetInt32()
{
	uint32 val = this->GetUInt16();
	return val | (this->GetUInt16() << 16);
}

/**
 * Read a nul-terminated string of unknown length.
 * @return Loaded string.
 * @pre File must be open, data must be available.
 */
std::string RcdFileReader::GetText()
{
	std::string result;
	for (;;) {
		char c = this->GetUInt8();
		if (c == '\0') break;
		result += c;
	}
	return result;
}

/**
 * Check whether the file header makes sense, and has the right version.
 * @param hdr_name Header name (should be 4 chars long).
 * @param version Header version.
 * @return The header seems correct.
 */
bool RcdFileReader::CheckFileHeader(const char *hdr_name, uint32 version)
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
 * Starting at the first byte of a block, read the block information, and put it in #name, #version, and #size.
 * @return Whether a block was found. If so, data is in #name, #version, and #size.
 */
bool RcdFileReader::ReadBlockHeader()
{
	if (this->GetRemaining() < 12) return false;
	this->GetBlob(this->name, 4);
	this->version = this->GetUInt32();
	this->size = this->GetUInt32();
	return this->file_pos + (size_t)this->size <= this->file_size;
}

/**
 * Skip a number of bytes in the file.
 * @param count Number of bytes to move forward.
 * @return Skipping was successful.
 */
bool RcdFileReader::SkipBytes(uint32 count)
{
	this->file_pos += count;
	if (this->file_pos > this->file_size) this->file_pos = this->file_size;
	return fseek(this->fp, this->file_pos, SEEK_SET) == 0;
}

/**
 * Get a blob of data from the file.
 * @param address Address to load into.
 * @param length Length of the data.
 * @return Loading was successful.
 */
bool RcdFileReader::GetBlob(void *address, size_t length)
{
	this->file_pos += length;
	return fread(address, length, 1, this->fp) == 1;
}

/**
 * Create a directory and all its parent directories if it did not exist yet.
 * @param path Path of the directory.
 * @todo At the time of writing (2021-06-30) this is tested only on Linux. Before using it anywhere else, test this on all platforms (especially Windows).
 */
void MakeDirectory(const std::string &path)
{
	if (path.empty() || PathIsDirectory(path)) return;

	bool check = std::filesystem::create_directories(path);
	if (!check) {
		error("Failed creating directory '%s'\n", path.c_str());
	}
}

/**
 * Copy a file.
 * @param src Source file.
 * @param dest Destination file.
 */
void CopyBinaryFile(const std::string &src, const std::string &dest)
{

	if (src.empty() || !PathIsFile(src)) {
		error("Path is empty or not a file: %s\n", src.c_str());
	}

	if (dest.empty() || !PathIsFile(dest)) {
		error("Path is empty or not a file: %s\n", dest.c_str());
	}

	std::filesystem::copy_file(src, dest, std::filesystem::copy_options::overwrite_existing);

}

/**
 * Locate the user's home directory. Failure is a fatal error.
 * @return User home directory.
 */
const std::string &GetUserHomeDirectory()
{
	static std::string homedir;
	if (!homedir.empty()) return homedir;

	for (const auto& var : {"HOME", "USERPROFILE", "HOMEPATH", "APPDATA"}) {
		const char *environment_variable = getenv(var);
		if (environment_variable != nullptr && environment_variable[0] != '\0') {
			homedir = environment_variable;
			return homedir;
		}
	}

	error("Unable to locate the user home directory. Set the HOME environment variable to fix the problem.\n");
}

/**
 * Locate a data file.
 * @param name Relative path of the file.
 * @return Actual path to the file.
 */
std::string FindDataFile(const std::string &name)
{
	for (std::string path : {std::string("."), std::string(".."), std::string("..") + DIR_SEP + "..", freerct_install_prefix()}) {
		path += DIR_SEP;
		path += name;
		if (PathIsFile(path)) return path;
	}
	error("Data file %s is missing, the installation seems to be broken!\n", name.c_str());
}

/**
 * Find the directory where the user's savegames are stored.
 * @return The directory path, with trailing directory separator.
 */
const std::string &SavegameDirectory()
{
	static std::string dir;
	if (dir.empty()) {
		dir = freerct_userdata_prefix();
		dir += DIR_SEP;
		dir += SAVEGAME_DIRECTORY;
		dir += DIR_SEP;
	}
	return dir;
}

/**
 * Find the directory where the user's track designs are stored.
 * @return The directory path, with trailing directory separator.
 */
const std::string &TrackDesignDirectory()
{
	static std::string dir;
	if (dir.empty()) {
		dir = freerct_userdata_prefix();
		dir += DIR_SEP;
		dir += TRACK_DESIGN_DIRECTORY;
		dir += DIR_SEP;
	}
	return dir;
}

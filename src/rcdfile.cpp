/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file rcdfile.cpp RCD file information and handling. */

#include "stdafx.h"
#include "rcdfile.h"
#include "fileio.h"
#include "string_func.h"

RcdFileCollection _rcd_collection; ///< Available RCD files.

/**
 * Constructor from its data elements.
 * @param path File path of the RCD file.
 * @param uri Uri string of the file.
 * @param build Build version of the file.
 */
RcdFileInfo::RcdFileInfo(const std::string &path, const std::string &uri, const std::string &build)
		: path(path), uri(uri), build(build)
{
}

/**
 * Copy constructor.
 * @param orig Existing instance to copy.
 */
RcdFileInfo::RcdFileInfo(const RcdFileInfo &orig) : path(orig.path), uri(orig.uri), build(orig.build)
{
}

/**
 * Assignment operator.
 * @param orig Existing instance to copy.
 * @return Assigned value.
 */
RcdFileInfo &RcdFileInfo::operator=(const RcdFileInfo &orig)
{
	if (&orig != this) {
		this->path = orig.path;
		this->uri = orig.uri;
		this->build = orig.build;
	}
	return *this;
}

RcdFileCollection::RcdFileCollection()
{
}

/**
 * Check whether the new file is useful to store in the available RCD files.
 * @param rcd New file to consider adding.
 */
void RcdFileCollection::AddFile(const RcdFileInfo &rcd)
{
	auto iter = rcdfiles.find(rcd.uri);
	if (iter == rcdfiles.end()) {
		rcdfiles.emplace(std::make_pair(rcd.uri, rcd));
	} else if (iter->second.build < rcd.build) {
		iter->second = rcd;
	}
}

/**
 * Paths to try looking for RCD files.
 * @todo Mke this configurable from cmake.
 */
static const char *_rcd_paths[] = {
	"rcd",
	nullptr,
};

/** Scan directories, looking for RCD files to add. */
void RcdFileCollection::ScanDirectories()
{
	DirectoryReader *reader = MakeDirectoryReader();

	const char **rcd_path = _rcd_paths;
	while (*rcd_path != nullptr) {
		reader->OpenPath(*rcd_path);
		for (;;) {
			const char *fname = reader->NextFile();
			if (fname == nullptr) break;
			if (!StrEndsWith(fname, ".rcd", false)) continue;
			this->ScanFileForMetaInfo(fname);
		}
		reader->ClosePath();
		rcd_path++;
	}
	delete reader;
}

/**
 * Read a nul-terminated string from the rcd file that takes a fixed maximum length.
 * Skip reading if \a remaining is #UINT32_MAX, set it to that value if there is an error in reading.
 * @param rcd_file Input rcd data stream.
 * @param max_bytes Maximum number of bytes available for the string, including the nul-terminating byte.
 * @param remaining [inout] Number of bytes available in the block. #UINT32_MAX means an error has happened.
 * @return The read utf-8 encoded string.
 */
static std::string GetString(RcdFileReader &rcd_file, uint max_bytes, uint32 *remaining)
{

	/* There needs to be at least one byte available for reading, and there must not be an error. */
	if (*remaining == 0 || *remaining == UINT32_MAX) {
		*remaining = UINT32_MAX;
		return "";
	}

	/* Temporary storage. */
	char buffer[512];
	if (max_bytes >= lengthof(buffer)) max_bytes = lengthof(buffer);

	uint8 byte;
	uint idx = 0;
	while (idx < max_bytes && *remaining > 0) {
		byte = rcd_file.GetUInt8();
		buffer[idx++] = byte;
		(*remaining)--;
		if (byte == 0) return buffer; // Seen the nul character.
	}
	/* Either the string is too long or there are no bytes left, report an error. */
	*remaining = UINT32_MAX;
	return "";
}

/**
 * Scan a file for Rcd meta-data, and add it to the collection if all is well.
 * @param fname Filename of the file to scan.
 * @return Error message, or \c nullptr if no error found.
 */
const char *RcdFileCollection::ScanFileForMetaInfo(const char *fname)
{
	RcdFileReader rcd_file(fname);
	if (!rcd_file.CheckFileHeader("RCDF", 2)) return "Wrong header";

	/* Load blocks. */
	for (uint blk_num = 1;; blk_num++) {
		if (!rcd_file.ReadBlockHeader()) break; // End reached.

		if (strcmp(rcd_file.name, "INFO") != 0) break; // Found a non-meta block, end scanning.

		/* Load INFO block. */
		if (rcd_file.version != 1) return "INFO block has wrong version";
		uint32 remaining = rcd_file.size;
		std::string build = GetString(rcd_file, 16, &remaining);
		std::string name = GetString(rcd_file, 64, &remaining);
		std::string uri = GetString(rcd_file, 128, &remaining);
		std::string website = GetString(rcd_file, 128, &remaining);
		std::string description = GetString(rcd_file, 512, &remaining);
		if (remaining != 0) return "Error while reading INFO text.";

		RcdFileInfo rfi(fname, uri, build);
		this->AddFile(rfi);
		return nullptr; // Success.
	}
	return "No INFO block found.";
}

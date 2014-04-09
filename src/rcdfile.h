/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file rcdfile.h RCD file handling. */

#ifndef RCDFILE_H
#define RCDFILE_H

#include <string>
#include <map>

/** Information about an RCD file. */
class RcdFileInfo {
public:
	RcdFileInfo(const std::string &path, const std::string &uri, const std::string &build);
	RcdFileInfo(const RcdFileInfo &orig);
	RcdFileInfo &operator=(const RcdFileInfo &orig);

	std::string path;  ///< Path to the file, utf-8 encoded.
	std::string uri;   ///< URI of the RCD file, utf-8 encoded.
	std::string build; ///< Build version, utf-8 encoded.
};

/** Collected RCD files. */
class RcdFileCollection {
public:
	RcdFileCollection();

	void ScanDirectories();
	void AddFile(const RcdFileInfo &rcd);

	std::map<std::string, RcdFileInfo> rcdfiles; ///< Found unique RCD files, mapping of uri to the Rcd file information.

private:
	const char *ScanFileForMetaInfo(const char *fname);
};

extern RcdFileCollection _rcd_collection;

#endif

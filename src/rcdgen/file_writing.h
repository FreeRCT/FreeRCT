/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file file_writing.h File writing declarations. */

#ifndef FILE_WRITING_H
#define FILE_WRITING_H

#include <map>
#include <memory>
#include <vector>

/** A block in an RCD file. See #StartSave for details on usage. */
class FileBlock {
public:
	FileBlock();
	~FileBlock();

	void StartSave(const char *blk_name, int version, int data_length);
	void SaveUInt8(uint8 d);
	void SaveInt8(int8 d);
	void SaveUInt16(uint16 d);
	void SaveInt16(int16 d);
	void SaveUInt32(uint32 d);
	void SaveInt32(int32 d);
	void SaveBytes(const uint8 *data, int size);
	void SaveText(const std::string &text);
	void CheckEndSave();

	void Write(FILE *fp);

	uint8 *data;    ///< Data of the block.
	int length;     ///< Length of the block.
	int save_index; ///< Index in #data to write content into the file block.
};

bool operator==(const FileBlock &fb1, const FileBlock &fb2);

/** Type definition for a list of file blocks. */
using FileBlockPtrList = std::vector<std::unique_ptr<FileBlock>>;

/** RCD output file. */
class FileWriter {
public:
	FileWriter() = default;
	~FileWriter() = default;

	int AddBlock(FileBlock *fb);

	void WriteFile(const std::string fname);

private:
	FileBlockPtrList blocks; ///< Blocks stored in the file so far.
	std::map<size_t, std::vector<std::pair<int, FileBlock*>>> blocks_by_length;  ///< All blocks with their index grouped by their length for faster searching.
};

#endif

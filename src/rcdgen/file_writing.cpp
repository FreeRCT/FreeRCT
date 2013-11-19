/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file file_writing.cpp File write code. */

#include "../stdafx.h"
#include "file_writing.h"


FileBlock::FileBlock()
{
	this->data = nullptr;
	this->length = 0;
}

FileBlock::~FileBlock()
{
	delete[] this->data;
}

/**
 * Set up storing of data to the file block.
 * Supply name of the block, version number, and expected data length (without the 12 byte header).
 * After setting up, use #SaveUInt8, #SaveUInt16, #SaveUInt32, and #SaveBytes to store the data in the block.
 * Afterwards, use #CheckEndSave to verify the amount of actually written data matches with the expected length.
 * @param blk_name Name of the block (a 4 character text string).
 * @param version Version of the block.
 * @param data_length Length of the data part (that is, excluding the header).
 */
void FileBlock::StartSave(const char *blk_name, int version, int data_length)
{
	this->length = data_length + 12; // Add length of the header.
	delete[] this->data;
	this->data = new uint8[this->length];
	this->save_index = 0;

	assert(strlen(blk_name) == 4);
	this->SaveBytes((uint8 *)blk_name, 4);
	this->SaveUInt32(version);
	this->SaveUInt32(data_length);
}

/**
 * Save a 8 bit unsigned value into the file block.
 * @param d Value to write.
 */
void FileBlock::SaveUInt8(uint8 d)
{
	assert(this->save_index < this->length);
	this->data[this->save_index] = d;
	this->save_index++;
}

/**
 * Save a 8 bit signed value into the file block.
 * @param d Value to write.
 */
void FileBlock::SaveInt8(int8 d)
{
	this->SaveUInt8(d);
}

/**
 * Save a 16 bit unsigned value into the file block.
 * @param d Value to write.
 */
void FileBlock::SaveUInt16(uint16 d)
{
	this->SaveUInt8(d);
	this->SaveUInt8(d >> 8);
}
/**
 * Save a 16 bit signed value into the file block.
 * @param d Value to write.
 */
void FileBlock::SaveInt16(uint16 d)
{
	this->SaveUInt8(d);
	this->SaveUInt8(d >> 8);
}

/**
 * Save a 32 bit unsigned value into the file block.
 * @param d Value to write.
 */
void FileBlock::SaveUInt32(uint32 d)
{
	this->SaveUInt16(d);
	this->SaveUInt16(d >> 16);
}

/**
 * Save a sequence of bytes in the file block.
 * @param data Start address of the data.
 * @param size Length of the data.
 */
void FileBlock::SaveBytes(uint8 *data, int size)
{
	while (size > 0) {
		this->SaveUInt8(*data);
		data++;
		size--;
	}
}

/** Check that all data has been written. */
void FileBlock::CheckEndSave()
{
	assert(this->save_index == this->length);
}

/**
 * Write the file block to the output.
 * @param fp File handle to write to.
 */
void FileBlock::Write(FILE *fp)
{
	if (this->length == 0) return;

	if ((int)fwrite(this->data, 1, this->length, fp) != this->length) {
		fprintf(stderr, "Failed to write RCD block\n");
		exit(1);
	}
}

/**
 * Check whether two file blocks are identical.
 * @param fb1 First block to compare.
 * @param fb2 Second block to compare.
 * @return Whether both blocks are the same.
 */
bool operator==(const FileBlock &fb1, const FileBlock &fb2)
{
	if (fb1.length != fb2.length) return false;
	if (fb1.length == 0) return true;
	return memcmp(fb1.data, fb2.data, fb1.length) == 0;
}

FileWriter::FileWriter()
{
}

FileWriter::~FileWriter()
{
	for (auto &iter : this->blocks) delete iter;
}

/**
 * Add a block to the file.
 * @param blk Block to add.
 * @return Block index number where the block is stored in the file.
 */
int FileWriter::AddBlock(FileBlock *blk)
{
	int idx = 1;
	for (auto &iter : this->blocks) {
		/* Block already added, just return the old block number. */
		if (*iter == *blk) {
			delete blk;
			return idx;
		}
		idx++;
	}
	this->blocks.push_back(blk);
	return idx;
}

/**
 * Write all blocks of the RCD file to the file.
 * @param fname Filename to create.
 */
void FileWriter::WriteFile(std::string fname)
{
	FILE *fp = fopen(fname.c_str(), "wb");
	if (fp == nullptr) {
		fprintf(stderr, "Failed to open \"%s\" for writing.", fname.c_str());
		exit(1);
	}

	static const uint8 file_header[8] = {'R', 'C', 'D', 'F', 1, 0, 0, 0};
	if (fwrite(file_header, 1, 8, fp) != 8) {
		fprintf(stderr, "Failed to write the RCD file header of \"%s\".", fname.c_str());
		exit(1);
	}

	for (auto &iter : this->blocks) iter->Write(fp);

	fclose(fp);
}

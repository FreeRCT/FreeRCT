/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sprite_data.h Code for sprite data. */

#include "stdafx.h"
#include "sprite_data.h"
#include "fileio.h"
#include "palette.h"
#include "bitmath.h"

#include <vector>

static const int MAX_IMAGE_COUNT  = 5000; ///< Maximum number of images that can be loaded (arbitrary number).

static std::vector<ImageData> _sprites;   ///< Available sprites to the program.

ImageData::ImageData()
{
	this->width = 0;
	this->height = 0;
	this->table = nullptr;
	this->data = nullptr;
}

ImageData::~ImageData()
{
	delete[] this->table;
	delete[] this->data;
}

/**
 * Load image data from the RCD file.
 * @param rcd_file File to load from.
 * @param length Length of the image data block.
 * @return Load was successful.
 * @pre File pointer is at first byte of the block.
 */
bool ImageData::Load8bpp(RcdFile *rcd_file, size_t length)
{
	if (length < 8) return false; // 2 bytes width, 2 bytes height, 2 bytes x-offset, and 2 bytes y-offset
	this->width  = rcd_file->GetUInt16();
	this->height = rcd_file->GetUInt16();
	this->xoffset = rcd_file->GetInt16();
	this->yoffset = rcd_file->GetInt16();

	/* Check against some arbitrary limits that look sufficient at this time. */
	if (this->width == 0 || this->width > 300 || this->height == 0 || this->height > 500) return false;

	length -= 8;
	if (length > 100*1024) return false; // Another arbitrary limit.

	size_t jmp_table = 4 * this->height;
	if (length <= jmp_table) return false; // You need at least place for the jump table.
	length -= jmp_table;

	this->table = new uint32[jmp_table / 4];
	this->data  = new uint8[length];
	if (this->table == nullptr || this->data == nullptr) return false;

	/* Load jump table, adjusting the entries while loading. */
	for (uint i = 0; i < this->height; i++) {
		uint32 dest = rcd_file->GetUInt32();
		if (dest == 0) {
			this->table[i] = INVALID_JUMP;
			continue;
		}
		dest -= jmp_table;
		if (dest >= length) return false;
		this->table[i] = dest;
	}

	rcd_file->GetBlob(this->data, length); // Load the image data.

	/* Verify the image data. */
	for (uint i = 0; i < this->height; i++) {
		uint32 offset = this->table[i];
		if (offset == INVALID_JUMP) continue;

		uint32 xpos = 0;
		for (;;) {
			if (offset + 2 >= length) return false;
			uint8 rel_pos = this->data[offset];
			uint8 count = this->data[offset + 1];
			xpos += (rel_pos & 127) + count;
			offset += 2 + count;
			if ((rel_pos & 128) == 0) {
				if (xpos >= this->width || offset >= length) return false;
			} else {
				if (xpos > this->width || offset > length) return false;
				break;
			}
		}
	}
	return true;
}

/**
 * Return the pixel-value of the provided position.
 * @param xoffset Horizontal offset in the sprite.
 * @param yoffset Vertical offset in the sprite.
 * @return Pixel value at the given position, or \c 0 if transparent.
 */
uint32 ImageData::GetPixel(uint16 xoffset, uint16 yoffset) const
{
	if (xoffset >= this->width) return _palette[0];
	if (yoffset >= this->height) return _palette[0];

	if (GB(this->flags, IFG_IS_8BPP, 1) != 0) {
		/* 8bpp image. */
		uint32 offset = this->table[yoffset];
		if (offset == INVALID_JUMP) return _palette[0];

		uint16 xpos = 0;
		while (xpos < xoffset) {
			uint8 rel_pos = this->data[offset];
			uint8 count = this->data[offset + 1];
			xpos += (rel_pos & 127);
			if (xpos > xoffset) return _palette[0];
			if (xoffset - xpos < count) return _palette[this->data[offset + 2 + xoffset - xpos]];
			xpos += count;
			offset += 2 + count;
			if ((rel_pos & 128) != 0) break;
		}
		return _palette[0];
	} else {
		return _palette[0]; // Temporary place holder.
	}
}

/**
 * Load 8bpp sprite block (8PXL, version 2) from the \a rcd_file.
 * @param rcd_file File being loaded.
 * @param length Length of the 8PXL block.
 * @param is_8bpp Whether the block being loaded is a 8PXL block (else it is assumed to be a 32PX block).
 * @return Loaded sprite, if loading was successful, else \c nullptr.
 */
ImageData *LoadImage(RcdFile *rcd_file, size_t length, bool is_8bpp)
{
	_sprites.emplace_back();
	ImageData *imd = &_sprites.back();
	bool loaded = imd->Load8bpp(rcd_file, length);
	if (!loaded) {
		_sprites.pop_back();
		return nullptr;
	}
	imd->flags = is_8bpp ? (1 << IFG_IS_8BPP) : 0;
	return imd;
}

/** Initialize image storage. */
void InitImageStorage()
{
	_sprites.reserve(MAX_IMAGE_COUNT);
}

/** Clear all memory. */
void DestroyImageStorage()
{
	_sprites.clear();
}

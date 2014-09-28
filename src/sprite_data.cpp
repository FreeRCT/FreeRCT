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

static const int MAX_IMAGE_COUNT = 5000; ///< Maximum number of images that can be loaded (arbitrary number).

static std::vector<ImageData> _sprites;  ///< Available sprites to the program.

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
bool ImageData::Load8bpp(RcdFileReader *rcd_file, size_t length)
{
	if (length < 8) return false; // 2 bytes width, 2 bytes height, 2 bytes x-offset, and 2 bytes y-offset
	this->width  = rcd_file->GetUInt16();
	this->height = rcd_file->GetUInt16();
	this->xoffset = rcd_file->GetInt16();
	this->yoffset = rcd_file->GetInt16();

	/* Check against some arbitrary limits that look sufficient at this time. */
	if (this->width == 0 || this->width > 300 || this->height == 0 || this->height > 500) return false;

	length -= 8;
	if (length > 100 * 1024) return false; // Another arbitrary limit.

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
 * Load a 32bpp image.
 * @param rcd_file Input stream to read from.
 * @param length Length of the 32bpp block.
 * @return Exeit code, \0 means ok, every other number indicates an error.
 */
bool ImageData::Load32bpp(RcdFileReader *rcd_file, size_t length)
{
	if (length < 8) return false; // 2 bytes width, 2 bytes height, 2 bytes x-offset, and 2 bytes y-offset
	this->width  = rcd_file->GetUInt16();
	this->height = rcd_file->GetUInt16();
	this->xoffset = rcd_file->GetInt16();
	this->yoffset = rcd_file->GetInt16();

	/* Check against some arbitrary limits that look sufficient at this time. */
	if (this->width == 0 || this->width > 300 || this->height == 0 || this->height > 500) return false;

	length -= 8;
	if (length > 100 * 1024) return false; // Another arbitrary limit.

	/* Allocate and load the image data. */
	this->data = new uint8[length];
	if (this->data == nullptr) return false;
	rcd_file->GetBlob(this->data, length);

	/* Verify the data. */
	uint8 *abs_end = this->data + length;
	int line_count = 0;
	const uint8 *ptr = this->data;
	bool finished = false;
	while (ptr < abs_end && !finished) {
		line_count++;

		/* Find end of this line. */
		uint16 line_length = ptr[0] | (ptr[1] << 8);
		const uint8 *end;
		if (line_length == 0) {
			finished = true;
			end = abs_end;
		} else {
			end = ptr + line_length;
			if (end > abs_end) return false;
		}
		ptr += 2;

		/* Read line. */
		bool finished_line = false;
		uint xpos = 0;
		while (ptr < end && !finished_line) {
			uint8 mode = *ptr++;
			if (mode == 0) {
				finished_line = true;
				break;
			}
			xpos += mode & 0x3F;
			switch (mode >> 6) {
				case 0: ptr += 3 * (mode & 0x3F); break;
				case 1: ptr += 1 + 3 * (mode & 0x3F); break;
				case 2: break;
				case 3: ptr += 1 + 1 + (mode & 0x3F); break;
			}
		}
		if (xpos > this->width) return false;
		if (!finished_line) return false;
		if (ptr != end) return false;
	}
	if (line_count != this->height) return false;
	if (ptr != abs_end) return false;
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
		while (xpos <= xoffset) {
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
		/* 32bpp image. */
		const uint8 *ptr = this->data;
		while (yoffset > 0) {
			uint16 length = ptr[0] | (ptr[1] << 8);
			ptr += length;
			yoffset--;
		}
		ptr += 2;
		while (xoffset > 0) {
			uint8 mode = *ptr++;
			if (mode == 0) break;
			if ((mode & 0x3F) < xoffset) {
				xoffset -= mode & 0x3F;
				switch (mode >> 6) {
					case 0: ptr += 3 * (mode & 0x3F); break;
					case 1: ptr += 1 + 3 * (mode & 0x3F); break;
					case 2: ptr++; break;
					case 3: ptr += 1 + 1 + (mode & 0x3F); break;
				}
			} else {
				switch (mode >> 6) {
					case 0:
						ptr += 3 * xoffset;
						return MakeRGBA(ptr[0], ptr[1], ptr[2], OPAQUE);
					case 1: {
						uint8 opacity = *ptr;
						ptr += 1 + 3 * xoffset;
						return MakeRGBA(ptr[0], ptr[1], ptr[2], opacity);
					}
					case 2:
						return _palette[0]; // Arbitrary fully transparent.
					case 3: {
						uint8 opacity = ptr[1];
						return MakeRGBA(0, 0, 0, opacity); // Arbitrary colour with the correct opacity.
					}
				}
			}
		}
		return _palette[0]; // Arbitrary fully transparent.
	}
}

/**
 * Load 8bpp or 32bpp sprite block from the \a rcd_file.
 * @param rcd_file File being loaded.
 * @return Loaded sprite, if loading was successful, else \c nullptr.
 */
ImageData *LoadImage(RcdFileReader *rcd_file)
{
	bool is_8bpp = strcmp(rcd_file->name, "8PXL") == 0;
	if (rcd_file->version != (is_8bpp ? 2 : 1)) return nullptr;

	_sprites.emplace_back();
	ImageData *imd = &_sprites.back();
	bool loaded = is_8bpp ? imd->Load8bpp(rcd_file, rcd_file->size) : imd->Load32bpp(rcd_file, rcd_file->size);
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

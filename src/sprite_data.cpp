/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sprite_data.cpp Code for sprite data. */

#include "stdafx.h"
#include "palette.h"
#include "sprite_data.h"
#include "fileio.h"
#include "bitmath.h"

#include <vector>

static const uint32 MAX_IMAGE_COUNT = 20000;  ///< Maximum number of images that can be loaded (arbitrary number).

static std::vector<ImageData> _sprites;  ///< Available sprites to the program.
static uint32 _sprites_loaded;           ///< Total number of sprites loaded.

ImageData::ImageData() : width(0), height(0), table(nullptr), data(nullptr)
{
}

/**
 * Load image data from the RCD file.
 * @param rcd_file File to load from.
 * @param length Length of the image data block.
 * @pre File pointer is at first byte of the block.
 */
void ImageData::Load8bpp(RcdFileReader *rcd_file, size_t length)
{
	rcd_file->CheckMinLength(length, 8, "8bpp header"); // 2 bytes width, 2 bytes height, 2 bytes x-offset, and 2 bytes y-offset
	this->width  = rcd_file->GetUInt16();
	this->height = rcd_file->GetUInt16();
	this->xoffset = rcd_file->GetInt16();
	this->yoffset = rcd_file->GetInt16();

	/* Check against some arbitrary limits that look sufficient at this time. */
	if (this->width == 0 || this->width > 300 || this->height == 0 || this->height > 500) rcd_file->Error("Size out of bounds");

	length -= 8;
	if (length > 100 * 1024) rcd_file->Error("Data too long"); // Another arbitrary limit.

	size_t jmp_table = 4 * this->height;
	if (length <= jmp_table) rcd_file->Error("Jump table too short"); // You need at least place for the jump table.
	length -= jmp_table;

	this->table.reset(new uint32[jmp_table / 4]);
	this->data.reset(new uint8[length]);
	if (this->table == nullptr || this->data == nullptr) rcd_file->Error("Out of memory");

	/* Load jump table, adjusting the entries while loading. */
	for (uint i = 0; i < this->height; i++) {
		uint32 dest = rcd_file->GetUInt32();
		if (dest == 0) {
			this->table[i] = INVALID_JUMP;
			continue;
		}
		dest -= jmp_table;
		if (dest >= length) rcd_file->Error("Jump destination out of bounds");
		this->table[i] = dest;
	}

	rcd_file->GetBlob(this->data.get(), length); // Load the image data.

	/* Verify the image data. */
	for (uint i = 0; i < this->height; i++) {
		uint32 offset = this->table[i];
		if (offset == INVALID_JUMP) continue;

		uint32 xpos = 0;
		for (;;) {
			if (offset + 2 >= length) rcd_file->Error("Offset out of bounds");
			uint8 rel_pos = this->data[offset];
			uint8 count = this->data[offset + 1];
			xpos += (rel_pos & 127) + count;
			offset += 2 + count;
			if ((rel_pos & 128) == 0) {
				if (xpos >= this->width || offset >= length) rcd_file->Error("X coordinate out of exclusive bounds");
			} else {
				if (xpos > this->width || offset > length) rcd_file->Error("X coordinate out of inclusive bounds");
				break;
			}
		}
	}
}

/**
 * Load a 32bpp image.
 * @param rcd_file Input stream to read from.
 * @param length Length of the 32bpp block.
 */
void ImageData::Load32bpp(RcdFileReader *rcd_file, size_t length)
{
	rcd_file->CheckMinLength(length, 8, "32bpp header");  // 2 bytes width, 2 bytes height, 2 bytes x-offset, and 2 bytes y-offset
	this->width  = rcd_file->GetUInt16();
	this->height = rcd_file->GetUInt16();
	this->xoffset = rcd_file->GetInt16();
	this->yoffset = rcd_file->GetInt16();

	/* Check against some arbitrary limits that look sufficient at this time. */
	if (this->width == 0 || this->width > 2000 || this->height == 0 || this->height > 1200) rcd_file->Error("Size out of bounds");

	length -= 8;
	if (length > 2000 * 1200) rcd_file->Error("Data too long"); // Another arbitrary limit.

	/* Allocate and load the image data. */
	this->data.reset(new uint8[length]);
	if (this->data == nullptr) rcd_file->Error("Out of memory");
	rcd_file->GetBlob(this->data.get(), length);

	/* Verify the data. */
	uint8 *abs_end = this->data.get() + length;
	int line_count = 0;
	const uint8 *ptr = this->data.get();
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
			if (end > abs_end) rcd_file->Error("End out of bounds");
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
		if (xpos > this->width) rcd_file->Error("X coordinate out of bounds");
		if (!finished_line) rcd_file->Error("Incomplete line");
		if (ptr != end) rcd_file->Error("Trailing bytes at end of line");
	}
	if (line_count != this->height) rcd_file->Error("Line count mismatch");
	if (ptr != abs_end) rcd_file->Error("Trailing bytes at end of file");
}

/**
 * Return the pixel-value of the provided position.
 * @param xoffset Horizontal offset in the sprite.
 * @param yoffset Vertical offset in the sprite.
 * @param recolour Recolouring to apply to the retrieved pixel. Use \c nullptr for disabling recolouring.
 * @param shift Gradient shift to apply to the retrieved pixel. Use #GS_NORMAL for not shifting the colour.
 * @return Pixel value at the given position, or \c 0 if transparent.
 */
uint32 ImageData::GetPixel(uint16 xoffset, uint16 yoffset, const Recolouring *recolour, GradientShift shift) const
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
			if (xoffset - xpos < count) {
				uint8 pixel = this->data[offset + 2 + xoffset - xpos];
				if (recolour != nullptr) {
					const uint8 *recolour_table = recolour->GetPalette(shift);
					pixel = recolour_table[pixel];
				}
				return _palette[pixel];
			}
			xpos += count;
			offset += 2 + count;
			if ((rel_pos & 128) != 0) break;
		}
		return _palette[0];
	} else {
		/* 32bpp image. */
		const uint8 *ptr = this->data.get();
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
				ShiftFunc sf = GetGradientShiftFunc(shift);
				switch (mode >> 6) {
					case 0:
						ptr += 3 * xoffset;
						return MakeRGBA(sf(ptr[0]), sf(ptr[1]), sf(ptr[2]), OPAQUE);
					case 1: {
						uint8 opacity = *ptr;
						ptr += 1 + 3 * xoffset;
						return MakeRGBA(sf(ptr[0]), sf(ptr[1]), sf(ptr[2]), opacity);
					}
					case 2:
						return _palette[0]; // Arbitrary fully transparent.
					case 3: {
						uint8 opacity = ptr[1];
						if (recolour == nullptr) return MakeRGBA(0, 0, 0, opacity); // Arbitrary colour with the correct opacity.
						const uint32 *table = recolour->GetRecolourTable(ptr[0] - 1);
						ptr += 2 + xoffset;
						uint32 recoloured = table[*ptr];
						return MakeRGBA(sf(GetR(recoloured)), sf(GetG(recoloured)), sf(GetB(recoloured)), opacity);
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
	if (_sprites_loaded >= MAX_IMAGE_COUNT) {
		error("Attempt to load too many sprites! MAX_IMAGE_COUNT needs to be increased.\n");
	}
	bool is_8bpp = strcmp(rcd_file->name, "8PXL") == 0;
	rcd_file->CheckVersion(is_8bpp ? 2 : 1);

	_sprites.emplace_back();
	_sprites_loaded++;
	ImageData *imd = &_sprites.back();
	try {
		if (is_8bpp) {
			imd->Load8bpp(rcd_file, rcd_file->size);
		} else {
			imd->Load32bpp(rcd_file, rcd_file->size);
		}
	} catch (...) {
		_sprites.pop_back();
		_sprites_loaded--;
		throw;
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

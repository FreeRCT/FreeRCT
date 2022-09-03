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

ImageData::ImageData() : is_8bpp(false), width(0), height(0)
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

	std::unique_ptr<uint32[]> table(new uint32[jmp_table / 4]);
	std::unique_ptr<uint8[]> data(new uint8[length]);
	if (table == nullptr || data == nullptr) rcd_file->Error("Out of memory");

	/* Load jump table, adjusting the entries while loading. */
	for (uint i = 0; i < this->height; i++) {
		uint32 dest = rcd_file->GetUInt32();
		if (dest == 0) {
			table[i] = INVALID_JUMP;
			continue;
		}
		dest -= jmp_table;
		if (dest >= length) rcd_file->Error("Jump destination out of bounds");
		table[i] = dest;
	}

	rcd_file->GetBlob(data.get(), length); // Load the image data.

	/* Verify the image data. */
	this->rgba.reset(new uint8[this->width * this->height * 4]);
	this->recol.reset(new uint8[this->width * this->height]);
	for (uint i = 0; i < this->height; i++) {
		uint32 offset = table[i];
		if (offset == INVALID_JUMP) {
			/* Whole line is transparent. */
			for (int x = 0; x < 4 * this->width; ++x) this->rgba[this->width * i * 4 + x] = 255;
			for (int x = 0; x < this->width; ++x) this->recol[this->width * i + x] = 0;
			continue;
		}

		uint32 xpos = 0;
		for (;;) {
			if (offset + 2 >= length) rcd_file->Error("Offset out of bounds");
			uint8 rel_pos = data[offset];
			uint8 count = data[offset + 1];
			xpos += (rel_pos & 127) + count;
			for (int dx = 0; dx < count; ++dx) {
				uint8 pixel = data[offset + 2 + dx];
				uint32 rgba = _palette[pixel];
				this->recol[this->width * i + dx] = pixel;
				this->rgba[4 * (this->width * i + dx) + 0] = (rgba >> 24) & 0xff;
				this->rgba[4 * (this->width * i + dx) + 1] = (rgba >> 16) & 0xff;
				this->rgba[4 * (this->width * i + dx) + 2] = (rgba >>  8) & 0xff;
				this->rgba[4 * (this->width * i + dx) + 3] = (rgba      ) & 0xff;
			}
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
	std::unique_ptr<uint8[]> data(new uint8[length]);
	if (data == nullptr) rcd_file->Error("Out of memory");
	rcd_file->GetBlob(data.get(), length);

	/* Verify the data. */
	this->rgba.reset(new uint8[this->width * this->height * 4]);
	this->recol.reset(new uint8[this->width * this->height * 2]);
	uint8 *rgba_ptr = this->rgba.get();
	uint8 *recol_ptr = this->recol.get();
	uint8 *abs_end = data.get() + length;
	int line_count = 0;
	const uint8 *ptr = data.get();
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
				case 0:
					for (int i = mode & 0x3F; i > 0; --i) {
						*(rgba_ptr++) = *(ptr++);
						*(rgba_ptr++) = *(ptr++);
						*(rgba_ptr++) = *(ptr++);
						*(rgba_ptr++) = 255;
						*(recol_ptr++) = 0;
						*(recol_ptr++) = 0;
					}
					break;
				case 1: {
					uint8 alpha = *(ptr++);
					for (int i = mode & 0x3F; i > 0; --i) {
						*(rgba_ptr++) = *(ptr++);
						*(rgba_ptr++) = *(ptr++);
						*(rgba_ptr++) = *(ptr++);
						*(rgba_ptr++) = alpha;
						*(recol_ptr++) = 0;
						*(recol_ptr++) = 0;
					}
					break;
				}
				case 2:
					for (int i = mode & 0x3F; i > 0; --i) {
						*(rgba_ptr++) = 255;
						*(rgba_ptr++) = 255;
						*(rgba_ptr++) = 255;
						*(rgba_ptr++) = 255;
						*(recol_ptr++) = 0;
						*(recol_ptr++) = 0;
					}
					break;
				case 3: {
					uint8 layer = *(ptr++);
					uint8 alpha = *(ptr++);
					for (int i = mode & 0x3F; i > 0; --i) {
						*(rgba_ptr++) = 0;
						*(rgba_ptr++) = 0;
						*(rgba_ptr++) = 0;
						*(rgba_ptr++) = alpha;
						*(recol_ptr++) = layer;
						*(recol_ptr++) = *(ptr++);
					}
					break;
				}
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
	ShiftFunc sf = GetGradientShiftFunc(shift);
	const uint8 *rgba_base = &this->rgba[4 * (yoffset * this->width + xoffset)];
	const uint8 *recol_base = &this->recol[(this->is_8bpp ? 1 : 2) * (yoffset * this->width + xoffset)];

	if (recol_base == nullptr || recolour == nullptr || recol_base[0] == 0) {
		/* No recolouring, */
		return sf(rgba_base[0] << 24) | sf(rgba_base[1] << 16) | sf(rgba_base[2] << 8) | sf(rgba_base[3]);
	}

	if (this->is_8bpp) {
		return recolour->GetPalette(shift)[recol_base[0]];
	}

	const uint32 recoloured = recolour->GetRecolourTable(recol_base[0] - 1)[recol_base[1]];
	return MakeRGBA(sf(GetR(recoloured)), sf(GetG(recoloured)), sf(GetB(recoloured)), rgba_base[3]);
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
			imd->is_8bpp = true;
			imd->Load8bpp(rcd_file, rcd_file->size);
		} else {
			imd->is_8bpp = false;
			imd->Load32bpp(rcd_file, rcd_file->size);
		}
	} catch (...) {
		_sprites.pop_back();
		_sprites_loaded--;
		throw;
	}
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

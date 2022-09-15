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

#include <cmath>
#include <vector>

static const uint32 IMAGE_BATCH_SIZE = 1024;  ///< Number of images that are batch-preallocated (arbitrary number).

static std::vector<std::unique_ptr<ImageData[]>> _sprites;  ///< Available sprites to the program.
static uint32 _sprites_loaded;                              ///< Total number of sprites loaded.

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
	uint8 *rgba_ptr = this->rgba.get();
	uint8 *recol_ptr = this->recol.get();
	for (uint i = 0; i < this->height; i++) {
		uint32 offset = table[i];
		if (offset == INVALID_JUMP) {
			/* Whole line is transparent. */
			for (int x = 0; x < this->width; ++x) {
				*(rgba_ptr++) = 0;
				*(rgba_ptr++) = 0;
				*(rgba_ptr++) = 0;
				*(rgba_ptr++) = 0;
				*(recol_ptr++) = 0;
			}
			continue;
		}

		uint32 xpos = 0;
		for (;;) {
			if (offset + 2 >= length) rcd_file->Error("Offset out of bounds");
			uint8 rel_pos = data[offset];
			uint8 count = data[offset + 1];
			for (int g = 0; g < (rel_pos & 127); ++g) {
				*(rgba_ptr++) = 0;
				*(rgba_ptr++) = 0;
				*(rgba_ptr++) = 0;
				*(rgba_ptr++) = 0;
				*(recol_ptr++) = 0;
			}
			xpos += (rel_pos & 127) + count;
			for (int dx = 0; dx < count; ++dx) {
				uint8 pixel = data[offset + 2 + dx];
				*(recol_ptr++) = pixel;
				uint32 rgba = _palette[pixel];
				*(rgba_ptr++) = (rgba >> 24) & 0xff;
				*(rgba_ptr++) = (rgba >> 16) & 0xff;
				*(rgba_ptr++) = (rgba >>  8) & 0xff;
				*(rgba_ptr++) = (rgba      ) & 0xff;
			}
			offset += 2 + count;
			if ((rel_pos & 128) == 0) {
				if (xpos >= this->width || offset >= length) rcd_file->Error("X coordinate out of exclusive bounds");
			} else {
				if (xpos > this->width || offset > length) rcd_file->Error("X coordinate out of inclusive bounds");
				break;
			}
		}

		for (; xpos < this->width; ++xpos) {
			*(rgba_ptr++) = 0;
			*(rgba_ptr++) = 0;
			*(rgba_ptr++) = 0;
			*(rgba_ptr++) = 0;
			*(recol_ptr++) = 0;
		}
	}
	assert(recol_ptr - this->recol.get() == 1L * this->width * this->height);
	assert(rgba_ptr - this->rgba.get() == 4L * this->width * this->height);
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
				for (; xpos < this->width; ++xpos) {
					*(rgba_ptr++) = 0;
					*(rgba_ptr++) = 0;
					*(rgba_ptr++) = 0;
					*(rgba_ptr++) = 0;
					*(recol_ptr++) = 0;
					*(recol_ptr++) = 0;
				}
				finished_line = true;
				break;
			}
			xpos += mode & 0x3F;
			switch (mode >> 6) {
				case 0:  // Fully opaque colour.
					for (int i = mode & 0x3F; i > 0; --i) {
						*(rgba_ptr++) = *(ptr++);
						*(rgba_ptr++) = *(ptr++);
						*(rgba_ptr++) = *(ptr++);
						*(rgba_ptr++) = 255;
						*(recol_ptr++) = 0;
						*(recol_ptr++) = 0;
					}
					break;
				case 1: {  // Semi-transparent colour.
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
				case 2:  // Fully transparent.
					for (int i = mode & 0x3F; i > 0; --i) {
						*(rgba_ptr++) = 0;
						*(rgba_ptr++) = 0;
						*(rgba_ptr++) = 0;
						*(rgba_ptr++) = 0;
						*(recol_ptr++) = 0;
						*(recol_ptr++) = 0;
					}
					break;
				case 3: {  // Recolour layer.
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
	assert(recol_ptr - this->recol.get() == 2L * this->width * this->height);
	assert(rgba_ptr - this->rgba.get() == 4L * this->width * this->height);
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
	if (xoffset >= this->width || yoffset >= this->height) return 0;
	if (this->is_8bpp) {
		uint8 pixel = this->recol[yoffset * this->width + xoffset];
		if (recolour != nullptr) pixel = recolour->GetPalette(shift)[pixel];
		return _palette[pixel];
	}

	const uint8 *rgba_base = &this->rgba[4 * (yoffset * this->width + xoffset)];
	const uint8 *recol_base = &this->recol[2 * (yoffset * this->width + xoffset)];
	ShiftFunc sf = GetGradientShiftFunc(shift);
	ShiftFunc af = GetAlphaShiftFunc(shift);

	if (recol_base == nullptr || recolour == nullptr || recol_base[0] == 0) {
		/* No recolouring, */
		return sf(rgba_base[0] << 24) | sf(rgba_base[1] << 16) | sf(rgba_base[2] << 8) | af(rgba_base[3]);
	}

	const uint32 recoloured = recolour->GetRecolourTable(recol_base[0] - 1)[recol_base[1]];
	return MakeRGBA(sf(GetR(recoloured)), sf(GetG(recoloured)), sf(GetB(recoloured)), af(rgba_base[3]));
}

/**
 * Get this image with a gradient shift and/or recolouring applied.
 * @param shift Gradient shift to apply to the image.
 * @param recolour Recolouring to apply to the image.
 * @return The altered image's RGBA pixel values.
 */
const uint8 *ImageData::GetRecoloured(GradientShift shift, const Recolouring &recolour) const
{
	RecolourData map_key(shift, recolour.ToCondensed());
	const auto it = this->recoloured.find(map_key);
	if (it != this->recoloured.end()) return it->second.get();

	ShiftFunc af = GetAlphaShiftFunc(shift);
	std::unique_ptr<uint8[]> result(new uint8[this->width * this->height * 4]);
	uint8 *ptr = result.get();
	const uint8 *recol_ptr = this->recol.get();

	if (this->is_8bpp) {
		for (int y = 0; y < this->height; ++y) {
			for (int x = 0; x < this->width; ++x) {
				uint32 pixel = _palette[recolour.GetPalette(shift)[*(recol_ptr++)]];
				*(ptr++) = GetR(pixel);
				*(ptr++) = GetG(pixel);
				*(ptr++) = GetB(pixel);
				*(ptr++) = af(GetA(pixel));
			}
		}
	} else {
		const uint8 *rgba_ptr = this->rgba.get();
		ShiftFunc sf = GetGradientShiftFunc(shift);
		for (int y = 0; y < this->height; ++y) {
			for (int x = 0; x < this->width; ++x) {
				if (recol_ptr[0] == 0) {
					*(ptr++) = sf(*(rgba_ptr++));
					*(ptr++) = sf(*(rgba_ptr++));
					*(ptr++) = sf(*(rgba_ptr++));
					*(ptr++) = af(*(rgba_ptr++));
				} else {
					const uint32 recoloured = recolour.GetRecolourTable(recol_ptr[0] - 1)[recol_ptr[1]];
					*(ptr++) = sf(GetR(recoloured));
					*(ptr++) = sf(GetG(recoloured));
					*(ptr++) = sf(GetB(recoloured));
					*(ptr++) = af(rgba_ptr[3]);
					rgba_ptr += 4;
				}
				recol_ptr += 2;
			}
		}
	}

	ptr = result.get();
	this->recoloured.emplace(map_key, std::move(result));
	return ptr;
}

/**
 * Scale this image to a different size.
 * @param factor Factor by which to scale.
 * @return The scaled instance.
 */
ImageData *ImageData::Scale(float factor)
{
	constexpr float EPSILON = 0.01;  ///< Threshold for float comparisons.
	if (fabs(factor - 1.0f) < EPSILON) return this;
	for (const auto &pair : this->scaled) if (fabs(pair.first - factor) < EPSILON) return pair.second.get();

	ImageData *img = new ImageData;
	img->is_8bpp = this->is_8bpp;
	img->width = round(this->width * factor);
	img->height = round(this->height * factor);
	img->xoffset = round(this->xoffset * factor);
	img->yoffset = round(this->yoffset * factor);

	const int nrecol = (img->is_8bpp ? 1 : 2);
	img->recol.reset(new uint8[img->width * img->height * nrecol]);
	img->rgba.reset(new uint8[img->width * img->height * 4]);

	if (factor > 1.f) {
		/* Upscaling. Each old pixel is copied to multiple new pixels. */
		for (uint16 x = 0; x < img->width; ++x) {
			for (uint16 y = 0; y < img->height; ++y) {
				uint16 oldx = this->width * x / img->width;
				uint16 oldy = this->height * y / img->height;
				uint8 *newptr = &img->rgba[4 * (y * img->width + x)];
				const uint8 *oldptr = &this->rgba[4 * (oldy * this->width + oldx)];
				for (int i = 0; i < 4; ++i) *(newptr++) = *(oldptr++);

				newptr = &img->recol[nrecol * (y * img->width + x)];
				oldptr = &this->recol[nrecol * (oldy * this->width + oldx)];
				for (int i = 0; i < nrecol; ++i) *(newptr++) = *(oldptr++);
			}
		}
	} else {
		/* Downscaling. Each new pixel is averaged from multiple old pixels. */
		for (uint16 x = 0; x < img->width; ++x) {
			for (uint16 y = 0; y < img->height; ++y) {
				uint16 oldx1 = this->width * x / img->width;
				uint16 oldy1 = this->height * y / img->height;
				uint16 oldx2 = this->width * (x + 1) / img->width;
				uint16 oldy2 = this->height * (y + 1) / img->height;
				assert(oldx2 > oldx1 && oldy2 > oldy1);
				int avg[] = {0, 0, 0, 0};
				for (uint16 oldx = oldx1; oldx < oldx2; ++oldx) {
					for (uint16 oldy = oldy1; oldy < oldy2; ++oldy) {
						const uint8 *oldptr = &this->rgba[4 * (oldy * this->width + oldx)];
						for (int i = 0; i < 4; ++i) avg[i] += *(oldptr++);
					}
				}
				uint8 *newptr = &img->rgba[4 * (y * img->width + x)];
				for (int i = 0; i < 4; ++i) *(newptr++) = avg[i] / ((oldx2 - oldx1) * (oldy2 - oldy1));

				newptr = &img->recol[nrecol * (y * img->width + x)];
				const uint8 *oldptr = &this->recol[nrecol * (oldy1 * this->width + oldx1)];
				for (int i = 0; i < nrecol; ++i) *(newptr++) = *(oldptr++);
			}
		}
	}

	this->scaled.emplace_back(factor, img);
	return img;
}

/**
 * Load 8bpp or 32bpp sprite block from the \a rcd_file.
 * @param rcd_file File being loaded.
 * @return Loaded sprite, if loading was successful, else \c nullptr.
 */
ImageData *LoadImage(RcdFileReader *rcd_file)
{
	const uint32 batch_index = _sprites_loaded / IMAGE_BATCH_SIZE;
	const uint32 index_in_batch = _sprites_loaded % IMAGE_BATCH_SIZE;
	if (_sprites_loaded >= IMAGE_BATCH_SIZE * _sprites.size()) _sprites.emplace_back(new ImageData[IMAGE_BATCH_SIZE]);

	bool is_8bpp = strcmp(rcd_file->name, "8PXL") == 0;
	rcd_file->CheckVersion(is_8bpp ? 2 : 1);

	_sprites_loaded++;
	ImageData *imd = &_sprites.at(batch_index)[index_in_batch];
	try {
		if (is_8bpp) {
			imd->is_8bpp = true;
			imd->Load8bpp(rcd_file, rcd_file->size);
		} else {
			imd->is_8bpp = false;
			imd->Load32bpp(rcd_file, rcd_file->size);
		}
	} catch (...) {
		_sprites_loaded--;
		throw;
	}
	return imd;
}

/** Initialize image storage. */
void InitImageStorage()
{
	/* Nothing to do currently. */
}

/** Clear all memory. */
void DestroyImageStorage()
{
	_sprites.clear();
}

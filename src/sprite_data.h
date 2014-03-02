/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sprite_data.h Declarations for sprite data. */

#ifndef SPRITE_DATA_H
#define SPRITE_DATA_H

static const uint32 INVALID_JUMP = UINT32_MAX; ///< Invalid jump destination in image data.

class RcdFile;

/**
 * Image data of 8bpp images.
 * @ingroup sprites_group
 */
class ImageData {
public:
	ImageData();
	~ImageData();

	bool Load(RcdFile *rcd_file, size_t length);

	uint32 GetPixel(uint16 xoffset, uint16 yoffset) const;

	uint16 width;  ///< Width of the image.
	uint16 height; ///< Height of the image.
	int16 xoffset; ///< Horizontal offset of the image.
	int16 yoffset; ///< Vertical offset of the image.
	uint32 *table; ///< The jump table. For missing entries, #INVALID_JUMP is used.
	uint8 *data;   ///< The image data itself.
};

ImageData *LoadImage(RcdFile *rcd_file, size_t length);

void InitImageStorage();
void DestroyImageStorage();

#endif


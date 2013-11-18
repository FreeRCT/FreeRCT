/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file image.h %Image loader, cutting, and writing. */

#ifndef IMAGE_H
#define IMAGE_H

#include <png.h>
#include <string>

/** Bitmask description. */
class BitMaskData {
public:
	int x_pos;        ///< Base X position.
	int y_pos;        ///< Base Y position.
	std::string type; ///< Name of the mask to apply.
};

struct MaskInformation;

/** A PNG image file. */
class Image {
public:
	Image();
	~Image();

	const char *LoadFile(const char *fname, BitMaskData *mask);
	int GetWidth();
	int GetHeight();
	bool IsEmpty(int xpos, int ypos, int dx, int dy, int length);
	uint8 GetPixel(int x, int y);

	bool png_initialized; ///< Whether the data structures below are initialized.
	uint8 **row_pointers; ///< Pointers into the rows of the image.

private:
	int mask_xpos;               ///< X position of the left of the mask.
	int mask_ypos;               ///< Y position of the top of the mask.
	const MaskInformation *mask; ///< Information about the used bitmask (or \c nullptr).

	int width;            ///< Width of the loaded image.
	int height;           ///< Height of the loaded image.
	png_structp png_ptr;  ///< Png image data.
	png_infop info_ptr;   ///< Png information.
	png_infop end_info;   ///< Png end information.
};

/** A Sprite. */
class SpriteImage {
public:
	SpriteImage();
	~SpriteImage();

	const char *CopySprite(Image *img, int xoffset, int yoffset, int xpos, int ypos, int xsize, int ysize, bool crop);

	int xoffset;       ///< Horizontal offset from the origin to the top-left pixel of the sprite image.
	int yoffset;       ///< Vertical offset from the origin to the top-left pixel of the sprite image.
	int width;         ///< Width of the image.
	int height;        ///< Number of rows of the image.

	uint8 *data;       ///< Compressed image data.
	uint16 *row_sizes; ///< Number of bytes needed for each row.
	int data_size;     ///< Size of the #data field.
};

#endif

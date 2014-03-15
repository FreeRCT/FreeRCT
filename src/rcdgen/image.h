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
class ImageFile {
public:
	ImageFile();
	~ImageFile();

	void Clear();
	const char *LoadFile(const std::string &fname);
	int GetWidth() const;
	int GetHeight() const;
	bool Is8bpp() const;

	bool png_initialized; ///< Whether the data structures below are initialized.
	uint8 **row_pointers; ///< Pointers into the rows of the image.

	int width;         ///< Width of the loaded image.
	int height;        ///< Height of the loaded image.
	int color_type;    ///< Type of image.
	std::string fname; ///< Name of the loaded file.

private:
	png_structp png_ptr; ///< Png image data.
	png_infop info_ptr;  ///< Png information.
	png_infop end_info;  ///< Png end information.
};

/**
 * Pixel access to the image.
 *
 * Code assumes that the block structure of 32bpp and 8bpp is exactly the same, just the block name,
 * block version, and the sprite encoding are different.
 */
class Image {
public:
	Image(const ImageFile *imf, const char *block_name, int block_version, BitMaskData *mask);
	virtual ~Image();

	int GetWidth() const;
	int GetHeight() const;
	bool IsEmpty(int xpos, int ypos, int dx, int dy, int length) const;
	bool IsMaskedOut(int xpos, int ypos) const;

	virtual bool IsTransparent(int xpos, int ypos) const = 0;
	virtual uint8 *Encode(int xpos, int ypos, int width, int height, int *size) const = 0;

	const char *block_name;  ///< Name of the block to write.
	const int block_version; ///< Version number of the block to write.

protected:
	const ImageFile *imf;        ///< Image file (not owned by the object).
	int mask_xpos;               ///< X position of the left of the mask.
	int mask_ypos;               ///< Y position of the top of the mask.
	const MaskInformation *mask; ///< Information about the used bitmask (or \c nullptr).
};

/** An 8bpp image. */
class Image8bpp : public Image {
public:
	Image8bpp(const ImageFile *imf, BitMaskData *mask);

	uint8 GetPixel(int x, int y) const;
	virtual bool IsTransparent(int xpos, int ypos) const override;
	virtual uint8 *Encode(int xpos, int ypos, int width, int height, int *size) const override;
};

/** A 32bpp image. */
class Image32bpp : public Image {
public:
	Image32bpp(const ImageFile *imf, BitMaskData *mask);

	void SetRecolourImage(Image8bpp *recolour);

	uint32 GetPixel(int x, int y) const;
	virtual bool IsTransparent(int xpos, int ypos) const override;
	virtual uint8 *Encode(int xpos, int ypos, int width, int height, int *size) const override;

protected:
	uint8 GetCurrentRecolour(int x, int y, int max_length, int *count) const;
	uint8 GetSameOpaqueness(int x, int y, int max_length, int *count) const;

	Image8bpp *recolour; ///< Recolour information (not owned by this class).
};

/** A Sprite. */
class SpriteImage {
public:
	SpriteImage();
	~SpriteImage();

	const char *CopySprite(Image *img, int xoffset, int yoffset, int xpos, int ypos, int xsize, int ysize, bool crop);

	int xoffset;            ///< Horizontal offset from the origin to the top-left pixel of the sprite image.
	int yoffset;            ///< Vertical offset from the origin to the top-left pixel of the sprite image.
	int width;              ///< Width of the image.
	int height;             ///< Number of rows of the image.
	const char *block_name; ///< Name of the block to write.
	int block_version;      ///< Version number of the block to write.

	uint8 *data;   ///< Compressed image data.
	int data_size; ///< Size of the #data field.
};

#endif

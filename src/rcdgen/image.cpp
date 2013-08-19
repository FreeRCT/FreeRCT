/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file image.cpp %Image loading, cutting, and saving the sprites. */

#include "../stdafx.h"
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include "image.h"

#include "mask64.xbm"

static const size_t HEADER_SIZE = 4; ///< Number of bytes to read to decide whether a provided file is indeed a PNG file.
static const int TRANSPARENT = 0;    ///< Colour index of 'transparent' in the 8bpp image.


/** Information about available bit masks. */
struct MaskInformation {
	int width;  ///< Width of the mask.
	int height; ///< Height of the mask.
	const unsigned char *data; ///< Data of the mask.
	const char *name; ///< Name of the mask.
};

/** List of bit masks. */
static const MaskInformation _masks[] = {
	{mask64_width, mask64_height, mask64_bits, "voxel64"},
	{0, 0, NULL, NULL}
};

/**
 * Retrieve a bitmask by its name.
 * @param name Name of the bitmask to retrieve.
 * @return The bitmask and its meta-information (static reference, do not free).
 */
static const MaskInformation *GetMask(const std::string &name)
{
	const MaskInformation *msk = _masks;
	while (msk->name != NULL) {
		if (msk->name == name) return msk;
		msk++;
	}
	fprintf(stderr, "Error: Cannot find a bitmask named \"%s\"\n", name.c_str());
	exit(1);
	return NULL;
}

Image::Image()
{
	this->png_initialized = false;
}

Image::~Image()
{
	if (this->png_initialized) {
		png_destroy_read_struct(&this->png_ptr, &this->info_ptr, &this->end_info);
	}
}

/**
 * Load a .png file from the disk.
 * @param fname Name of the .png file to load.
 * @param mask Bitmask to apply.
 * @return An error message if loading failed, or \c NULL if loading succeeded.
 */
const char *Image::LoadFile(const char *fname, BitMaskData *mask)
{
	FILE *fp = fopen(fname, "rb");
	if (fp == NULL) return "Input file does not exist";

	uint8 header[HEADER_SIZE];
	if (fread(header, 1, HEADER_SIZE, fp) != HEADER_SIZE) {
		fclose(fp);
		return "Failed to read the PNG header";
	}
	bool is_png = !png_sig_cmp(header, 0, HEADER_SIZE);
	if (!is_png) {
		fclose(fp);
		return "Header indicates it is not a PNG file";
	}

	this->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!this->png_ptr) {
		fclose(fp);
		return "Failed to initialize the png data";
	}

	this-> info_ptr = png_create_info_struct(this->png_ptr);
	if (!this->info_ptr) {
		png_destroy_read_struct(&this->png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		fclose(fp);
		return "Failed to setup a png info structure";
	}

	this->end_info = png_create_info_struct(this->png_ptr);
	if (!this->end_info) {
		png_destroy_read_struct(&this->png_ptr, &this->info_ptr, (png_infopp)NULL);
		fclose(fp);
		return "Failed to setup a png end info structure";
	}

	/* Setup callback in case of errors. */
	if (setjmp(png_jmpbuf(this->png_ptr))) {
		png_destroy_read_struct(&this->png_ptr, &this->info_ptr, &this->end_info);
		fclose(fp);
		return "Error detected while reading PNG file";
	}

	/* Initialize for file reading. */
	png_init_io(this->png_ptr, fp);
	png_set_sig_bytes(this->png_ptr, HEADER_SIZE);

	png_read_png(this->png_ptr, this->info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	int bit_depth = png_get_bit_depth(this->png_ptr, this->info_ptr);
	if (bit_depth != 8) {
		png_destroy_read_struct(&this->png_ptr, &this->info_ptr, &this->end_info);
		fclose(fp);
		return "PNG file is not an 8bpp file";
	}

	if (mask != NULL) {
		this->mask = GetMask(mask->type);
		this->mask_xpos = mask->x_pos;
		this->mask_ypos = mask->y_pos;
	} else {
		this->mask = NULL;
		this->mask_xpos = 0;
		this->mask_ypos = 0;
	}

	this->png_initialized = true;
	this->row_pointers = png_get_rows(this->png_ptr, this->info_ptr);
	fclose(fp);

	this->width = png_get_image_width(this->png_ptr, this->info_ptr);
	this->height = png_get_image_height(this->png_ptr, this->info_ptr);
	return NULL;
}

/**
 * Get the width of the image.
 * @return Width of the loaded image, or \c -1.
 */
int Image::GetWidth()
{
	if (!this->png_initialized) return -1;
	return this->width;
}

/**
 * Get the height of the image.
 * @return Height of the loaded image, or \c -1.
 */
int Image::GetHeight()
{
	if (!this->png_initialized) return -1;
	return this->height;
}

/**
 * Get a pixel from the image.
 * @param x Horizontal position.
 * @param y Vertical position.
 * @return Value of the pixel (palette index, as it is an 8bpp indexed image).
 */
uint8 Image::GetPixel(int x, int y)
{
	assert(this->png_initialized);
	assert(x >= 0 && x < this->width);
	assert(y >= 0 && y < this->height);

	if (this->mask != NULL) {
		if (x >= this->mask_xpos && x < this->mask_xpos + this->mask->width &&
				y >= this->mask_ypos && y < this->mask_ypos + this->mask->height) {
			const unsigned char *p = this->mask->data;
			int off = (this->mask_ypos - y) * ((this->mask->width + 7) / 8);
			p += off;
			off = this->mask_xpos - x;
			p += off / 8;
			if ((*p & (1 << (off & 7))) == 0) return TRANSPARENT;
		}
	}

	return this->row_pointers[y][x];
}

/**
 * Is the queried set of pixels empty?
 * @param xpos Horizontal start position.
 * @param ypos Vertical start position.
 * @param dx Horizontal stepping size.
 * @param dy Vertical stepping size.
 * @param length Number of pixels to examine.
 * @return All examined pixels are empty (have colour \c 0).
 */
bool Image::IsEmpty(int xpos, int ypos, int dx, int dy, int length)
{
	while (length > 0) {
		if (this->GetPixel(xpos, ypos) != TRANSPARENT) return false;
		xpos += dx;
		ypos += dy;
		length--;
	}
	return true;
}


SpriteImage::SpriteImage()
{
	this->data = NULL;
	this->row_sizes = NULL;
	this->data_size = 0;
	this->width = 0;
	this->height = 0;
}

SpriteImage::~SpriteImage()
{
	free(this->data);
	free(this->row_sizes);
}

/**
 * Copy a part of the image as a sprite.
 * @param img %Image source.
 * @param xoffset Horizontal offset of the origin to the top-left pixel of the sprite.
 * @param yoffset Vertical offset of the origin to the top-left pixel of the sprite.
 * @param xpos Left position of the sprite in the image.
 * @param ypos Top position of the sprite in the image.
 * @param xsize Width of the sprite in the image.
 * @param ysize Height of the sprite in the image.
 * @param crop Perform cropping of the sprite.
 * @return Error message if the conversion failed, else \c NULL.
 */
const char *SpriteImage::CopySprite(Image *img, int xoffset, int yoffset, int xpos, int ypos, int xsize, int ysize, bool crop)
{
	assert(img->png_initialized);

	/* Remove any old data. */
	free(this->data);
	free(this->row_sizes);
	this->data = NULL;
	this->row_sizes = NULL;
	this->data_size = 0;
	this->height = 0;

	int img_width = img->GetWidth();
	int img_height = img->GetHeight();
	if (xpos < 0 || ypos < 0) return "Negative starting position";
	if (xpos >= img_width || ypos >= img_height) return "Starting position beyond image";
	if (xsize < 0 || ysize < 0) return "Negative image size";
	if (xpos + xsize > img_width) return "Sprite too wide";
	if (ypos + ysize > img_height) return "Sprite too high";

	if (crop) {
		/* Perform cropping. */

		/* Crop left columns. */
		while (xsize > 0 && img->IsEmpty(xpos, ypos, 0, 1, ysize)) {
			xpos++;
			xsize--;
			xoffset++;
		}

		/* Crop top rows. */
		while (ysize > 0 && img->IsEmpty(xpos, ypos, 1, 0, xsize)) {
			ypos++;
			ysize--;
			yoffset++;
		}

		/* Crop right columns. */
		while (xsize > 0 && img->IsEmpty(xpos + xsize - 1, ypos, 0, 1, ysize)) {
			xsize--;
		}

		/* Crop bottom rows. */
		while (ysize > 0 && img->IsEmpty(xpos, ypos + ysize - 1, 1, 0, xsize)) {
			ysize--;
		}
	}

	if (xsize == 0 || ysize == 0) {
		free(this->data);
		this->data = NULL;
		this->data_size = 0;
		this->xoffset = 0;
		this->yoffset = 0;
		this->width = 0;
		this->height = 0;
		return NULL;
	}

	this->xoffset = xoffset;
	this->yoffset = yoffset;
	this->width = xsize;
	this->height = ysize;

	this->row_sizes = (uint16 *)malloc(this->height * sizeof(uint16));
	if (this->row_sizes == NULL) return "Cannot allocate row sizes";

	/* Examine the sprite, and record length of data for each row. */
	this->data_size = 0;
	for (int y = 0; y < this->height; y++) {
		int length = 0;
		int last_stored = 0; // Up to this position (exclusive), the row was counted.
		for (int x = 0; x < xsize; x++) {
			uint8 col = img->GetPixel(xpos + x, ypos + y);
			if (col == TRANSPARENT) continue;

			int start = x;
			x++;
			while (x < xsize) {
				uint8 col = img->GetPixel(xpos + x, ypos + y);
				if (col == TRANSPARENT) break;
				x++;
			}
			/* from 'start' upto and excluding 'x' are pixels to draw. */
			while (last_stored + 127 < start) {
				length += 2; // 127 pixels gap, 0 pixels to draw.
				last_stored += 127;
			}
			while (x - start > 255) {
				length += 2 + 255; // ((start-last_stored) gap, 255 pixels, and 255 colour indices.
				start += 255;
				last_stored = start;
			}
			length += 2 + x - start;
			last_stored = x;
		}
		assert(length <= 0xffff);
		this->row_sizes[y] = length;
		this->data_size += length;
	}
	if (this->data_size == 0) { // No pixels -> no need to store any data.
		this->data = NULL;
		return NULL;
	}

	/* Copy sprite pixels. */
	this->data = (uint8 *)malloc(this->data_size);
	if (this->data == NULL) return "Cannot allocate sprite pixel memory";

	uint8 *ptr = this->data;
	for (int y = 0; y < this->height; y++) {
		if (this->row_sizes[y] == 0) continue;

		uint8 *last_header = NULL;
		int last_stored = 0; // Up to this position (exclusive), the row was counted.
		for (int x = 0; x < xsize; x++) {
			uint8 col = img->GetPixel(xpos + x, ypos + y);
			if (col == TRANSPARENT) continue;

			int start = x;
			x++;
			while (x < xsize) {
				uint8 col = img->GetPixel(xpos + x, ypos + y);
				if (col == TRANSPARENT) break;
				x++;
			}
			/* from 'start' up to and excluding 'x' are pixels to draw. */
			while (last_stored + 127 < start) {
				*ptr++ = 127; // 127 pixels gap, 0 pixels to draw.
				*ptr++ = 0;
				last_stored += 127;
			}
			while (x - start > 255) {
				*ptr++ = start - last_stored;
				*ptr++ = 255;
				for (int i = 0; i < 255; i++) {
					*ptr++ = img->GetPixel(xpos + start, ypos + y);
					start++;
				}
				last_stored = start;
			}
			last_header = ptr;
			*ptr++ = start - last_stored;
			*ptr++ = x - start;
			while (x > start) {
				*ptr++ = img->GetPixel(xpos + start, ypos + y);
				start++;
			}
			last_stored = x;
		}
		assert(last_header != NULL);
		*last_header |= 128; // This was the last sequence of pixels.
	}
	assert(ptr - this->data == this->data_size);
	return NULL;
}

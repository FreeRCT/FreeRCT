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

static const size_t HEADER_SIZE = 4;      ///< Number of bytes to read to decide whether a provided file is indeed a PNG file.
static const int TRANSPARENT_INDEX = 0;   ///< Colour index of 'transparent' in the 8bpp image.
static const uint8 FULLY_TRANSPARENT = 0; ///< Opacity value of a fully transparent pixel.
static const uint8 FULLY_OPAQUE = 255;    ///< Opacity value of a fully opaque pixel.


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
	{0, 0, nullptr, nullptr}
};

/**
 * Retrieve a bitmask by its name.
 * @param name Name of the bitmask to retrieve.
 * @return The bitmask and its meta-information (static reference, do not free).
 */
static const MaskInformation *GetMask(const std::string &name)
{
	const MaskInformation *msk = _masks;
	while (msk->name != nullptr) {
		if (msk->name == name) return msk;
		msk++;
	}
	fprintf(stderr, "Error: Cannot find a bitmask named \"%s\"\n", name.c_str());
	exit(1);
	return nullptr;
}

ImageFile::ImageFile()
{
	this->png_initialized = false;
	this->row_pointers = nullptr;
	this->fname = "";
}

ImageFile::~ImageFile()
{
	this->Clear();
}

/** Reset the object, to prepare it for loading another image file. */
void ImageFile::Clear()
{
	if (!this->png_initialized) return;

	png_destroy_read_struct(&this->png_ptr, &this->info_ptr, &this->end_info);
	this->row_pointers = nullptr;
	this->png_initialized = false;
	this->fname = "";
}

/**
 * Load a .png file from the disk.
 * @param fname Name of the .png file to load.
 * @return An error message if loading failed, or \c nullptr if loading succeeded.
 */
const char *ImageFile::LoadFile(const std::string &fname)
{
	FILE *fp = fopen(fname.c_str(), "rb");
	if (fp == nullptr) return "Input file does not exist";

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

	this->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!this->png_ptr) {
		fclose(fp);
		return "Failed to initialize the png data";
	}

	this-> info_ptr = png_create_info_struct(this->png_ptr);
	if (!this->info_ptr) {
		png_destroy_read_struct(&this->png_ptr, (png_infopp)nullptr, (png_infopp)nullptr);
		fclose(fp);
		return "Failed to setup a png info structure";
	}

	this->end_info = png_create_info_struct(this->png_ptr);
	if (!this->end_info) {
		png_destroy_read_struct(&this->png_ptr, &this->info_ptr, (png_infopp)nullptr);
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

	png_read_png(this->png_ptr, this->info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);
	this->row_pointers = png_get_rows(this->png_ptr, this->info_ptr);
	fclose(fp);
	this->png_initialized = true; // Clear will now clean up.

	int bit_depth = png_get_bit_depth(this->png_ptr, this->info_ptr);
	if (bit_depth != 8) return "Depth of the image channels is not 8 bit";

	this->width = png_get_image_width(this->png_ptr, this->info_ptr);
	this->height = png_get_image_height(this->png_ptr, this->info_ptr);
	this->color_type = png_get_color_type(this->png_ptr, this->info_ptr);
	if (this->color_type != PNG_COLOR_TYPE_PALETTE && this->color_type != PNG_COLOR_TYPE_RGB_ALPHA) {
		return "Incorrect type of image (expected either 8bpp paletted image or RGBA)";
	}

	this->fname = fname;
	return nullptr; // Loading was a success.
}

/**
 * Get the width of the image.
 * @return Width of the loaded image, or \c -1.
 */
int ImageFile::GetWidth() const
{
	if (!this->png_initialized) return -1;
	return this->width;
}

/**
 * Get the height of the image.
 * @return Height of the loaded image, or \c -1.
 */
int ImageFile::GetHeight() const
{
	if (!this->png_initialized) return -1;
	return this->height;
}

/**
 * Return whether the loaded image file can be interpreted as an 8bpp image.
 * @return \c true if the file is an 8bpp image, else \c false (which means it is a 32bpp image).
 */
bool ImageFile::Is8bpp() const {
	return this->color_type == PNG_COLOR_TYPE_PALETTE;
}

/**
 * Write an unsigned 32 bit value into a byte array (little endian format).
 * @param value Value to write.
 * @param ptr Starting point for writing in the array.
 */
static void WriteUInt32(uint32 value, uint8 *ptr)
{
	for (int i = 0; i < 4; i++) {
		*ptr = value & 0xff;
		ptr++;
		value >>= 8;
	}
}

/**
 * Constructor of an 8bpp sprite image.
 * @param imf File data containing the sprite image.
 * @param block_name Name of the sprite block (the 4 letter code)
 * @param block_version Version number of the sprite block.
 * @param mask Bitmask to apply, or \c nullptr.
 */
Image::Image(const ImageFile *imf, const char *block_name, int block_version, BitMaskData *mask) : block_name(block_name), block_version(block_version), imf(imf)
{
	assert(this->imf->png_initialized);

	if (mask != nullptr) {
		this->mask = GetMask(mask->type);
		this->mask_xpos = mask->x_pos;
		this->mask_ypos = mask->y_pos;
	} else {
		this->mask = nullptr;
		this->mask_xpos = 0;
		this->mask_ypos = 0;
	}
}

/** Compiler doesn't like base classes without virtual destructor. */
Image::~Image()
{
}

/**
 * Get the height of the image.
 * @return Height of the loaded image, or \c -1.
 */
int Image::GetHeight() const
{
	return this->imf->GetHeight();
}

/**
 * Get the width of the image.
 * @return Width of the loaded image, or \c -1.
 */
int Image::GetWidth() const
{
	return this->imf->GetWidth();
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
bool Image::IsEmpty(int xpos, int ypos, int dx, int dy, int length) const
{
	while (length > 0) {
		if (!this->IsTransparent(xpos, ypos)) return false;
		xpos += dx;
		ypos += dy;
		length--;
	}
	return true;
}

/**
 * If the pixel at the given coordinate masked away?
 * @param x Horizontal position.
 * @param y Vertical position.
 * @return Whether the pixel at the given coordinate is masked away.
 */
bool Image::IsMaskedOut(int x, int y) const
{
	if (this->mask != nullptr) {
		if (x >= this->mask_xpos && x < this->mask_xpos + this->mask->width &&
				y >= this->mask_ypos && y < this->mask_ypos + this->mask->height) {
			const uint8 *p = this->mask->data;
			int off = (this->mask_ypos - y) * ((this->mask->width + 7) / 8);
			p += off;
			off = this->mask_xpos - x;
			p += off / 8;
			return (*p & (1 << (off & 7))) == 0;
		}
	}
	return false;
}

/**
 * \fn bool Image::IsTransparent(int xpos, int ypos) const
 * Return whether the pixel at the given coordinate is fully transparent.
 * @param xpos Horizontal position of the pixel.
 * @param ypos Vertical position of the pixel.
 * @return Whether the pixel at the given coordinate is fully transparent.
 */

/**
 * \fn uint8 *Image::Encode(int xpos, int ypos, int width, int height, int *size) const
 * Encode an 8bpp sprite from (a part of) the image.
 * @param xpos Left position of the sprite in the image.
 * @param ypos Top position of the sprite in the image.
 * @param width Width of the sprite.
 * @param height Height of the sprite.
 * @param size [out] Number of bytes needed to store the sprite.
 * @return Memory of length \a size holding the sprite data. An empty sprite is return as \c nullptr with \a size /c 0.
 *         A \c nullptr with a non-zero size indicates a memory error.
 */

/**
 * Enable making an 8bpp sprite.
 * @param imf Image file to use.
 * @param mask Bitmask to apply, or \c nullptr.
 */
Image8bpp::Image8bpp(const ImageFile *imf, BitMaskData *mask) : Image(imf, "8PXL", 2, mask)
{
}

/**
 * Get a pixel from the image.
 * @param x Horizontal position.
 * @param y Vertical position.
 * @return Value of the pixel (palette index, as it is an 8bpp indexed image).
 */
uint8 Image8bpp::GetPixel(int x, int y) const
{
	assert(x >= 0 && x < this->imf->width);
	assert(y >= 0 && y < this->imf->height);

	if (this->IsMaskedOut(x, y)) return TRANSPARENT_INDEX;
	return this->imf->row_pointers[y][x];
}

bool Image8bpp::IsTransparent(int xpos, int ypos) const
{
	return this->GetPixel(xpos, ypos) == TRANSPARENT_INDEX;
}

uint8 *Image8bpp::Encode(int xpos, int ypos, int width, int height, int *size) const
{
	auto row_sizes = std::vector<int>();
	row_sizes.reserve(height);

	/* Examine the sprite, and record length of data for each row. */
	int data_size = 0;
	for (int y = 0; y < height; y++) {
		int length = 0;
		int last_stored = 0; // Up to this position (exclusive), the row was counted.
		for (int x = 0; x < width; x++) {
			if (this->IsTransparent(xpos + x, ypos + y)) continue;

			int start = x;
			x++;
			while (x < width) {
				if (this->IsTransparent(xpos + x, ypos + y)) break;
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
		row_sizes.push_back(length);
		data_size += length;
	}
	if (data_size == 0) { // No pixels -> no need to store any data.
		*size = 0;
		return nullptr;
	}

	data_size += 4 * height; // Add jump table space.
	uint8 *data = new uint8[data_size];
	if (data == nullptr) {
		*size = 1;
		return nullptr;
	}
	uint8 *ptr = data;

	/* Construct jump table. */
	uint32 offset = 4 * height;
	for (int y = 0; y < height; y++) {
		uint32 value = (row_sizes[y] == 0) ? 0 : offset;
		WriteUInt32(value, ptr);
		ptr += 4;
		offset += row_sizes[y];
	}

	/* Copy sprite pixels. */
	for (int y = 0; y < height; y++) {
		if (row_sizes[y] == 0) continue;

		uint8 *last_header = nullptr;
		int last_stored = 0; // Up to this position (exclusive), the row was counted.
		for (int x = 0; x < width; x++) {
			if (this->IsTransparent(xpos + x, ypos + y)) continue;

			int start = x;
			x++;
			while (x < width) {
				if (this->IsTransparent(xpos + x, ypos + y)) break;
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
					*ptr++ = this->GetPixel(xpos + start, ypos + y);
					start++;
				}
				last_stored = start;
			}
			last_header = ptr;
			*ptr++ = start - last_stored;
			*ptr++ = x - start;
			while (x > start) {
				*ptr++ = this->GetPixel(xpos + start, ypos + y);
				start++;
			}
			last_stored = x;
		}
		assert(last_header != nullptr);
		*last_header |= 128; // This was the last sequence of pixels.
	}
	assert(ptr - data == data_size);
	*size = data_size;
	return data;
}

/**
 * Construct a 32bpp pixel value from its components.
 * @param r Intensity of the red colour component.
 * @param g Intensity of the green colour component.
 * @param b Intensity of the blue colour component.
 * @param a Opacity of the pixel.
 * @return Pixel value of the given combination of components.
 */
static inline uint32 MakeRGBA(uint8 r, uint8 g, uint8 b, uint8 a)
{
    uint32 ret, x;
    x = r; ret = x;
    x = g; ret |= (x << 8);
    x = b; ret |= (x << 16);
    x = a; ret |= (x << 24);
    return ret;
}

/**
 * Get the red colour component of a 32bpp pixel value.
 * @param rgba Pixel value to use.
 * @return Value of the red colour component.
 */
static inline uint8 GetR(uint32 rgba)
{
	return rgba & 0xFF;
}

/**
 * Get the green colour component of a 32bpp pixel value.
 * @param rgba Pixel value to use.
 * @return Value of the green colour component.
 */
static inline uint8 GetG(uint32 rgba)
{
	return (rgba >> 8) & 0xFF;
}

/**
 * Get the blue colour component of a 32bpp pixel value.
 * @param rgba Pixel value to use.
 * @return Value of the blue colour component.
 */
static inline uint8 GetB(uint32 rgba)
{
	return (rgba >> 16) & 0xFF;
}

/**
 * Get the opacity component of a 32bpp pixel value.
 * @param rgba Pixel value to use.
 * @return Value of the opacity component.
 */
static inline uint8 GetA(uint32 rgba)
{
	return (rgba >> 24) & 0xFF;
}

/**
 * Enable making a 32bpp sprite.
 * @param imf Image file to use.
 * @param mask Bitmask to apply, or \c nullptr.
 */
Image32bpp::Image32bpp(const ImageFile *imf, BitMaskData *mask) : Image(imf, "32PX", 1, mask)
{
	this->recolour = nullptr;
}

/**
 * Setup a recolour image.
 * @param recolour Recolour image to use.
 * @note Recolour image is not owned by the 32bpp sprite creation class.
 */
void Image32bpp::SetRecolourImage(Image8bpp *recolour)
{
	this->recolour = recolour;
	assert(this->GetWidth()  <= recolour->GetWidth());
	assert(this->GetHeight() <= recolour->GetHeight());
}

/**
 * Get a pixel from the image.
 * @param x Horizontal position.
 * @param y Vertical position.
 * @return Value of the pixel (32 bit full colour with alpha channel).
 */
uint32 Image32bpp::GetPixel(int x, int y) const
{
	static const uint32 transparent_pixel = MakeRGBA(0, 0, 0, FULLY_TRANSPARENT);

	assert(x >= 0 && x < this->imf->width);
	assert(y >= 0 && y < this->imf->height);

	if (this->IsMaskedOut(x, y)) return transparent_pixel;
	uint8 *pixel = imf->row_pointers[y] + x * 4;
	return MakeRGBA(pixel[0], pixel[1], pixel[2], pixel[3]);
}

bool Image32bpp::IsTransparent(int xpos, int ypos) const
{
	return GetA(this->GetPixel(xpos, ypos)) == FULLY_TRANSPARENT;
}

/**
 * Find out how many pixels at the line have the same recolour information.
 * @param x X position of the first pixel in the image to examine.
 * @param y Y position of the first pixel in the image to examine.
 * @param max_length Maximum number of pixels to test (should be at least \c 1).
 * @param count [out] Number of pixels with the same recolour information.
 * @return Value of the recolour information (at \a x, \a y).
 */
uint8 Image32bpp::GetCurrentRecolour(int x, int y, int max_length, int *count) const
{
	if (this->recolour == nullptr) { // No recolour image available.
		*count = max_length;
		return 0;
	}

	uint8 rec = recolour->GetPixel(x, y);
	*count = 1;
	x++;
	max_length--;
	while (max_length > 0) {
		uint8 rr = recolour->GetPixel(x, y);
		if (rr != rec) break;
		(*count)++;
		x++;
		max_length--;
	}
	return rec;
}

/**
 * Find how many pixels at a line from the given position, have the same opaqueness value.
 * @param x X position of the first pixel in the image to examine.
 * @param y Y position of the first pixel in the image to examine.
 * @param max_length Maximum number of pixels to test (should be at least \c 1).
 * @param count [out] Number of pixels with the same opaqueness value.
 * @return Value of the opaqueness (at \a x, \a y).
 */
uint8 Image32bpp::GetSameOpaqueness(int x, int y, int max_length, int *count) const
{
	uint8 a = GetA(this->GetPixel(x, y));
	*count = 1;
	x++;
	max_length--;
	while (max_length > 0) {
		uint8 aa = GetA(this->GetPixel(x, y));
		if (aa != a) break;
		(*count)++;
		x++;
		max_length--;
	}
	return a;
}

uint8 *Image32bpp::Encode(int xpos, int ypos, int width, int height, int *size) const
{
	/// \todo Add recolouring image handling.
	//
	auto row_sizes = std::vector<int>();
	row_sizes.reserve(height);

	/* Decide size of each scanline. */
	for (int y = 0; y < height; y++) {
		int count = 0;
		int x = 0;
		while (x < width) {
			int recolour_length, opaq_length;
			uint8 recolour = GetCurrentRecolour(xpos + x, ypos + y, width - x, &recolour_length);
			uint8 opaq = this->GetSameOpaqueness(xpos + x, ypos + y, recolour_length, &opaq_length);

			if (recolour != 0) {
				/* Recoloured pixels. */
				if (opaq_length > 63) opaq_length = 63;
				count += 1 + 1 + 1 + opaq_length;
				x += opaq_length;
				continue;
			}
			if (opaq == FULLY_OPAQUE) {
				/* Plain fully opaque 24 bit pixels. */
				if (opaq_length > 63) opaq_length = 63;
				count += 1 + opaq_length * 3;
				x += opaq_length;
				continue;
			}
			if (opaq == FULLY_TRANSPARENT) {
				/* Plain fully transparent pixels. */
				if (opaq_length + x == width) {
					x = width;
					break; // End of the line reached.
				}
				if (opaq_length > 63) opaq_length = 63;
				count += 1;
				x += opaq_length;
				continue;
			}
			/* Partially transparent pixels. */
			if (opaq_length > 63) opaq_length = 63;
			count += 1 + 1 + opaq_length * 3;
			x += opaq_length;
		}
		count++; // Terminating 0 byte
		row_sizes.push_back(count);
	}

	*size = 0;
	for (int y = 0; y < height; y++) {
		assert(row_sizes[y] <= 0xFFFF - 2);
		*size += 2 + row_sizes[y];
	}
	uint8 *data = new uint8[*size];
	uint8 *ptr = data;

	/* Store pixel data. */
	for (int y = 0; y < height; y++) {
		uint size = (y < height - 1) ? row_sizes[y] + 2 : 0;
		*ptr++ = size & 0xff;
		*ptr++ = (size >> 8) & 0xff;

		int x = 0;
		while (x < width) {
			int recolour_length, opaq_length;
			uint8 recolour = GetCurrentRecolour(xpos + x, ypos + y, width - x, &recolour_length);
			uint8 opaq = this->GetSameOpaqueness(xpos + x, ypos + y, recolour_length, &opaq_length);


			if (recolour != 0) {
				/* Recoloured pixels. */
				if (opaq_length > 63) opaq_length = 63;
				*ptr++ = 128 + 64 + opaq_length;
				*ptr++ = recolour;
				*ptr++ = opaq;
				for (int dx = 0; dx < opaq_length; dx++) {
					uint32 pixel = this->GetPixel(xpos + x, ypos + y);
					*ptr++ = std::max(std::max(GetR(pixel), GetG(pixel)), GetB(pixel));
					x++;
				}
				continue;
			}
			if (opaq == FULLY_OPAQUE) {
				/* Plain fully opaque 24 bit pixels. */
				if (opaq_length > 63) opaq_length = 63;
				*ptr++ = 0 + opaq_length;
				for (int dx = 0; dx < opaq_length; dx++) {
					uint32 pixel = this->GetPixel(xpos + x, ypos + y);
					*ptr++ = GetR(pixel);
					*ptr++ = GetG(pixel);
					*ptr++ = GetB(pixel);
					x++;
				}
				continue;
			}
			if (opaq == FULLY_TRANSPARENT) {
				/* Plain fully transparent pixels. */
				if (opaq_length + x == width) {
					x = width;
					break; // End of the line reached.
				}
				if (opaq_length > 63) opaq_length = 63;
				*ptr++ = 128 + opaq_length;
				x += opaq_length;
				continue;
			}
			/* Partially transparent pixels. */
			if (opaq_length > 63) opaq_length = 63;
			*ptr++ = 64 + opaq_length;
			*ptr++ = opaq;
			for (int dx = 0; dx < opaq_length; dx++) {
				uint32 pixel = this->GetPixel(xpos + x, ypos + y);
				*ptr++ = GetR(pixel);
				*ptr++ = GetG(pixel);
				*ptr++ = GetB(pixel);
				x++;
			}
		}
		*ptr++ = 0; // Terminating 0 byte.
	}
	assert(ptr - data == *size);
	return data;
}

SpriteImage::SpriteImage()
{
	this->data = nullptr;
	this->data_size = 0;
	this->width = 0;
	this->height = 0;
}

SpriteImage::~SpriteImage()
{
	delete[] this->data;
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
 * @return Error message if the conversion failed, else \c nullptr.
 */
const char *SpriteImage::CopySprite(Image *img, int xoffset, int yoffset, int xpos, int ypos, int xsize, int ysize, bool crop)
{
	/* Remove any old data. */
	delete[] this->data;
	this->data = nullptr;
	this->data_size = 0;
	this->height = 0;
	this->block_name = img->block_name;
	this->block_version = img->block_version;

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
		delete[] this->data;
		this->data = nullptr;
		this->data_size = 0;
		this->xoffset = 0;
		this->yoffset = 0;
		this->width = 0;
		this->height = 0;
		return nullptr;
	}

	this->xoffset = xoffset;
	this->yoffset = yoffset;
	this->width = xsize;
	this->height = ysize;
	this->data = img->Encode(xpos, ypos, xsize, ysize, &this->data_size);
	if (this->data == nullptr && this->data_size != 0) return "Cannot store sprite (not enough memory)";
	return nullptr;
}

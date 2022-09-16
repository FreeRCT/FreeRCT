/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sprite_data.h Declarations for sprite data. */

#ifndef SPRITE_DATA_H
#define SPRITE_DATA_H

#include <cmath>
#include <map>
#include <memory>
#include "palette.h"

static const uint32 INVALID_JUMP = UINT32_MAX; ///< Invalid jump destination in image data.

class RcdFileReader;

/**
 * Image data of 8bpp images.
 * @ingroup sprites_group
 */
class ImageData {
public:
	ImageData();

	void Load8bpp(RcdFileReader *rcd_file, size_t length);
	void Load32bpp(RcdFileReader *rcd_file, size_t length);

	uint32 GetPixel(uint16 xoffset, uint16 yoffset, const Recolouring *recolour = nullptr, GradientShift shift = GS_NORMAL) const;
	const uint8 *GetRecoloured(GradientShift shift, const Recolouring &recolour) const;

	/**
	 * Is the sprite just a single pixel?
	 * @return Whether the sprite is a single pixel.
	 */
	inline bool IsSinglePixel() const
	{
		return this->width == 1 && this->height == 1;
	}

	bool is_8bpp;  ///< Whether this image is an 8bpp image.
	uint16 width;  ///< Width of the image.
	uint16 height; ///< Height of the image.
	int16 xoffset; ///< Horizontal offset of the image.
	int16 yoffset; ///< Vertical offset of the image.

	std::unique_ptr<uint8[]> rgba;   ///< All pixel values of the image in RGBA format.
	std::unique_ptr<uint8[]> recol;  ///< The recolouring layer and table index of each pixel.

	mutable std::map<RecolourData, std::unique_ptr<uint8[]>> recoloured;       ///< Copies of this image with various alterations.
};

ImageData *LoadImage(RcdFileReader *rcd_file);

void InitImageStorage();
void DestroyImageStorage();

/** A sprite rescaled to a different size. */
struct ScaledImage {
	ScaledImage() : sprite(nullptr), factor(1.0f) {}
	ScaledImage(const ImageData *img, float f = 1.0f) : sprite(img), factor(f)
	{
		assert(this->factor > 0.0f);
	}

	/**
	 * Get the scaled image's width.
	 * @return The width.
	 */
	inline uint16 Width() const
	{
		return lround(sprite->width * factor);
	}

	/**
	 * Get the scaled image's height.
	 * @return The height.
	 */
	inline uint16 Height() const
	{
		return lround(sprite->height * factor);
	}

	/**
	 * Get the scaled image's horizontal offset.
	 * @return The offset.
	 */
	inline int16 XOffset() const
	{
		return lround(sprite->xoffset * factor);
	}

	/**
	 * Get the scaled image's vertical offset.
	 * @return The offset.
	 */
	inline int16 YOffset() const
	{
		return lround(sprite->yoffset * factor);
	}

	/**
	 * Return the pixel-value of the provided position.
	 * @param xoffset Horizontal offset in the sprite.
	 * @param yoffset Vertical offset in the sprite.
	 * @param recolour Recolouring to apply to the retrieved pixel. Use \c nullptr for disabling recolouring.
	 * @param shift Gradient shift to apply to the retrieved pixel. Use #GS_NORMAL for not shifting the colour.
	 * @return Pixel value at the given position, or \c 0 if transparent.
	 */
	inline uint32 GetPixel(uint16 xoffset, uint16 yoffset, const Recolouring *recolour = nullptr, GradientShift shift = GS_NORMAL) const
	{
		return sprite->GetPixel(xoffset / factor, yoffset / factor, recolour, shift);
	}

	const ImageData *sprite;  ///< The source image.
	float factor;             ///< The factor by which to scale the image.
};

#endif

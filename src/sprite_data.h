/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sprite_data.h Declarations for sprite data. */

#ifndef SPRITE_DATA_H
#define SPRITE_DATA_H

#include <map>
#include <memory>
#include "palette.h"
#include "time.h"

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
	std::unique_ptr<uint8[]> GetRecoloured(GradientShift shift, const Recolouring &recolour) const;

	const ImageData *Scale(float factor) const;

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
};

/** Keeps track of cached recolouring and scaling variants of images. */
class ImageVariants {
public:
	ImageVariants();

	ImageData *GetScaled(const ImageData *img, float scale);

	void Insert(const ImageData *img, RecolourData key, uint8 *rgba);
	void Insert(const ImageData *img, float scale, ImageData *scaled);
	void DropStale();

	/** Frequent maintenance tasks. */
	void Tick()
	{
		this->DropStale();
	}

private:
	/** Variants of an image. */
	struct Variant {
		explicit Variant(const ImageData *img);

		const ImageData *sprite;                                           ///< The source image.
		std::vector<std::pair<float, std::unique_ptr<ImageData>>> scaled;  ///< Scaled copies of this image and their scaling factor.
		Realtime last_accessed;                                            ///< When this variant was last fetched from the cache.

		/**
		 * Get the number of data entries in this Variant.
		 * @return Number of entries.
		 */
		uint32 Size() const
		{
			return this->scaled.size();
		}
	};

	std::vector<Variant> cache;  ///< Cache of image variants.
};
extern ImageVariants _image_variants;

ImageData *LoadImage(RcdFileReader *rcd_file);

void InitImageStorage();
void DestroyImageStorage();

#endif

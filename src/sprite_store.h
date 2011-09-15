/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sprite_store.h Sprite storage data structure. */

#ifndef SPRITE_STORE_H
#define SPRITE_STORE_H

#include "orientation.h"
#include <map>

class RcdFile;

/** Block of data from a RCD file. */
class RcdBlock {
public:
	RcdBlock();
	virtual ~RcdBlock();

	RcdBlock *next; ///< Pointer to next block.
};

/** Palette of 8bpp images. */
class PaletteData : public RcdBlock {
public:
	PaletteData();
	~PaletteData();

	bool Load(RcdFile *rcd_file, size_t length);

	uint8 colours[256][3]; ///< 256 RGB colours.
};

typedef std::map<uint32, PaletteData *> PaletteMap; ///< Map of loaded palette data blocks.

/** Image data of 8bpp images. */
class ImageData : public RcdBlock {
public:
	ImageData();
	~ImageData();

	bool Load(RcdFile *rcd_file, size_t length);

	uint16 width;  ///< Width of the image.
	uint16 height; ///< Height of the image.
	uint32 *table; ///< The jump table. For missing entries, #INVALID_JUMP is used.
	uint8 *data;   ///< The image data itself.

	static const uint32 INVALID_JUMP;
};

typedef std::map<uint32, ImageData *> ImageMap; ///< Map of loaded image data blocks.

/**
 * %Sprite data.
 * @todo Move the offsets to the image data.
 */
class Sprite : public RcdBlock {
public:
	Sprite();
	~Sprite();

	bool Load(RcdFile *rcd_file, size_t length, const ImageMap &images, const PaletteMap &palettes);

	PaletteData *palette; ///< Palette of the sprite.
	ImageData *img_data;  ///< Image data of the sprite.
	int16 xoffset;        ///< Horizontal offset of the image.
	int16 yoffset;        ///< Vertical offset of the image.
};

typedef std::map<uint32, Sprite *> SpriteMap; ///< Map of loaded sprite blocks.

/** Sprites that make a surface for one orientation. */
class SurfaceOrientationSprites : public RcdBlock {
public:
	SurfaceOrientationSprites();
	~SurfaceOrientationSprites();

	Sprite *non_steep[15]; ///< Non-steep sprites.
	Sprite *steep[4];      ///< Steep sprites.
};

/** A surface in all orientations. */
class SurfaceData : public RcdBlock {
public:
	SurfaceData();
	~SurfaceData();

	bool Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites);

	uint16 width;  ///< Width of a tile.
	uint16 height; ///< Height of a tile.
	SurfaceOrientationSprites surf_orient[VOR_NUM_ORIENT]; ///< Sprites of the surface, in each view orientation.
};


/**
 * Storage of all sprites.
 * @todo We may want to have more than one surface.
 */
class SpriteStore {
public:
	SpriteStore();
	~SpriteStore();

	const char *Load(const char *fname);
	void AddBlock(RcdBlock *block);

	const Sprite *GetSurfaceSprite(uint type, uint8 slope, uint16 size, ViewOrientation orient);
	const PaletteData *GetPalette();

	RcdBlock *blocks; ///< List of loaded Rcd data blocks.
	SurfaceData *surface; ///< Surface data.
};

extern SpriteStore _sprite_store;

#endif

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
#include "path.h"
#include <map>

class RcdFile;

/** Block of data from a RCD file. */
class RcdBlock {
public:
	RcdBlock();
	virtual ~RcdBlock();

	RcdBlock *next; ///< Pointer to next block.
};

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

	bool Load(RcdFile *rcd_file, size_t length, const ImageMap &images);

	ImageData *img_data;  ///< Image data of the sprite.
	int16 xoffset;        ///< Horizontal offset of the image.
	int16 yoffset;        ///< Vertical offset of the image.
};

typedef std::map<uint32, Sprite *> SpriteMap; ///< Map of loaded sprite blocks.

/** A surface in all orientations. */
class SurfaceData : public RcdBlock {
public:
	SurfaceData();
	~SurfaceData();

	bool Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites);

	uint16 width;  ///< Width of a tile.
	uint16 height; ///< Height of a tile.
	uint16 type;   ///< Type of surface.
	Sprite *surface[NUM_SLOPE_SPRITES]; ///< Sprites displaying the slopes.
};

/** Highlight the currently selected ground tile with a selection cursor. */
class TileSelection : public RcdBlock {
public:
	TileSelection();
	~TileSelection();

	bool Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites);

	uint16 width;  ///< Width of a tile.
	uint16 height; ///< Height of a tile.
	Sprite *surface[NUM_SLOPE_SPRITES]; ///< Sprites with a selection cursor.
};

/** Highlight the currently selected corner of a ground tile with a corner selection sprite. */
class TileCorners : public RcdBlock {
public:
	TileCorners();
	~TileCorners();

	bool Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites);

	uint16 width;  ///< Width of a tile.
	uint16 height; ///< Height of a tile.
	Sprite *sprites[VOR_NUM_ORIENT][NUM_SLOPE_SPRITES]; ///< Corner selection sprites.
};

/** %Path sprites. */
class Path : public RcdBlock {
public:
	Path();
	~Path();

	bool Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites);

	uint16 type;   ///< Type of the path. @see PathTypes
	uint16 width;  ///< Width of a tile.
	uint16 height; ///< Height of a tile.
	Sprite *sprites[PATH_COUNT]; ///< Path sprites, may contain \c NULL sprites.
};

/** Types of foundations. */
enum FoundationType {
	FDT_INVALID = 0, ///< Invalid foundation type.
	FDT_GROUND,      ///< Bare (ground) foundation type.
	FDT_WOOD,        ///< Foundation is covered with wood.
	FDT_BRICK,       ///< Foundation is made of bricks.

	FDT_COUNT,       ///< Number of foundation types.
};

/** %Foundation sprites. */
class Foundation : public RcdBlock {
public:
	Foundation();
	~Foundation();

	bool Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites);

	uint16 type;        ///< Type of the foundation. @see FoundationType
	uint16 width;       ///< Width of a tile.
	uint16 height;      ///< Height of a tile.
	Sprite *sprites[6]; ///< Foundation sprites.
};

/**
 * Storage of all sprites.
 * @todo We may want to have more than one surface.
 */
class SpriteStore {
public:
	SpriteStore();
	~SpriteStore();

	const char *LoadRcdFiles();
	void AddBlock(RcdBlock *block);

	const Sprite *GetSurfaceSprite(uint8 type, uint8 slope, uint16 size, ViewOrientation orient);

protected:
	const char *Load(const char *fname);

	RcdBlock *blocks;           ///< List of loaded Rcd data blocks.
	SurfaceData *surface;       ///< Surface data.
	Foundation *foundation;     ///< Foundation.
	TileSelection *tile_select; ///< Tile selection sprites.
	TileCorners *tile_corners;  ///< Tile corner sprites.
	Path *path_sprites;         ///< Path sprites.
};

extern SpriteStore _sprite_store;

#endif

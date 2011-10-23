/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sprite_store.h %Sprite storage data structure. */

#ifndef SPRITE_STORE_H
#define SPRITE_STORE_H

#include "orientation.h"
#include "path.h"
#include "tile.h"
#include <map>

extern const uint8 _slope_rotation[NUM_SLOPE_SPRITES][4];

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

	uint8 GetPixel(uint16 xoffset, uint16 yoffset) const;

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

	/**
	 * Return the pixel-value of the provided position.
	 * @param xoffset Horizontal offset in the sprite.
	 * @param yoffset Vertical offset in the sprite.
	 * @return Pixel value at the given postion, or \c 0 if transparent.
	 */
	FORCEINLINE uint8 GetPixel(uint16 xoffset, uint16 yoffset) const {
		return img_data->GetPixel(xoffset, yoffset);
	}

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
	uint8 type;    ///< Type of surface.
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
 * Storage of all sprites for a single tile size.
 * @note The data does not belong to this class, it is managed by #SpriteManager instead.
 */
class SpriteStorage {
public:
	SpriteStorage(uint16 size);
	~SpriteStorage();

	void AddSurfaceSprite(SurfaceData *sd);
	void AddTileSelection(TileSelection *tsel);
	void AddTileCorners(TileCorners *tc);
	void AddFoundations(Foundation *fnd);
	void AddPath(Path *path);

	bool HasSufficientGraphics() const;

	/**
	 * Get a ground sprite.
	 * @param type Type of surface.
	 * @param surf_spr Surface sprite index.
	 * @param orient Orientation.
	 * @return Requested sprite if available.
	 * @todo Move this code closer to the sprite selection code.
	 */
	const Sprite *GetSurfaceSprite(uint8 type, uint8 surf_spr, ViewOrientation orient) const
	{
		if (!this->surface[type]) return NULL;
		return this->surface[type]->surface[_slope_rotation[surf_spr][orient]];
	}

	/**
	 * Get a mouse tile cursor sprite.
	 * @param surf_spr Surface sprite index.
	 * @param orient Orientation.
	 * @return Requested sprite if available.
	 */
	const Sprite *GetCursorSprite(uint8 surf_spr, ViewOrientation orient) const
	{
		if (!this->tile_select) return NULL;
		return this->tile_select->surface[_slope_rotation[surf_spr][orient]];
	}

	/**
	 * Get a mouse tile corner sprite.
	 * @param surf_spr Surface sprite index.
	 * @param orient Orientation (selected corner).
	 * @param cursor Ground cursor orientation.
	 * @return Requested sprite if available.
	 */
	const Sprite *GetCornerSprite(uint8 surf_spr, ViewOrientation orient, ViewOrientation cursor) const
	{
		if (!this->tile_corners) return NULL;
		return this->tile_corners->sprites[cursor][_slope_rotation[surf_spr][orient]];
	}

	const uint16 size; ///< Width of the tile.

	SurfaceData *surface[GTP_COUNT];   ///< Surface data.
	Foundation *foundation[FDT_COUNT]; ///< Foundation.
	TileSelection *tile_select;        ///< Tile selection sprites.
	TileCorners *tile_corners;         ///< Tile corner sprites.
	Path *path_sprites;                ///< Path sprites.

protected:
	void Clear();
};

/** Storage and management of all sprites. */
class SpriteManager {
public:
	SpriteManager();
	~SpriteManager();

	const char *LoadRcdFiles();
	void AddBlock(RcdBlock *block);

	bool HasSufficientGraphics() const;
	const SpriteStorage *GetSprites(uint16 size) const;

protected:
	const char *Load(const char *fname);

	RcdBlock *blocks;    ///< List of loaded Rcd data blocks.

	SpriteStorage store; ///< %Sprite storage of size 64.
};

extern SpriteManager _sprite_manager;

#endif

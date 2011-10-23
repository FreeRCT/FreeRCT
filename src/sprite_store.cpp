/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sprite_store.h %Sprite storage functions. */

#include "stdafx.h"
#include "sprite_store.h"
#include "fileio.h"
#include "string_func.h"

SpriteManager _sprite_manager; ///< Sprite manager.

const uint32 ImageData::INVALID_JUMP = 0xFFFFFFFF; ///< Invalid jump destination in image data.

/** %Sprite indices of ground/surface sprites after rotation of the view. */
const uint8 _slope_rotation[NUM_SLOPE_SPRITES][4] = {
	{ 0,  0,  0,  0},
	{ 1,  8,  4,  2},
	{ 2,  1,  8,  4},
	{ 3,  9, 12,  6},
	{ 4,  2,  1,  8},
	{ 5, 10,  5, 10},
	{ 6,  3,  9, 12},
	{ 7, 11, 13, 14},
	{ 8,  4,  2,  1},
	{ 9, 12,  6,  3},
	{10,  5, 10,  5},
	{11, 13, 14,  7},
	{12,  6,  3,  9},
	{13, 14,  7, 11},
	{14,  7, 11, 13},
	{15, 18, 17, 16},
	{16, 15, 18, 17},
	{17, 16, 15, 18},
	{18, 17, 16, 15},
};


/** Class representing a RCD file. */
class RcdFile {
public:
	RcdFile(const char *fname);
	~RcdFile();

	bool CheckFileHeader();

	bool CheckVersion(uint32 version);
	bool GetBlob(void *address, size_t length);

	uint8 GetUInt8();
	uint16 GetUInt16();
	uint32 GetUInt32();
	int16 GetInt16();

	size_t Remaining();

private:
	FILE *fp;         ///< File handle of the opened file.
	size_t file_pos;  ///< Position in the opened file.
	size_t file_size; ///< Size of the opened file.
};

/**
 * Rcd file constructor, loading data from a file.
 * @param fname Name of the file to load.
 */
RcdFile::RcdFile(const char *fname)
{
	this->file_pos = 0;
	this->file_size = 0;
	this->fp = fopen(fname, "rb");
	if (this->fp == NULL) return;

	fseek(this->fp, 0L, SEEK_END);
	this->file_size = ftell(this->fp);
	fseek(this->fp, 0L, SEEK_SET);
}

/** Destructor. */
RcdFile::~RcdFile()
{
	if (this->fp != NULL) fclose(fp);
}

/**
 * Get length of data not yet read.
 * @return Remaining data.
 */
size_t RcdFile::Remaining()
{
	return (this->file_size >= this->file_pos) ? this->file_size - this->file_pos : 0;
}

/**
 * Read an 8 bits unsigned number.
 * @return Loaded number.
 * @pre File must be open, data must be available.
 */
uint8 RcdFile::GetUInt8()
{
	this->file_pos++;
	return fgetc(this->fp);
}

/**
 * Read an 16 bits unsigned number.
 * @return Loaded number.
 * @pre File must be open, data must be available.
 */
uint16 RcdFile::GetUInt16()
{
	uint8 val = this->GetUInt8();
	return val | (this->GetUInt8() << 8);
}

/**
 * Read an 16 bits signed number.
 * @return Loaded number.
 * @pre File must be open, data must be available.
 */
int16 RcdFile::GetInt16()
{
	uint8 val = this->GetUInt8();
	return val | (this->GetUInt8() << 8);
}

/**
 * Read an 32 bits unsigned number.
 * @return Loaded number.
 * @pre File must be open, data must be available.
 */
uint32 RcdFile::GetUInt32()
{
	uint16 val = this->GetUInt16();
	return val | (this->GetUInt16() << 16);
}

/**
 * Check whether the file header makes sense, and has the right version.
 * @return The header seems correct.
 */
bool RcdFile::CheckFileHeader()
{
	if (this->fp == NULL) return false;
	if (this->Remaining() < 8) return false;

	char name[5];
	this->GetBlob(name, 4);
	name[4] = '\0';
	if (strcmp(name, "RCDF") != 0) return false;
	if (!this->CheckVersion(1)) return false;
	return true;
}

/**
 * Get a blob of data from the file.
 * @param address Address to load into.
 * @param length Length of the data.
 * @return Loading was successful.
 */
bool RcdFile::GetBlob(void *address, size_t length)
{
	this->file_pos += length;
	return fread(address, length, 1, this->fp) == 1;
}

/**
 * Check whether the read version matches with the expected version.
 * @param ver Expected version.
 * @return Expected version was loaded.
 * @todo In the future, we may want to have a fallback for loading previous versions, which makes this function useless.
 */
bool RcdFile::CheckVersion(uint32 ver)
{
	uint32 val = this->GetUInt32();
	return val == ver;
}

/** Default constructor of a in-memory RCD block. */
RcdBlock::RcdBlock()
{
	this->next = NULL;
}

RcdBlock::~RcdBlock()
{
}


ImageData::ImageData() : RcdBlock()
{
	this->width = 0;
	this->height = 0;
	this->table = NULL;
	this->data = NULL;
}

ImageData::~ImageData()
{
	free(this->table);
	free(this->data);
}

/**
 * Load image data from the RCD file.
 * @param rcd_file File to load from.
 * @param length Length of the image data block.
 * @return Load was successful.
 * @pre File pointer is at first byte of the block.
 */
bool ImageData::Load(RcdFile *rcd_file, size_t length)
{
	if (length < 4) return false; // 2 bytes width, 2 bytes height
	this->width  = rcd_file->GetUInt16();
	this->height = rcd_file->GetUInt16();

	/* Check against some arbitrary limits that look sufficient at this time. */
	if (this->width == 0 || this->width > 300 || this->height == 0 || this->height > 500) return false;

	length -= 4;
	if (length > 100*1024) return false; // Another arbitrary limit.

	size_t jmp_table = 4 * this->height;
	if (length <= jmp_table) return false; // You need at least place for the jump table.
	length -= jmp_table;

	this->table = (uint32 *)malloc(jmp_table);
	this->data  = (uint8 *)malloc(length);
	if (this->table == NULL || this->data == NULL) return false;

	/* Load jump table, adjusting the entries while loading. */
	for (uint i = 0; i < this->height; i++) {
		uint32 dest = rcd_file->GetUInt32();
		if (dest == 0) {
			this->table[i] = INVALID_JUMP;
			continue;
		}
		dest -= jmp_table;
		if (dest >= length) return false;
		this->table[i] = dest;
	}

	rcd_file->GetBlob(this->data, length); // Load the image data.

	/* Verify the image data. */
	for (uint i = 0; i < this->height; i++) {
		uint32 offset = this->table[i];
		if (offset == INVALID_JUMP) continue;

		uint32 xpos = 0;
		for (;;) {
			if (offset + 2 >= length) return false;
			uint8 rel_pos = this->data[offset];
			uint8 count = this->data[offset + 1];
			xpos += (rel_pos & 127) + count;
			offset += 2 + count;
			if ((rel_pos & 128) == 0) {
				if (xpos >= this->width || offset >= length) return false;
			} else {
				if (xpos > this->width || offset > length) return false;
				break;
			}
		}
	}

	return true;
}

/**
 * Return the pixel-value of the provided position.
 * @param xoffset Horizontal offset in the sprite.
 * @param yoffset Vertical offset in the sprite.
 * @return Pixel value at the given postion, or \c 0 if transparent.
 */
uint8 ImageData::GetPixel(uint16 xoffset, uint16 yoffset) const
{
	if (xoffset >= this->width) return 0;
	if (yoffset >= this->height) return 0;

	uint32 offset = this->table[yoffset];
	if (offset == INVALID_JUMP) return 0;

	uint16 xpos = 0;
	while (xpos < xoffset) {
		uint8 rel_pos = this->data[offset];
		uint8 count = this->data[offset + 1];
		xpos += (rel_pos & 127);
		if (xpos > xoffset) return 0;
		if (xoffset - xpos < count) return this->data[offset + 2 + xoffset - xpos];
		xpos += count;
		offset += 2 + count;
		if ((rel_pos & 128) != 0) break;
	}
	return 0;
}

Sprite::Sprite() : RcdBlock()
{
	this->img_data = NULL;
	this->xoffset = 0;
	this->yoffset = 0;
}

Sprite::~Sprite()
{
	/* Do not release the RCD blocks. */
}

/**
 * Load a sprite from the RCD file.
 * @param rcd_file File to load from.
 * @param length Length of the sprite block.
 * @param images Already loaded image data blocks.
 * @return Load was successful.
 * @pre File pointer is at first byte of the block.
 */
bool Sprite::Load(RcdFile *rcd_file, size_t length, const ImageMap &images)
{
	if (length != 8) return false;
	this->xoffset = rcd_file->GetInt16();
	this->yoffset = rcd_file->GetInt16();

	/* Find the image data block (required). */
	uint32 img_blk = rcd_file->GetUInt32();
	ImageMap::const_iterator img_iter = images.find(img_blk);
	if (img_iter == images.end()) return false;
	this->img_data = (*img_iter).second;

	return true;
}

SurfaceData::SurfaceData() : RcdBlock()
{
	this->width = 0;
	this->height = 0;
	this->type = 0;
	for (uint i = 0; i < lengthof(this->surface); i++) this->surface[i] = NULL;
}

SurfaceData::~SurfaceData()
{
}

/**
 * Load a surface game block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param length Length of the data part of the block.
 * @param sprites Map of already loaded sprites.
 * @return Loading was successful.
 */
bool SurfaceData::Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites)
{
	if (length != 2 + 2 + 2 + 4 * NUM_SLOPE_SPRITES) return false;

	uint16 gt = rcd_file->GetUInt16(); // Ground type bytes.
	uint8 type = GTP_INVALID;
	if (gt == 16) type = GTP_GRASS0;
	if (gt == 17) type = GTP_GRASS1;
	if (gt == 18) type = GTP_GRASS2;
	if (gt == 19) type = GTP_GRASS3;
	if (gt == 32) type = GTP_DESERT;
	if (gt == 48) type = GTP_CURSOR_TEST;
	if (type == GTP_INVALID) return false; // Unknown type of ground.

	this->type = type;
	this->width  = rcd_file->GetUInt16();
	this->height = rcd_file->GetUInt16();

	for (uint sprnum = 0; sprnum < NUM_SLOPE_SPRITES; sprnum++) {
		uint32 val = rcd_file->GetUInt32();
		Sprite *spr;
		if (val == 0) {
			spr = NULL;
		} else {
			SpriteMap::const_iterator iter = sprites.find(val);
			if (iter == sprites.end()) return false;
			spr = (*iter).second;
		}
		this->surface[sprnum] = spr;
	}
	return true;
}


TileSelection::TileSelection() : RcdBlock()
{
	this->width = 0;
	this->height = 0;
	for (uint i = 0; i < lengthof(this->surface); i++) this->surface[i] = NULL;
}

TileSelection::~TileSelection()
{
}

/**
 * Load a tile selection block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param length Length of the data part of the block.
 * @param sprites Map of already loaded sprites.
 * @return Loading was successful.
 */
bool TileSelection::Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites)
{
	if (length != 2 + 2 + 4 * NUM_SLOPE_SPRITES) return false;

	this->width  = rcd_file->GetUInt16();
	this->height = rcd_file->GetUInt16();

	for (uint sprnum = 0; sprnum < NUM_SLOPE_SPRITES; sprnum++) {
		uint32 val = rcd_file->GetUInt32();
		Sprite *spr;
		if (val == 0) {
			spr = NULL;
		} else {
			SpriteMap::const_iterator iter = sprites.find(val);
			if (iter == sprites.end()) return false;
			spr = (*iter).second;
		}
		this->surface[sprnum] = spr;
	}
	return true;
}


Path::Path() : RcdBlock()
{
	this->type = PT_INVALID;
	this->width = 0;
	this->height = 0;
	for (uint i = 0; i < lengthof(this->sprites); i++) this->sprites[i] = NULL;
}

Path::~Path()
{
}

/**
 * Load a path sprites block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param length Length of the data part of the block.
 * @param sprites Map of already loaded sprites.
 * @return Loading was successful.
 */
bool Path::Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites)
{
	if (length != 2 + 2 + 2 + 4 * PATH_COUNT) return false;

	this->type = rcd_file->GetUInt16() / 16;
	if (this->type == PT_INVALID || this->type >= PT_COUNT) return false;

	this->width  = rcd_file->GetUInt16();
	this->height = rcd_file->GetUInt16();

	for (uint sprnum = 0; sprnum < PATH_COUNT; sprnum++) {
		uint32 val = rcd_file->GetUInt32();
		Sprite *spr;
		if (val == 0) {
			spr = NULL;
		} else {
			SpriteMap::const_iterator iter = sprites.find(val);
			if (iter == sprites.end()) return false;
			spr = (*iter).second;
		}
		this->sprites[sprnum] = spr;
	}
	return true;
}


TileCorners::TileCorners() : RcdBlock()
{
	this->width = 0;
	this->height = 0;
	for (uint v = 0; v < VOR_NUM_ORIENT; v++) {
		for (uint i = 0; i < NUM_SLOPE_SPRITES; i++) {
			this->sprites[v][i] = NULL;
		}
	}
}

TileCorners::~TileCorners()
{
}

/**
 * Load a path sprites block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param length Length of the data part of the block.
 * @param sprites Map of already loaded sprites.
 * @return Loading was successful.
 */
bool TileCorners::Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites)
{
	if (length != 2 + 2 + 4 * VOR_NUM_ORIENT * NUM_SLOPE_SPRITES) return false;

	this->width  = rcd_file->GetUInt16();
	this->height = rcd_file->GetUInt16();

	for (uint v = 0; v < VOR_NUM_ORIENT; v++) {
		for (uint sprnum = 0; sprnum < NUM_SLOPE_SPRITES; sprnum++) {
			uint32 val = rcd_file->GetUInt32();
			Sprite *spr;
			if (val == 0) {
				spr = NULL;
			} else {
				SpriteMap::const_iterator iter = sprites.find(val);
				if (iter == sprites.end()) return false;
				spr = (*iter).second;
			}
			this->sprites[v][sprnum] = spr;
		}
	}
	return true;
}


Foundation::Foundation() : RcdBlock()
{
	this->type = FDT_INVALID;
	this->width = 0;
	this->height = 0;
	for (uint i = 0; i < lengthof(this->sprites); i++) this->sprites[i] = NULL;
}

Foundation::~Foundation()
{
}

/**
 * Load a path sprites block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param length Length of the data part of the block.
 * @param sprites Map of already loaded sprites.
 * @return Loading was successful.
 */
bool Foundation::Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites)
{
	if (length != 2 + 2 + 2 + 4 * 6) return false;

	uint16 type = rcd_file->GetUInt16();
	this->type = FDT_INVALID;
	if (type == 16) this->type = FDT_GROUND;
	if (type == 32) this->type = FDT_WOOD;
	if (type == 48) this->type = FDT_BRICK;
	if (this->type == FDT_INVALID) return false;

	this->width  = rcd_file->GetUInt16();
	this->height = rcd_file->GetUInt16();

	for (uint sprnum = 0; sprnum < lengthof(this->sprites); sprnum++) {
		uint32 val = rcd_file->GetUInt32();
		Sprite *spr;
		if (val == 0) {
			spr = NULL;
		} else {
			SpriteMap::const_iterator iter = sprites.find(val);
			if (iter == sprites.end()) return false;
			spr = (*iter).second;
		}
		this->sprites[sprnum] = spr;
	}
	return true;
}

DisplayedObject::DisplayedObject()
{
	this->width = 0;
	for (uint i = 0; i < lengthof(this->sprites); i++) this->sprites[i] = NULL;
}

DisplayedObject::~DisplayedObject()
{
}

/**
 * Load a displayed object block.
 * @param rcd_file RCD file used for loading.
 * @param length Length of the data part of the block.
 * @param sprites Map of already loaded sprites.
 * @return Loading was successful.
 */
bool DisplayedObject::Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites)
{
	if (length != 2 + 4*4) return false;

	this->width = rcd_file->GetUInt16();

	for (uint sprnum = 0; sprnum < lengthof(this->sprites); sprnum++) {
		uint32 val = rcd_file->GetUInt32();
		Sprite *spr;
		if (val == 0) {
			spr = NULL;
		} else {
			SpriteMap::const_iterator iter = sprites.find(val);
			if (iter == sprites.end()) return false;
			spr = (*iter).second;
		}
		this->sprites[sprnum] = spr;
	}
	return true;
}


/**
 * Storage constructor for a single size.
 * @param _size Width of the tile stored in this object.
 */
SpriteStorage::SpriteStorage(uint16 _size) : size(_size)
{
	this->Clear();
}

SpriteStorage::~SpriteStorage()
{
	this->Clear();
}

/** Clear all data from the storage. */
void SpriteStorage::Clear()
{
	for (uint i = 0; i < lengthof(this->surface); i++)    this->surface[i] = NULL;
	for (uint i = 0; i < lengthof(this->foundation); i++) this->foundation[i] = NULL;
	this->tile_select = NULL;
	this->tile_corners = NULL;
	this->path_sprites = NULL;
	this->build_arrows = NULL;
}

/**
 * Add ground tile sprites.
 * @param sd New ground tile sprites.
 * @pre Width of the ground tile sprites must match with #size.
 */
void SpriteStorage::AddSurfaceSprite(SurfaceData *sd)
{
	assert(sd->width == this->size);
	assert(sd->type < GTP_COUNT);
	this->surface[sd->type] = sd;
}

/**
 * Add tile selection sprites.
 * @param tsel New tile selection sprites.
 * @pre Width of the tile selection sprites must match with #size.
 */
void SpriteStorage::AddTileSelection(TileSelection *tsel)
{
	assert(tsel->width == this->size);
	this->tile_select = tsel;
}

/**
 * Add tile corner sprites.
 * @param tc New tile corner sprites.
 * @pre Width of the tile corner sprites must match with #size.
 */
void SpriteStorage::AddTileCorners(TileCorners *tc)
{
	assert(tc->width == this->size);
	this->tile_corners = tc;
}

/**
 * Add foundation sprite.
 * @param fnd New foundation sprite.
 * @pre Width of the foundation sprite must match with #size.
 */
void SpriteStorage::AddFoundations(Foundation *fnd)
{
	assert(fnd->width == this->size);
	assert(fnd->type < FDT_COUNT);
	this->foundation[fnd->type] = fnd;
}

/**
 * Add path sprites.
 * @param path %Path sprites to add.
 * @pre Width of the path sprites must match with #size.
 */
void SpriteStorage::AddPath(Path *path)
{
	assert(path->width == this->size);
	this->path_sprites = path;
}

/**
 * Add build arrow sprites.
 * @param obj Arrow sprites to add.
 * @pre Width of the arrow sprites must match with #size.
 */
void SpriteStorage::AddBuildArrows(DisplayedObject *obj)
{
	assert(obj->width == this->size);
	this->build_arrows = obj;
}

/**
 * Is the collection complete enough to be used in a display?
 * @return Sufficient data has been loaded.
 */
bool SpriteStorage::HasSufficientGraphics() const
{
	return this->surface != NULL; // Check that ground tiles got loaded.
}


/** %Sprite manager constructor. */
SpriteManager::SpriteManager() : store(64)
{
	this->blocks = NULL;
}

/** %Sprite manager destructor. */
SpriteManager::~SpriteManager()
{
	while (this->blocks != NULL) {
		RcdBlock *next_block = this->blocks->next;
		delete this->blocks;
		this->blocks = next_block;
	}
	/* Sprite stores will be deleted soon as well. */
}

/**
 * Load sprites from the disk.
 * @param filename Name of the RCD file to load.
 * @return Error message if load failed, else \c NULL.
 * @todo Try to re-use already loaded blocks.
 * @todo Code will use last loaded surface as grass.
 */
const char *SpriteManager::Load(const char *filename)
{
	char name[5];
	name[4] = '\0';
	uint32 version, length;

	RcdFile rcd_file(filename);
	if (!rcd_file.CheckFileHeader()) return "Could not read header";

	ImageMap   images;   // Images loaded from this file.
	SpriteMap  sprites;  // Sprites loaded from this file.

	/* Load blocks. */
	for (uint blk_num = 1;; blk_num++) {
		size_t remain = rcd_file.Remaining();
		if (remain == 0) return NULL; // End reached.

		if (remain < 12) return "Insufficient space for a block"; // Not enough for a rcd block header, abort.

		if (!rcd_file.GetBlob(name, 4)) return "Loading block name failed";
		version = rcd_file.GetUInt32();
		length = rcd_file.GetUInt32();

		if (length + 12 > remain) return false; // Not enough data in the file.

		if (strcmp(name, "8PXL") == 0 && version == 1) {
			ImageData *img_data = new ImageData;
			if (!img_data->Load(&rcd_file, length)) {
				delete img_data;
				return "Image data loading failed";
			}
			this->AddBlock(img_data);

			std::pair<uint, ImageData *> p(blk_num, img_data);
			images.insert(p);
			continue;
		}

		if (strcmp(name, "SPRT") == 0 && version == 2) {
			Sprite *spr = new Sprite;
			if (!spr->Load(&rcd_file, length, images)) {
				delete spr;
				return "Sprite load failed.";
			}
			this->AddBlock(spr);

			std::pair<uint, Sprite *> p(blk_num, spr);
			sprites.insert(p);
			continue;
		}

		if (strcmp(name, "SURF") == 0 && version == 3) {
			SurfaceData *surf = new SurfaceData;
			if (!surf->Load(&rcd_file, length, sprites)) {
				delete surf;
				return "Surface block loading failed.";
			}
			this->AddBlock(surf);

			this->store.AddSurfaceSprite(surf);
			continue;
		}

		if (strcmp(name, "TSEL") == 0 && version == 1) {
			TileSelection *tsel = new TileSelection;
			if (!tsel->Load(&rcd_file, length, sprites)) {
				delete tsel;
				return "Tile-selection block loading failed.";
			}
			this->AddBlock(tsel);

			this->store.AddTileSelection(tsel);
			continue;
		}

		if (strcmp(name, "PATH") == 0 && version == 1) {
			Path *block = new Path;
			if (!block->Load(&rcd_file, length, sprites)) {
				delete block;
				return "Path-sprites block loading failed.";
			}
			this->AddBlock(block);

			this->store.AddPath(block);
			continue;
		}

		if (strcmp(name, "TCOR") == 0 && version == 1) {
			TileCorners *block = new TileCorners;
			if (!block->Load(&rcd_file, length, sprites)) {
				delete block;
				return "Tile-corners block loading failed.";
			}
			this->AddBlock(block);

			this->store.AddTileCorners(block);
			continue;
		}

		if (strcmp(name, "FUND") == 0 && version == 1) {
			Foundation *block = new Foundation;
			if (!block->Load(&rcd_file, length, sprites)) {
				delete block;
				return "Foundation block loading failed.";
			}
			this->AddBlock(block);

			this->store.AddFoundations(block);
			continue;
		}

		if (strcmp(name, "BDIR") == 0 && version == 1) {
			DisplayedObject *block = new DisplayedObject;
			if (!block->Load(&rcd_file, length, sprites)) {
				delete block;
				return "Build arrows block loading failed.";
			}
			this->AddBlock(block);

			this->store.AddBuildArrows(block);
			continue;
		}

		/* Unknown block in the RCD file. Abort. */
		fprintf(stderr, "Unknown RCD block '%s'\n", name);
		return "Unknown RCD block";
	}
}

/**
 * Load all useful RCD files into the program.
 * @return Error message if loading failed, \c NULL if all went well.
 */
const char *SpriteManager::LoadRcdFiles()
{
	DirectoryReader *reader = MakeDirectoryReader();
	const char *mesg = NULL;

	reader->OpenPath("../rcd");
	while (mesg == NULL) {
		const char *name = reader->NextFile();
		if (name == NULL) break;
		if (!StrEndsWith(name, ".rcd", false)) continue;
		
		mesg = this->Load(name);
	}
	reader->ClosePath();
	delete reader;

	return mesg;
}

/**
 * Add a RCD data block to the list of managed blocks.
 * @param block New block to add.
 */
void SpriteManager::AddBlock(RcdBlock *block)
{
	block->next = this->blocks;
	this->blocks = block;
}

/**
 * Are there sufficient graphics loaded to display something?
 * @return Sufficient data has been loaded.
 */
bool SpriteManager::HasSufficientGraphics() const
{
	return this->store.HasSufficientGraphics();
}

/**
 * Get a sprite store of a given size.
 * @param size Requested size.
 * @return %Sprite store with sprites of the requested size, if it exists, else \c NULL.
 * @todo Add support for other sprite sizes as well.
 */
const SpriteStorage *SpriteManager::GetSprites(uint16 size) const
{
	if (size != 64) return NULL;
	return &this->store;
}


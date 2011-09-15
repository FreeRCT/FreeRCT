/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sprite_store.h Sprite storage functions. */

#include "stdafx.h"
#include "sprite_store.h"

SpriteStore _sprite_store; ///< Sprite storage.

const uint32 ImageData::INVALID_JUMP = 0xFFFFFFFF; ///< Invalid jump destination in image data.


/** Class representing a RCD file. */
class RcdFile {
public:
	RcdFile(const char *fname);
	~RcdFile();

	bool CheckFileHeader();

	bool LoadName(char *name);
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
	this->LoadName(name);
	name[4] = '\0';
	if (strcmp(name, "RCDF") != 0) return false;
	if (!this->CheckVersion(1)) return false;
	return true;
}

/**
 * Load a name (of 4 characters) from the file.
 * @param name Start address of the name.
 * @return Name was read successfully.
 * @deprecated Replace with \c this->GetBlob(name,4)
 * @todo Replace calls with calls to #GetBlob.
 */
bool RcdFile::LoadName(char *name)
{
	return this->GetBlob(name, 4);
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

/**
 * Default constructor.
 * Clears all data.
 */
PaletteData::PaletteData() : RcdBlock()
{
	for (int i = 0; i < 256; i++) {
		this->colours[i][0] = 0;
		this->colours[i][1] = 0;
		this->colours[i][3] = 0;
	}
}

/** Destructor. */
PaletteData::~PaletteData()
{
}

/**
 * Load palette data from the RCD file.
 * @param rcd_file File to load from.
 * @param length Length of the palette data block.
 * @return Load was successful.
 * @pre File pointer is at first byte of the block.
 */
bool PaletteData::Load(RcdFile *rcd_file, size_t length)
{
	uint16 count = rcd_file->GetUInt16();
	if (count < 1u || count > 256u ||
	length != 2 + 3 * (size_t)count) return false;

	uint i = 0;
	while (i < count) {
		this->colours[i][0] = rcd_file->GetUInt8();
		this->colours[i][1] = rcd_file->GetUInt8();
		this->colours[i][3] = rcd_file->GetUInt8();
		i++;
	}
	/* Clear remaining entries. */
	while (i < 256) {
		this->colours[i][0] = 0;
		this->colours[i][1] = 0;
		this->colours[i][3] = 0;
		i++;
	}

	return true;
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

Sprite::Sprite() : RcdBlock()
{
	this->palette = NULL;
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
 * @param palettes Already loaded palette blocks.
 * @return Load was successful.
 * @pre File pointer is at first byte of the block.
 */
bool Sprite::Load(RcdFile *rcd_file, size_t length, const ImageMap &images, const PaletteMap &palettes)
{
	if (length != 12) return false;
	this->xoffset = rcd_file->GetInt16();
	this->yoffset = rcd_file->GetInt16();

	/* Find the image data block (required). */
	uint32 img_blk = rcd_file->GetUInt32();
	ImageMap::const_iterator img_iter = images.find(img_blk);
	if (img_iter == images.end()) return false;
	this->img_data = (*img_iter).second;

	/* Find the palette block (if used). */
	uint32 pal_blk = rcd_file->GetUInt32();
	if (pal_blk == 0) {
		this->palette = NULL;
	} else {
		PaletteMap::const_iterator pal_iter = palettes.find(pal_blk);
		if (pal_iter == palettes.end()) return false;
		this->palette = (*pal_iter).second;
	}

	return true;
}

SurfaceOrientationSprites::SurfaceOrientationSprites() : RcdBlock()
{
	uint i;
	for (i = 0; i < lengthof(this->non_steep); i++) this->non_steep[i] = NULL;
	for (i = 0; i < lengthof(this->steep); i++) this->steep[i] = NULL;
}

SurfaceOrientationSprites::~SurfaceOrientationSprites()
{
	/* Do not free the RCD blocks. */
}

SurfaceData::SurfaceData() : RcdBlock()
{
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
	if (length != 2 + 2 + 4 * 19 * 4) return false;

	this->width  = rcd_file->GetUInt16();
	this->height = rcd_file->GetUInt16();

	for (uint orient = VOR_NORTH; orient < VOR_NUM_ORIENT; orient++) {
		for (uint sprnum = 0; sprnum < 19; sprnum++) {
			uint32 val = rcd_file->GetUInt32();
			Sprite *spr;
			if (val == 0) {
				spr = NULL;
			} else {
				SpriteMap::const_iterator iter = sprites.find(val);
				if (iter == sprites.end()) return false;
				spr = (*iter).second;
			}
			if (sprnum < 15) {
				this->surf_orient[orient].non_steep[sprnum] = spr;
			} else {
				this->surf_orient[orient].steep[sprnum - 15] = spr;
			}
		}
	}
	return true;
}


/** %Sprite storage constructor. */
SpriteStore::SpriteStore()
{
	this->blocks = NULL;
	this->surface = NULL;
}

/** %Sprite storage destructor. */
SpriteStore::~SpriteStore()
{
	while (this->blocks != NULL) {
		RcdBlock *next_block = this->blocks->next;
		delete this->blocks;
		this->blocks = next_block;
	}
	this->surface = NULL; // Released above.
}

/**
 * Load sprites from the disk.
 * @param filename Name of the RCD file to load.
 * @return Error message if load failed, else \c NULL.
 * @todo Try to re-use already loaded blocks. This is at least useful for palettes.
 */
const char *SpriteStore::Load(const char *filename)
{
	char name[5];
	name[4] = '\0';
	uint32 version, length;

	RcdFile rcd_file(filename);
	if (!rcd_file.CheckFileHeader()) return "Could not read header";

	PaletteMap palettes; // Palettes loaded from this file.
	ImageMap   images;   // Images loaded from this file.
	SpriteMap  sprites;  // Sprites loaded from this file.

	/* Load blocks. */
	for (uint blk_num = 1;; blk_num++) {
		size_t remain = rcd_file.Remaining();
		if (remain == 0) return NULL; // End reached.

		if (remain < 12) return "Insufficient space for a block"; // Not enough for a rcd block header, abort.

		if (!rcd_file.LoadName(name)) return "Loading block name failed";
		version = rcd_file.GetUInt32();
		length = rcd_file.GetUInt32();

		if (length + 12 > remain) return false; // Not enough data in the file.

		if (strcmp(name, "8PAL") == 0 && version == 1) {
			PaletteData *pal_data = new PaletteData;
			if (!pal_data->Load(&rcd_file, length)) {
				delete pal_data;
				return "Palette data loading failed";
			}
			this->AddBlock(pal_data);

			std::pair<uint, PaletteData *> p(blk_num, pal_data);
			palettes.insert(p);
			continue;
		}

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

		if (strcmp(name, "SPRT") == 0 && version == 1) {
			Sprite *spr = new Sprite;
			if (!spr->Load(&rcd_file, length, images, palettes)) {
				delete spr;
				return "Sprite load failed.";
			}
			this->AddBlock(spr);

			std::pair<uint, Sprite *> p(blk_num, spr);
			sprites.insert(p);
			continue;
		}

		if (strcmp(name, "SURF") == 0 && version == 1) {
			SurfaceData *surf = new SurfaceData;
			if (!surf->Load(&rcd_file, length, sprites)) {
				delete surf;
				return "Surface block loading failed.";
			}
			this->AddBlock(surf);

			this->surface = surf;
			continue;
		}

		/* Unknown block in the RCD file. Abort. */
		fprintf(stderr, "Unknown RCD block '%s'\n", name);
		return "Unknown RCD block";
	}
}

/**
 * Add a RCD data block to the list of managed blocks.
 * @param block New block to add.
 */
void SpriteStore::AddBlock(RcdBlock *block)
{
	block->next = this->blocks;
	this->blocks = block;
}

/**
 * Get a palette.
 * @todo Take the right one, instead of a pseudo-random one.
 */
const PaletteData *SpriteStore::GetPalette()
{
	if (this->surface == NULL) return NULL;
	if (this->surface->surf_orient[VOR_NORTH].non_steep[0] == NULL) return NULL;
	return this->surface->surf_orient[VOR_NORTH].non_steep[0]->palette;
}

/**
 * Get a surface sprite.
 * @param type Type of surface.
 * @param slope Slope definition.
 * @param size Sprite size.
 * @param orient Orientation.
 * @return Requested sprite if available.
 * @todo Steep handling is sub-optimal, perhaps use #TC_NORTH,  #TC_EAST, #TC_SOUTH, #TC_WEST instead of a bit encoding?
 */
const Sprite *SpriteStore::GetSurfaceSprite(uint type, uint8 slope, uint16 size, ViewOrientation orient)
{
	if (!this->surface) return NULL;
	if (this->surface->width != size) return NULL;
	if ((slope & TCB_STEEP) == 0) {
		assert(slope < 15);
		return this->surface->surf_orient[orient].non_steep[slope];
	}
	if ((slope & TCB_NORTH) != 0) return this->surface->surf_orient[orient].steep[TC_NORTH];
	if ((slope & TCB_EAST)  != 0) return this->surface->surf_orient[orient].steep[TC_EAST];
	if ((slope & TCB_WEST)  != 0) return this->surface->surf_orient[orient].steep[TC_WEST];
	return this->surface->surf_orient[orient].steep[TC_SOUTH];
}


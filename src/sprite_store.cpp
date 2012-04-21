/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sprite_store.cpp %Sprite storage functions. */

/**
 * @defgroup sprites_group Sprites and sprite handling
 */

/**
 * @defgroup gui_sprites_group Gui sprites and sprite handling
 * @ingroup sprites_group
 */

#include "stdafx.h"
#include "sprite_store.h"
#include "fileio.h"
#include "string_func.h"
#include "math_func.h"
#include "table/gui_sprites.h"

SpriteManager _sprite_manager; ///< Sprite manager.
GuiSprites _gui_sprites; ///< Gui sprites.

const uint32 ImageData::INVALID_JUMP = 0xFFFFFFFF; ///< Invalid jump destination in image data.

/**
 * %Sprite indices of ground/surface sprites after rotation of the view.
 * @ingroup sprites_group
 */
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

DisplayedObject::DisplayedObject() : RcdBlock()
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

/** %Animation default constructor. */
Animation::Animation() : RcdBlock()
{
	this->width = 0;
	this->frame_count = 0;
	this->person_type = PERSONS_INVALID;
	this->anim_type = ANIM_INVALID;
	this->frames = NULL;
}

/** %Animation destructor. */
Animation::~Animation()
{
	free(this->frames);
}

/**
 * Load an animation.
 * @param rcd_file RCD file used for loading.
 * @param length Length of the data part of the block.
 * @param sprites Map of already loaded sprites.
 * @return Loading was successful.
 */
bool Animation::Load(RcdFile *rcd_file, size_t length, const SpriteMap &sprites)
{
	const uint BASE_LENGTH = 2 + 1 + 2 + 2;

	if (length < BASE_LENGTH) return false;
	this->width = rcd_file->GetUInt16();

	uint8 pt = rcd_file->GetUInt8();
	switch (pt) {
		case  0: this->person_type = PERSONS_ANY; break;
		case  8: this->person_type = PERSONS_PILLAR; break;
		case 16: this->person_type = PERSONS_EATYH; break;
		default: return false;
	}
	uint16 at = rcd_file->GetUInt16();
	if (at < ANIM_BEGIN || at > ANIM_LAST) return false;
	this->anim_type = (AnimationType)at;

	this->frame_count = rcd_file->GetUInt16();
	if (length != BASE_LENGTH + this->frame_count * 12) return false;
	this->frames = (AnimationFrame *)malloc(this->frame_count * sizeof(AnimationFrame));
	if (this->frames == NULL || this->frame_count == 0) return false;

	AnimationFrame *prev = NULL;
	for (uint i = 0; i < this->frame_count; i++) {
		AnimationFrame *frame = this->frames + i;

		uint32 val = rcd_file->GetUInt32();
		if (val == 0) {
			frame->sprite = NULL;
		} else {
			SpriteMap::const_iterator iter = sprites.find(val);
			if (iter == sprites.end()) return false;
			frame->sprite = (*iter).second;
		}

		frame->duration = rcd_file->GetUInt16();
		if (frame->duration == 0 || frame->duration >= 1000) return false; // Arbitrary sanity limit.

		frame->dx = rcd_file->GetInt16();
		if (frame->dx < -100 || frame->dx > 100) return false; // Arbitrary sanity limit.

		frame->dy = rcd_file->GetInt16();
		if (frame->dy < -100 || frame->dy > 100) return false; // Arbitrary sanity limit.

		frame->number = rcd_file->GetUInt16();

		if (prev != NULL) prev->next = frame;
		prev = frame;
	}
	prev->next = this->frames; // All animations loop back to the first frame currently.
	return true;
}

/** Clear the border sprite data. */
void BorderSpriteData::Clear()
{
	this->border_top = 0;
	this->border_left = 0;
	this->border_right = 0;
	this->border_bottom = 0;

	this->min_width = 0;
	this->min_height = 0;
	this->hor_stepsize = 0;
	this->vert_stepsize = 0;

	for (int i = 0; i < WBS_COUNT; i++) {
		this->normal[i] = NULL;
		this->pressed[i] = NULL;
	}
}

/**
 * Check whether the border sprite data is actually loaded.
 * @return Whether the sprite data is loaded.
 */
bool BorderSpriteData::IsLoaded() const
{
	return this->min_width != 0 && this->min_height != 0;
}

/**
 * Load sprites of a GUI widget border.
 * @param rcd_file RCD file being loaded.
 * @param length Length of the block to load.
 * @param sprites Sprites loaded from this file.
 * @return Load was successful.
 */
bool GuiSprites::LoadGBOR(RcdFile *rcd_file, size_t length, const SpriteMap &sprites)
{
	if (length != 2 + 8*1 + WBS_COUNT * 4) return false;

	/* Select sprites to save to. */
	uint16 tp = rcd_file->GetUInt16(); // Widget type
	BorderSpriteData *sprdata = NULL;
	bool pressed = false;
	switch (tp) {
		case 32: sprdata = &this->titlebar;       pressed = false; break;
		case 48: sprdata = &this->button;         pressed = false; break;
		case 49: sprdata = &this->button;         pressed = true;  break;
		case 52: sprdata = &this->rounded_button; pressed = false; break;
		case 53: sprdata = &this->rounded_button; pressed = true;  break;
		case 64: sprdata = &this->frame;          pressed = false; break;
		case 68: sprdata = &this->panel;          pressed = false; break;
		case 80: sprdata = &this->inset_frame;    pressed = false; break;
		default:
			return false;
	}

	sprdata->border_top    = rcd_file->GetUInt8();
	sprdata->border_left   = rcd_file->GetUInt8();
	sprdata->border_right  = rcd_file->GetUInt8();
	sprdata->border_bottom = rcd_file->GetUInt8();
	sprdata->min_width     = rcd_file->GetUInt8();
	sprdata->min_height    = rcd_file->GetUInt8();
	sprdata->hor_stepsize  = rcd_file->GetUInt8();
	sprdata->vert_stepsize = rcd_file->GetUInt8();

	for (uint sprnum = 0; sprnum < WBS_COUNT; sprnum++) {
		uint32 val = rcd_file->GetUInt32();
		Sprite *spr;
		if (val == 0) {
			spr = NULL;
		} else {
			SpriteMap::const_iterator iter = sprites.find(val);
			if (iter == sprites.end()) return false;
			spr = (*iter).second;
		}
		if (pressed) {
			sprdata->pressed[sprnum] = spr;
		} else {
			sprdata->normal[sprnum] = spr;
		}
	}
	return true;
}

/** Completely clear the data of the checkable sprites. */
void CheckableWidgetSpriteData::Clear()
{
	this->width = 0;
	this->height = 0;

	for (uint sprnum = 0; sprnum < WCS_COUNT; sprnum++) {
		this->sprites[sprnum] = NULL;
	}
}

/**
 * Check whether the checkable sprite data is actually loaded.
 * @return Whether the sprite data is loaded.
 */
bool CheckableWidgetSpriteData::IsLoaded() const
{
	return this->width != 0 && this->height != 0;
}

/**
 * Load checkbox and radio button gui sprites.
 * @param rcd_file RCD file being loaded.
 * @param length Length of the block to load.
 * @param sprites Sprites loaded from this file.
 * @return Load was successful.
 * @todo Load width and height from the RCD file too.
 */
bool GuiSprites::LoadGCHK(RcdFile *rcd_file, size_t length, const SpriteMap &sprites)
{
	if (length != 2 + WCS_COUNT * 4) return false;

	/* Select sprites to save to. */
	uint16 tp = rcd_file->GetUInt16(); // Widget type
	CheckableWidgetSpriteData *sprdata = NULL;
	switch (tp) {
		case 96:  sprdata = &this->checkbox; break;
		case 112: sprdata = &this->radio_button; break;
		default:
			return false;
	}

	sprdata->width = 0;
	sprdata->height = 0;
	for (uint sprnum = 0; sprnum < WCS_COUNT; sprnum++) {
		uint32 val = rcd_file->GetUInt32();
		Sprite *spr;
		if (val == 0) {
			spr = NULL;
		} else {
			SpriteMap::const_iterator iter = sprites.find(val);
			if (iter == sprites.end()) return false;
			spr = (*iter).second;
		}
		sprdata->sprites[sprnum] = spr;

		if (spr != NULL) {
			sprdata->width = max(sprdata->width, spr->img_data->width);
			sprdata->height = max(sprdata->height, spr->img_data->height);
		}
	}
	return true;
}

/** Clear sprite data of a slider bar. */
void SliderSpriteData::Clear()
{
	this->min_bar_length = 0;
	this->stepsize = 0;
	this->height = 0;

	for (uint sprnum = 0; sprnum < WSS_COUNT; sprnum++) {
		this->normal[sprnum] = NULL;
		this->shaded[sprnum] = NULL;
	}
}

/**
 * Check whether the slider bar sprite data is actually loaded.
 * @return Whether the sprite data is loaded.
 */
bool SliderSpriteData::IsLoaded() const
{
	return this->min_bar_length != 0 && this->height != 0;
}

/**
 * Load slider bar sprite data.
 * @param rcd_file RCD file being loaded.
 * @param length Length of the block to load.
 * @param sprites Sprites loaded from this file.
 * @return Load was successful.
 * @todo Move widget_type further to the top in the RCD file block.
 */
bool GuiSprites::LoadGSLI(RcdFile *rcd_file, size_t length, const SpriteMap &sprites)
{
	if (length != 3 * 1 + 2 + WSS_COUNT * 4) return false;

	uint8 min_length = rcd_file->GetUInt8();
	uint8 stepsize = rcd_file->GetUInt8();
	uint8 height = rcd_file->GetUInt8();

	/* Select sprites to save to. */
	uint16 tp = rcd_file->GetUInt16(); // Widget type
	SliderSpriteData *sprdata = NULL;
	bool shaded = false;
	switch (tp) {
		case 128: sprdata = &this->hor_slider;  shaded = false; break;
		case 129: sprdata = &this->hor_slider;  shaded = true;  break;
		case 144: sprdata = &this->vert_slider; shaded = false; break;
		case 145: sprdata = &this->vert_slider; shaded = true;  break;
		default:
			return false;
	}

	sprdata->min_bar_length = min_length;
	sprdata->stepsize = stepsize;
	sprdata->height = height;

	for (uint sprnum = 0; sprnum < WSS_COUNT; sprnum++) {
		uint32 val = rcd_file->GetUInt32();
		Sprite *spr;
		if (val == 0) {
			spr = NULL;
		} else {
			SpriteMap::const_iterator iter = sprites.find(val);
			if (iter == sprites.end()) return false;
			spr = (*iter).second;
		}
		if (shaded) {
			sprdata->shaded[sprnum] = spr;
		} else {
			sprdata->normal[sprnum] = spr;
		}
	}
	return true;
}

/** Clear the scrollbar sprite data. */
void ScrollbarSpriteData::Clear()
{
	this->min_length_all = 0;
	this->min_length_slider = 0;
	this->stepsize_bar = 0;
	this->stepsize_slider = 0;
	this->height = 0;

	for (uint sprnum = 0; sprnum < WLS_COUNT; sprnum++) {
		this->normal[sprnum] = NULL;
		this->shaded[sprnum] = NULL;
	}
}

/**
 * Check whether the scrollbar sprite data is actually loaded.
 * @return Whether the sprite data is loaded.
 */
bool ScrollbarSpriteData::IsLoaded() const
{
	return this->min_length_all != 0 && this->height != 0;
}

/**
 * Load scroll bar sprite data.
 * @param rcd_file RCD file being loaded.
 * @param length Length of the block to load.
 * @param sprites Sprites loaded from this file.
 * @return Load was successful.
 * @todo Move widget_type further to the top in the RCD file block.
 * @todo Add width of the scrollbar in the RCD file block.
 */
bool GuiSprites::LoadGSCL(RcdFile *rcd_file, size_t length, const SpriteMap &sprites)
{
	if (length != 4*1 + 2 + WLS_COUNT * 4) return false;

	uint8 min_length_bar = rcd_file->GetUInt8();
	uint8 stepsize_back = rcd_file->GetUInt8();
	uint8 min_slider = rcd_file->GetUInt8();
	uint8 stepsize_slider = rcd_file->GetUInt8();

	/* Select sprites to save to. */
	uint16 tp = rcd_file->GetUInt16(); // Widget type
	ScrollbarSpriteData *sprdata = NULL;
	bool shaded = false;
	bool vertical = false;
	switch (tp) {
		case 160: sprdata = &this->hor_scroll;  shaded = false; vertical = false; break;
		case 161: sprdata = &this->hor_scroll;  shaded = true;  vertical = false; break;
		case 176: sprdata = &this->vert_scroll; shaded = false; vertical = true;  break;
		case 177: sprdata = &this->vert_scroll; shaded = true;  vertical = true;  break;
		default:
			return false;
	}

	sprdata->min_length_all = min_length_bar;
	sprdata->stepsize_bar = stepsize_back;
	sprdata->min_length_slider = min_slider;
	sprdata->stepsize_slider = stepsize_slider;

	uint16 max_width = 0;
	uint16 max_height = 0;
	for (uint sprnum = 0; sprnum < WLS_COUNT; sprnum++) {
		uint32 val = rcd_file->GetUInt32();
		Sprite *spr;
		if (val == 0) {
			spr = NULL;
		} else {
			SpriteMap::const_iterator iter = sprites.find(val);
			if (iter == sprites.end()) return false;
			spr = (*iter).second;
		}
		if (shaded) {
			sprdata->shaded[sprnum] = spr;
		} else {
			sprdata->normal[sprnum] = spr;
		}

		if (spr != NULL) {
			max_width = max(max_width, spr->img_data->width);
			max_height = max(max_height, spr->img_data->height);
		}
	}

	sprdata->height = (vertical) ? max_width : max_height;
	return true;
}

/** Default constructor. */
GuiSprites::GuiSprites()
{
	this->Clear();
}

/** Clear all Gui sprite data. */
void GuiSprites::Clear()
{
	this->titlebar.Clear();
	this->button.Clear();
	this->rounded_button.Clear();
	this->frame.Clear();
	this->panel.Clear();
	this->inset_frame.Clear();

	this->checkbox.Clear();
	this->radio_button.Clear();

	this->hor_slider.Clear();
	this->vert_slider.Clear();

	this->hor_scroll.Clear();
	this->vert_scroll.Clear();
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
	this->animations.clear(); // Blocks get deleted through the 'this->blocks'.
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
 * Add an animation to the sprite storage.
 * @param anim %Animation object to add.
 * @pre Width of the animation sprites must match with #size.
 */
void SpriteStorage::AddAnimation(Animation *anim)
{
	assert(anim->width == this->size);
	this->animations.insert(std::make_pair(anim->anim_type, anim));
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
	_gui_sprites.Clear();
	this->blocks = NULL;
}

/** %Sprite manager destructor. */
SpriteManager::~SpriteManager()
{
	_gui_sprites.Clear();
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
	if (!rcd_file.CheckFileHeader("RCDF", 1)) return "Could not read header";

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

		if (length + 12 > remain) return "Not enough data"; // Not enough data in the file.

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

		if (strcmp(name, "GCHK") == 0 && version == 1) {
			if (!_gui_sprites.LoadGCHK(&rcd_file, length, sprites)) {
				return "Loading Checkable Gui sprites failed.";
			}
			continue;
		}

		if (strcmp(name, "GBOR") == 0 && version == 1) {
			if (!_gui_sprites.LoadGBOR(&rcd_file, length, sprites)) {
				return "Loading Border Gui sprites failed.";
			}
			continue;
		}

		if (strcmp(name, "GSLI") == 0 && version == 1) {
			if (!_gui_sprites.LoadGSLI(&rcd_file, length, sprites)) {
				return "Loading Slider bar Gui sprites failed.";
			}
			continue;
		}

		if (strcmp(name, "GSCL") == 0 && version == 1) {
			if (!_gui_sprites.LoadGSCL(&rcd_file, length, sprites)) {
				return "Loading Scrollbar Gui sprites failed.";
			}
			continue;
		}

		if (strcmp(name, "ANIM") == 0 && version == 1) {
			Animation *anim = new Animation();
			if (!anim->Load(&rcd_file, length, sprites)) {
				delete anim;
				return "Animation failed to load.";
			}
			if (anim->person_type == PERSONS_INVALID || anim->anim_type == ANIM_INVALID) {
				delete anim;
				return "Unknown animation.";
			}
			this->AddBlock(anim);
			this->store.AddAnimation(anim);
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

/**
 * Get the image data for the GUI according to the table in <tt>table/gui_sprites.h</tt>.
 * @param number Number of the sprite to get.
 * @return The sprite if available, else \c NULL.
 * @todo Add lots of missing sprites.
 * @todo Make this more efficient; linearly trying every entry scales badly.
 */
const ImageData *SpriteManager::GetTableSprite(uint16 number) const
{
	if (number == SPR_GUI_BULLDOZER) return NULL;
	if (number >= SPR_GUI_SLOPES_START && number < SPR_GUI_SLOPES_END) {
		return NULL;
	}
	if (number >= SPR_GUI_BUILDARROW_START && number < SPR_GUI_BUILDARROW_END) {
		return this->store.GetArrowSprite(number - SPR_GUI_BUILDARROW_START, VOR_NORTH)->img_data;
	}
	return NULL;
}


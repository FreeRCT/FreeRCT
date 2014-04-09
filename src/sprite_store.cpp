/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sprite_store.cpp Sprite storage functions. */

/**
 * @defgroup sprites_group Sprites and sprite handling
 */

/**
 * @defgroup gui_sprites_group GUI sprites and sprite handling
 * @ingroup sprites_group
 */

#include "stdafx.h"
#include "sprite_store.h"
#include "sprite_data.h"
#include "rcdfile.h"
#include "fileio.h"
#include "math_func.h"
#include "shop_type.h"
#include "coaster.h"
#include "gui_sprites.h"

SpriteManager _sprite_manager; ///< Sprite manager.
GuiSprites _gui_sprites;       ///< GUI sprites.

static const int MAX_NUM_TEXT_STRINGS = 512; ///< Maximal number of strings in a TEXT data block.

#include "generated/gui_strings.cpp"

/**
 * Sprite indices of ground/surface sprites after rotation of the view.
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
	{15 + 4, 18 + 4, 17 + 4, 16 + 4},
	{16 + 4, 15 + 4, 18 + 4, 17 + 4},
	{17 + 4, 16 + 4, 15 + 4, 18 + 4},
	{18 + 4, 17 + 4, 16 + 4, 15 + 4},
};

/** Default constructor of a in-memory RCD block. */
RcdBlock::RcdBlock()
{
	this->next = nullptr;
}

RcdBlock::~RcdBlock()
{
}

TextData::TextData() : RcdBlock()
{
}

TextData::~TextData()
{
	delete[] this->strings;
	delete[] this->text_data;
}

/**
 * Decode an UTF-8 character.
 * @param data Pointer to the start of the data.
 * @param length Length of the \a data buffer.
 * @param[out] codepoint If decoding was successful, the value of the decoded character.
 * @return Number of bytes read to decode the character, or \c 0 if reading failed.
 */
static int DecodeUtf8Char(uint8 *data, size_t length, uint32 *codepoint)
{
	if (length < 1) return 0;
	uint32 value = *data;
	data++;
	if ((value & 0x80) == 0) {
		*codepoint = value;
		return 1;
	}
	int size;
	uint32 min_value;
	if ((value & 0xE0) == 0xC0) {
		size = 2;
		min_value = 0x80;
		value &= 0x1F;
	} else if ((value & 0xF0) == 0xE0) {
		size = 3;
		min_value = 0x800;
		value &= 0x0F;
	} else if ((value & 0xF8) == 0xF0) {
		size = 4;
		min_value = 0x10000;
		value &= 0x07;
	} else {
		return 0;
	}

	if (length < static_cast<size_t>(size)) return 0;
	for (int n = 1; n < size; n++) {
		uint8 val = *data;
		data++;
		if ((val & 0xC0) != 0x80) return 0;
		value = (value << 6) | (val & 0x3F);
	}
	if (value < min_value || (value >= 0xD800 && value <= 0xDFFF) || value > 0x10FFFF) return 0;
	*codepoint = value;
	return size;
}

/**
 * Check an UTF-8 string.
 * @param rcd_file Input file.
 * @param expected_length Expected length of the string in bytes, including terminating \c NUL byte.
 * @param buffer Buffer to copy the string into.
 * @param buf_length Length of the buffer
 * @param[out] used_size Length of the used part of the buffer, only updated on successful reading of the string.
 * @return Reading the string was a success.
 */
static bool ReadUtf8Text(RcdFileReader *rcd_file, size_t expected_length, uint8 *buffer, size_t buf_length, size_t *used_size)
{
	if (buf_length < *used_size + expected_length) return false;
	rcd_file->GetBlob(buffer + *used_size, expected_length);

	int real_size = expected_length;
	uint8 *start = buffer + *used_size;

	for (;;) {
		uint32 code_point;
		int sz = DecodeUtf8Char(start, real_size, &code_point);
		if (sz == 0 || sz > real_size) return false;
		real_size -= sz;
		start += sz;
		if (code_point == 0) break;
	}
	if (real_size != 0) return false;

	*used_size += expected_length;
	return true;
}

/**
 * Load a TEXT data block into the object.
 * @param rcd_file Input file.
 * @return Load was successful.
 */
bool TextData::Load(RcdFileReader *rcd_file)
{
	uint8 buffer[64*1024]; // Arbitrary sized block of temporary memory to store the text data.
	size_t used_size = 0;
	uint32 length = rcd_file->size;
	if (rcd_file->version != 2) return false;

	TextString strings[MAX_NUM_TEXT_STRINGS];
	uint used_strings = 0;
	while (length > 0) {
		if (used_strings >= lengthof(strings)) return false; // Too many text strings.

		if (length < 3) return false;
		int str_length   = rcd_file->GetUInt16();
		int ident_length = rcd_file->GetUInt8();

		if (static_cast<uint32>(str_length) > length) return false; // String does not fit in the block.
		length -= 3;

		if (ident_length + 2 + 1 >= str_length) return false; // No space for translations.
		int trs_length = str_length - (ident_length + 2 + 1);

		/* Read string name. */
		strings[used_strings].name = (char *)buffer + used_size;
		if (!ReadUtf8Text(rcd_file, ident_length, buffer, lengthof(buffer), &used_size)) return false;
		length -= ident_length;

		while (trs_length > 0) {
			if (length < 3) return false;
			int tr_length   = rcd_file->GetUInt16();
			int lang_length = rcd_file->GetUInt8();
			length -= 3;

			if (tr_length > trs_length) return false;
			if (lang_length + 2 + 1 >= tr_length) return false;
			int text_length = tr_length - (lang_length + 2 + 1);

			uint8 lang_buffer[1000]; // Arbitrary sized block to store the language name or a single string.
			size_t used = 0;

			/* Read translation language string. */
			if (!ReadUtf8Text(rcd_file, lang_length, lang_buffer, lengthof(lang_buffer), &used)) return false;
			length -= lang_length;

			int lang_idx = GetLanguageIndex((char *)lang_buffer);
			/* Read translation text. */
			if (lang_idx >= 0) {
				strings[used_strings].languages[lang_idx] = buffer + used_size;
				if (!ReadUtf8Text(rcd_file, text_length, buffer, lengthof(buffer), &used_size)) return false;
			} else {
				/* Illegal language, read into a dummy buffer. */
				used = 0;
				if (!ReadUtf8Text(rcd_file, text_length, lang_buffer, lengthof(lang_buffer), &used)) return false;
			}
			length -= text_length;

			trs_length -= 3 + lang_length + text_length;
		}
		assert(trs_length == 0);
		used_strings++;
	}
	assert (length == 0);

	this->strings = new TextString[used_strings];
	this->string_count = used_strings;
	this->text_data = new uint8[used_size];
	if (this->strings == nullptr || this->text_data == nullptr) return false;

	memcpy(this->text_data, buffer, used_size);
	for (uint i = 0; i < used_strings; i++) {
		this->strings[i].name = (strings[i].name == nullptr) ? nullptr : (char *)this->text_data + ((uint8 *)(strings[i].name) - buffer);
		for (uint lng = 0; lng < LANGUAGE_COUNT; lng++) {
			this->strings[i].languages[lng] = (strings[i].languages[lng] == nullptr)
					? nullptr : this->text_data + (strings[i].languages[lng] - buffer);
		}
	}
	return true;
}

/**
 * Get a sprite reference from the \a rcd_file, retrieve the corresponding sprite, and put it in the \a spr destination.
 * @param rcd_file File to load from.
 * @param sprites Sprites already loaded from this file.
 * @param [out] spr Pointer to write the loaded sprite to.
 * @return Loading was successful.
 */
bool LoadSpriteFromFile(RcdFileReader *rcd_file, const ImageMap &sprites, ImageData **spr)
{
	uint32 val = rcd_file->GetUInt32();
	if (val == 0) {
		*spr = nullptr;
		return true;
	}
	const auto iter = sprites.find(val);
	if (iter == sprites.end()) return false;
	*spr = iter->second;
	return true;
}

/**
 * Get a text reference from the \a rcd_file, retrieve the corresponding text data, and put it in the \a txt destination.
 * @param rcd_file File to load from.
 * @param texts Texts already loaded from this file.
 * @param [out] txt Pointer to write the loaded text data reference to.
 * @return Loading was successful.
 */
bool LoadTextFromFile(RcdFileReader *rcd_file, const TextMap &texts, TextData **txt)
{
	uint32 val = rcd_file->GetUInt32();
	if (val == 0) {
		*txt = nullptr;
		return true;
	}
	const auto iter = texts.find(val);
	if (iter == texts.end()) return false;
	*txt = iter->second;
	return true;
}

SurfaceData::SurfaceData()
{
	for (uint i = 0; i < lengthof(this->surface); i++) this->surface[i] = nullptr;
}

/**
 * Load a surface game block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 * @return Loading was successful.
 */
bool SpriteManager::LoadSURF(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	if (rcd_file->version != 4 || rcd_file->size != 2 + 2 + 2 + 4 * NUM_SLOPE_SPRITES) return false;

	uint16 gt = rcd_file->GetUInt16(); // Ground type bytes.
	uint8 type = GTP_INVALID;
	if (gt == 16) type = GTP_GRASS0;
	if (gt == 17) type = GTP_GRASS1;
	if (gt == 18) type = GTP_GRASS2;
	if (gt == 19) type = GTP_GRASS3;
	if (gt == 32) type = GTP_DESERT;
	if (gt == 48) type = GTP_CURSOR_TEST;
	if (type == GTP_INVALID) return false; // Unknown type of ground.

	uint16 width  = rcd_file->GetUInt16();
	rcd_file->GetUInt16(); /// \todo Remove height from RCD block.

	SpriteStorage *ss = this->GetSpriteStore(width);
	if (ss == nullptr || type >= GTP_COUNT) return false;
	SurfaceData *sd = &ss->surface[type];
	for (uint sprnum = 0; sprnum < NUM_SLOPE_SPRITES; sprnum++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &sd->surface[sprnum])) return false;
	}
	return true;
}

TileSelection::TileSelection()
{
	for (uint i = 0; i < lengthof(this->surface); i++) this->surface[i] = nullptr;
}

/**
 * Load a tile selection block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 * @return Loading was successful.
 */
bool SpriteManager::LoadTSEL(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	if (rcd_file->version != 2 || rcd_file->size != 2 + 2 + 4 * NUM_SLOPE_SPRITES) return false;

	uint16 width  = rcd_file->GetUInt16();
	rcd_file->GetUInt16(); /// \todo Remove height from RCD block.

	SpriteStorage *ss = this->GetSpriteStore(width);
	if (ss == nullptr) return false;
	TileSelection *ts = &ss->tile_select;
	for (uint sprnum = 0; sprnum < NUM_SLOPE_SPRITES; sprnum++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &ts->surface[sprnum])) return false;
	}
	return true;
}

Path::Path()
{
	for (uint i = 0; i < lengthof(this->sprites); i++) this->sprites[i] = nullptr;
}

/**
 * Load a path sprites block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 * @return Loading was successful.
 */
bool SpriteManager::LoadPATH(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	if (rcd_file->version != 1 || rcd_file->size != 2 + 2 + 2 + 4 * PATH_COUNT) return false;

	uint16 type = rcd_file->GetUInt16();
	if (type != 16) return false; // Only 'concrete' paths exist.

	uint16 width  = rcd_file->GetUInt16();
	rcd_file->GetUInt16(); /// \todo Remove height from RCD block.

	SpriteStorage *ss = this->GetSpriteStore(width);
	if (ss == nullptr) return false;
	Path *path = &ss->path_sprites;
	for (uint sprnum = 0; sprnum < PATH_COUNT; sprnum++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &path->sprites[sprnum])) return false;
	}
	return true;
}

TileCorners::TileCorners()
{
	for (uint v = 0; v < VOR_NUM_ORIENT; v++) {
		for (uint i = 0; i < NUM_SLOPE_SPRITES; i++) {
			this->sprites[v][i] = nullptr;
		}
	}
}

/**
 * Load a path sprites block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 * @return Loading was successful.
 */
bool SpriteManager::LoadTCOR(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	if (rcd_file->version != 2 || rcd_file->size != 2 + 2 + 4 * VOR_NUM_ORIENT * NUM_SLOPE_SPRITES) return false;

	uint16 width  = rcd_file->GetUInt16();
	rcd_file->GetUInt16(); /// \todo Remove height from RCD block.

	SpriteStorage *ss = this->GetSpriteStore(width);
	if (ss == nullptr) return false;
	TileCorners *tc = &ss->tile_corners;
	for (uint v = 0; v < VOR_NUM_ORIENT; v++) {
		for (uint sprnum = 0; sprnum < NUM_SLOPE_SPRITES; sprnum++) {
			if (!LoadSpriteFromFile(rcd_file, sprites, &tc->sprites[v][sprnum])) return false;
		}
	}
	return true;
}

Foundation::Foundation()
{
	for (uint i = 0; i < lengthof(this->sprites); i++) this->sprites[i] = nullptr;
}

/**
 * Load a path sprites block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 * @return Loading was successful.
 */
bool SpriteManager::LoadFUND(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	if (rcd_file->version != 1 || rcd_file->size != 2 + 2 + 2 + 4 * 6) return false;

	uint16 tp = rcd_file->GetUInt16();
	uint16 type = FDT_INVALID;
	if (tp == 16) type = FDT_GROUND;
	if (tp == 32) type = FDT_WOOD;
	if (tp == 48) type = FDT_BRICK;
	if (type == FDT_INVALID) return false;

	uint16 width  = rcd_file->GetUInt16();
	rcd_file->GetUInt16(); /// \todo Remove height from RCD block.

	SpriteStorage *ss = this->GetSpriteStore(width);
	if (ss == nullptr || type >= FDT_COUNT) return false;
	Foundation *fn = &ss->foundation[type];
	for (uint sprnum = 0; sprnum < lengthof(fn->sprites); sprnum++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &fn->sprites[sprnum])) return false;
	}
	return true;
}

Platform::Platform() : RcdBlock()
{
	this->width = 0;
	this->height = 0;
	this->flat[0] = nullptr; this->flat[1] = nullptr;
	for (uint i = 0; i < lengthof(this->ramp); i++) this->ramp[i] = nullptr;
}

Platform::~Platform()
{
}

/**
 * Load a platform sprites block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 * @return Loading was successful.
 */
bool Platform::Load(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	if (rcd_file->version != 2 || rcd_file->size != 2 + 2 + 2 + 2 * 4 + 12 * 4) return false;

	this->width  = rcd_file->GetUInt16();
	this->height = rcd_file->GetUInt16();
	uint16 type = rcd_file->GetUInt16();
	if (type != 16) return false; // Only accept type 16 'wood'.

	if (!LoadSpriteFromFile(rcd_file, sprites, &this->flat[0])) return false;
	if (!LoadSpriteFromFile(rcd_file, sprites, &this->flat[1])) return false;
	for (uint sprnum = 0; sprnum < lengthof(this->ramp); sprnum++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &this->ramp[sprnum])) return false;
	}
	for (uint sprnum = 0; sprnum < lengthof(this->right_ramp); sprnum++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &this->right_ramp[sprnum])) return false;
	}
	for (uint sprnum = 0; sprnum < lengthof(this->left_ramp); sprnum++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &this->left_ramp[sprnum])) return false;
	}
	return true;
}

Support::Support() : RcdBlock()
{
	this->type = 0;
	this->width = 0;
	this->height = 0;
	for (uint sprnum = 0; sprnum < lengthof(this->sprites); sprnum++) this->sprites[sprnum] = nullptr;
}

Support::~Support()
{
}

/**
 * Load a support sprites block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 * @return Loading was successful.
 */
bool Support::Load(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	if (rcd_file->version != 1 || rcd_file->size != 2 + 2 + 2 + SSP_COUNT * 4) return false;

	this->type = rcd_file->GetUInt16();
	if (this->type != 16) return false; // Only accept type 16 'wood'.
	this->width  = rcd_file->GetUInt16();
	this->height = rcd_file->GetUInt16();
	for (uint sprnum = 0; sprnum < lengthof(this->sprites); sprnum++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &this->sprites[sprnum])) return false;
	}
	return true;
}

DisplayedObject::DisplayedObject()
{
	for (uint i = 0; i < lengthof(this->sprites); i++) this->sprites[i] = nullptr;
}

/**
 * Load a displayed object block.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 * @return Loading was successful.
 */
bool SpriteManager::LoadBDIR(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	if (rcd_file->version != 1 || rcd_file->size != 2 + 4 * 4) return false;

	uint16 width = rcd_file->GetUInt16();

	SpriteStorage *ss = this->GetSpriteStore(width);
	if (ss == nullptr) return false;
	DisplayedObject *dob = &ss->build_arrows;
	for (uint sprnum = 0; sprnum < lengthof(dob->sprites); sprnum++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &dob->sprites[sprnum])) return false;
	}
	return true;
}

/** %Animation default constructor. */
Animation::Animation() : RcdBlock()
{
	this->frame_count = 0;
	this->person_type = PERSON_INVALID;
	this->anim_type = ANIM_INVALID;
	this->frames = nullptr;
}

/** %Animation destructor. */
Animation::~Animation()
{
	delete[] this->frames;
}

/**
 * Decode a read value to the internal representation of a person type.
 * @param pt Value read from the file.
 * @return Decoded person type, or #PERSON_INVALID when decoding fails.
 */
static PersonType DecodePersonType(uint8 pt)
{
	switch (pt) {
		case  0: return PERSON_ANY;
		case  8: return PERSON_PILLAR;
		case 16: return PERSON_EARTH;
		default: return PERSON_INVALID;
	}
}

/**
 * Load an animation.
 * @param rcd_file RCD file used for loading.
 * @return Loading was successful.
 */
bool Animation::Load(RcdFileReader *rcd_file)
{
	const uint BASE_LENGTH = 1 + 2 + 2;

	size_t length = rcd_file->size;
	if (rcd_file->version != 2 || length < BASE_LENGTH) return false;
	this->person_type = DecodePersonType(rcd_file->GetUInt8());
	if (this->person_type == PERSON_INVALID) return false;

	uint16 at = rcd_file->GetUInt16();
	if (at < ANIM_BEGIN || at > ANIM_LAST) return false;
	this->anim_type = (AnimationType)at;

	this->frame_count = rcd_file->GetUInt16();
	if (length != BASE_LENGTH + this->frame_count * 6) return false;
	this->frames = new AnimationFrame[this->frame_count];
	if (this->frames == nullptr || this->frame_count == 0) return false;

	for (uint i = 0; i < this->frame_count; i++) {
		AnimationFrame *frame = this->frames + i;

		frame->duration = rcd_file->GetUInt16();
		if (frame->duration == 0 || frame->duration >= 1000) return false; // Arbitrary sanity limit.

		frame->dx = rcd_file->GetInt16();
		if (frame->dx < -100 || frame->dx > 100) return false; // Arbitrary sanity limit.

		frame->dy = rcd_file->GetInt16();
		if (frame->dy < -100 || frame->dy > 100) return false; // Arbitrary sanity limit.
	}
	return true;
}

/** Animation sprites default constructor. */
AnimationSprites::AnimationSprites() : RcdBlock()
{
	this->width = 0;
	this->frame_count = 0;
	this->person_type = PERSON_INVALID;
	this->anim_type = ANIM_INVALID;
	this->sprites = nullptr;
}

/** Animation sprites destructor. */
AnimationSprites::~AnimationSprites()
{
	delete[] this->sprites;
}

/**
 * Load the sprites of an animation.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 * @return Loading was successful.
 */
bool AnimationSprites::Load(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	const uint BASE_LENGTH = 2 + 1 + 2 + 2;

	size_t length = rcd_file->size;
	if (rcd_file->version != 1 || length < BASE_LENGTH) return false;
	this->width = rcd_file->GetUInt16();

	this->person_type = DecodePersonType(rcd_file->GetUInt8());
	if (this->person_type == PERSON_INVALID) return false;

	uint16 at = rcd_file->GetUInt16();
	if (at < ANIM_BEGIN || at > ANIM_LAST) return false;
	this->anim_type = (AnimationType)at;

	this->frame_count = rcd_file->GetUInt16();
	if (length != BASE_LENGTH + this->frame_count * 4) return false;
	this->sprites = new ImageData *[this->frame_count];
	if (this->sprites == nullptr || this->frame_count == 0) return false;

	for (uint i = 0; i < this->frame_count; i++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &this->sprites[i])) return false;
	}
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
		this->normal[i] = nullptr;
		this->pressed[i] = nullptr;
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
 * @param sprites Sprites loaded from this file.
 * @return Load was successful.
 */
bool GuiSprites::LoadGBOR(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	if (rcd_file->version != 1 || rcd_file->size != 2 + 8 * 1 + WBS_COUNT * 4) return false;

	/* Select sprites to save to. */
	uint16 tp = rcd_file->GetUInt16(); // Widget type.
	BorderSpriteData *sprdata = nullptr;
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
		if (pressed) {
			if (!LoadSpriteFromFile(rcd_file, sprites, &sprdata->pressed[sprnum])) return false;
		} else {
			if (!LoadSpriteFromFile(rcd_file, sprites, &sprdata->normal[sprnum])) return false;
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
		this->sprites[sprnum] = nullptr;
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
 * Load checkbox and radio button GUI sprites.
 * @param rcd_file RCD file being loaded.
 * @param sprites Sprites loaded from this file.
 * @return Load was successful.
 * @todo Load width and height from the RCD file too.
 */
bool GuiSprites::LoadGCHK(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	if (rcd_file->version != 1 || rcd_file->size != 2 + WCS_COUNT * 4) return false;

	/* Select sprites to save to. */
	uint16 tp = rcd_file->GetUInt16(); // Widget type.
	CheckableWidgetSpriteData *sprdata = nullptr;
	switch (tp) {
		case 96:  sprdata = &this->checkbox; break;
		case 112: sprdata = &this->radio_button; break;
		default:
			return false;
	}

	sprdata->width = 0;
	sprdata->height = 0;
	for (uint sprnum = 0; sprnum < WCS_COUNT; sprnum++) {
		ImageData *spr;
		if (!LoadSpriteFromFile(rcd_file, sprites, &spr)) return false;
		sprdata->sprites[sprnum] = spr;

		if (spr != nullptr) {
			sprdata->width  = std::max(sprdata->width, spr->width);
			sprdata->height = std::max(sprdata->height, spr->height);
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
		this->normal[sprnum] = nullptr;
		this->shaded[sprnum] = nullptr;
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
 * @param sprites Sprites loaded from this file.
 * @return Load was successful.
 * @todo Move widget_type further to the top in the RCD file block.
 */
bool GuiSprites::LoadGSLI(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	if (rcd_file->version != 1 || rcd_file->size != 3 * 1 + 2 + WSS_COUNT * 4) return false;

	uint8 min_length = rcd_file->GetUInt8();
	uint8 stepsize = rcd_file->GetUInt8();
	uint8 height = rcd_file->GetUInt8();

	/* Select sprites to save to. */
	uint16 tp = rcd_file->GetUInt16(); // Widget type.
	SliderSpriteData *sprdata = nullptr;
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
		if (shaded) {
			if (!LoadSpriteFromFile(rcd_file, sprites, &sprdata->shaded[sprnum])) return false;
		} else {
			if (!LoadSpriteFromFile(rcd_file, sprites, &sprdata->normal[sprnum])) return false;
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
		this->normal[sprnum] = nullptr;
		this->shaded[sprnum] = nullptr;
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
 * @param sprites Sprites loaded from this file.
 * @return Load was successful.
 * @todo Move widget_type further to the top in the RCD file block.
 * @todo Add width of the scrollbar in the RCD file block.
 */
bool GuiSprites::LoadGSCL(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	if (rcd_file->version != 1 || rcd_file->size != 4 * 1 + 2 + WLS_COUNT * 4) return false;

	uint8 min_length_bar = rcd_file->GetUInt8();
	uint8 stepsize_back = rcd_file->GetUInt8();
	uint8 min_slider = rcd_file->GetUInt8();
	uint8 stepsize_slider = rcd_file->GetUInt8();

	/* Select sprites to save to. */
	uint16 tp = rcd_file->GetUInt16(); // Widget type.
	ScrollbarSpriteData *sprdata = nullptr;
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
		ImageData *spr;
		if (!LoadSpriteFromFile(rcd_file, sprites, &spr)) return false;

		if (shaded) {
			sprdata->shaded[sprnum] = spr;
		} else {
			sprdata->normal[sprnum] = spr;
		}

		if (spr != nullptr) {
			max_width  = std::max(max_width, spr->width);
			max_height = std::max(max_height, spr->height);
		}
	}

	sprdata->height = (vertical) ? max_width : max_height;
	return true;
}

/**
 * Load GUI slope selection sprites.
 * @param rcd_file RCD file being loaded.
 * @param sprites Sprites loaded from this file.
 * @param texts Texts loaded from this file.
 * @return Load was successful.
 */
bool GuiSprites::LoadGSLP(RcdFileReader *rcd_file, const ImageMap &sprites, const TextMap &texts)
{
	const uint8 indices[] = {TSL_STRAIGHT_DOWN, TSL_STEEP_DOWN, TSL_DOWN, TSL_FLAT, TSL_UP, TSL_STEEP_UP, TSL_STRAIGHT_UP};

	/* 'indices' entries of slope sprites, bends, banking, 4 triangle arrows,
	 * 4 entries with rotation sprites, 2 button sprites, one entry with a text block.
	 */
	if (rcd_file->version != 7 || rcd_file->size != (lengthof(indices) + TBN_COUNT + TPB_COUNT + 4 + 2 + 2 + 1 + TC_END + 1 + WES_COUNT + 4 + 2) * 4 + 4) return false;

	for (uint i = 0; i < lengthof(indices); i++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &this->slope_select[indices[i]])) return false;
	}
	for (uint i = 0; i < TBN_COUNT; i++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &this->bend_select[i])) return false;
	}
	for (uint i = 0; i < TPB_COUNT; i++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &this->bank_select[i])) return false;
	}
	if (!LoadSpriteFromFile(rcd_file, sprites, &this->triangle_left)) return false;
	if (!LoadSpriteFromFile(rcd_file, sprites, &this->triangle_right)) return false;
	if (!LoadSpriteFromFile(rcd_file, sprites, &this->triangle_up)) return false;
	if (!LoadSpriteFromFile(rcd_file, sprites, &this->triangle_down)) return false;
	for (uint i = 0; i < 2; i++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &this->platform_select[i])) return false;
	}
	for (uint i = 0; i < 2; i++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &this->power_select[i])) return false;
	}

	if (!LoadSpriteFromFile(rcd_file, sprites, &this->disabled)) return false;

	for (uint i = 0; i < TC_END; i++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &this->compass[i])) return false;
	}
	if (!LoadSpriteFromFile(rcd_file, sprites, &this->bulldozer)) return false;
	for (uint i = 0; i < WES_COUNT; i++) {
		if (!LoadSpriteFromFile(rcd_file, sprites, &this->weather[i])) return false;
	}

	if (!LoadSpriteFromFile(rcd_file, sprites, &this->rot_2d_pos)) return false;
	if (!LoadSpriteFromFile(rcd_file, sprites, &this->rot_2d_neg)) return false;
	if (!LoadSpriteFromFile(rcd_file, sprites, &this->rot_3d_pos)) return false;
	if (!LoadSpriteFromFile(rcd_file, sprites, &this->rot_3d_neg)) return false;

	if (!LoadSpriteFromFile(rcd_file, sprites, &this->close_sprite)) return false;
	if (!LoadSpriteFromFile(rcd_file, sprites, &this->dot_sprite)) return false;

	if (!LoadTextFromFile(rcd_file, texts, &this->text)) return false;
	_language.RegisterStrings(*this->text, _gui_strings_table, STR_GUI_START);
	return true;
}

/** Default constructor. */
GuiSprites::GuiSprites()
{
	this->Clear();
}

/** Clear all GUI sprite data. */
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

	for (uint i = 0; i < lengthof(this->slope_select); i++) this->slope_select[i] = nullptr;
	for (uint i = 0; i < TBN_COUNT; i++) this->bend_select[i] = nullptr;
	for (uint i = 0; i < TPB_COUNT; i++) this->bank_select[i] = nullptr;
	for (uint i = 0; i < 2; i++) this->platform_select[i] = nullptr;
	for (uint i = 0; i < 2; i++) this->power_select[i] = nullptr;
	this->triangle_left = nullptr;
	this->triangle_right = nullptr;
	this->triangle_up = nullptr;
	this->triangle_down = nullptr;
	this->disabled = nullptr;
	this->rot_2d_pos = nullptr;
	this->rot_2d_neg = nullptr;
	this->rot_3d_pos = nullptr;
	this->rot_3d_neg = nullptr;
	this->close_sprite = nullptr;
	this->dot_sprite = nullptr;
	this->bulldozer = nullptr;
	for (uint i = 0; i < TC_END; i++) this->compass[i] = nullptr;
	for (uint i = 0; i < WES_COUNT; i++) this->weather[i] = nullptr;
}

/**
 * Have essential GUI sprites been loaded to be used in a display.
 * This is the minimum number of sprites needed to display an error message.
 * @return Sufficient data has been loaded.
 */
bool GuiSprites::HasSufficientGraphics() const
{
	return this->titlebar.IsLoaded() && this->button.IsLoaded()
			&& this->rounded_button.IsLoaded() && this->frame.IsLoaded()
			&& this->panel.IsLoaded()          && this->inset_frame.IsLoaded()
			&& this->checkbox.IsLoaded()       && this->radio_button.IsLoaded()
			&& this->hor_scroll.IsLoaded()     && this->vert_scroll.IsLoaded()
			&& this->close_sprite != nullptr;
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
	this->animations.clear(); // Animation sprites objects are managed by the RCD blocks.
}

/**
 * Add platform block.
 * @param plat New platform block.
 * @pre Width of the platform block must match with #size.
 */
void SpriteStorage::AddPlatform(Platform *plat)
{
	assert(plat->width == this->size);
	this->platform = plat;
}

/**
 * Add support sprites block.
 * @param supp New support block.
 * @pre Width of the platform block must match with #size.
 */
void SpriteStorage::AddSupport(Support *supp)
{
	assert(supp->width == this->size);
	this->support = supp;
}

/**
 * Remove any sprites that were loaded for the provided animation.
 * @param anim_type %Animation type to remove.
 * @param pers_type Person type to remove.
 */
void SpriteStorage::RemoveAnimations(AnimationType anim_type, PersonType pers_type)
{
	for (auto iter = this->animations.find(anim_type); iter != this->animations.end(); ) {
		AnimationSprites *an_spr = iter->second;
		if (an_spr->anim_type != anim_type) return;
		if (an_spr->person_type == pers_type) {
			auto iter2 = iter;
			++iter2;
			this->animations.erase(iter);
			iter = iter2;
		}
	}
}

/**
 * Add an animation to the sprite manager.
 * @param an_spr %Animation sprites to add.
 */
void SpriteStorage::AddAnimationSprites(AnimationSprites *an_spr)
{
	assert(an_spr->width == this->size);
	this->animations.insert(std::make_pair(an_spr->anim_type, an_spr));
}

/** Sprite manager constructor. */
SpriteManager::SpriteManager() : store(64)
{
	_gui_sprites.Clear();
	this->blocks = nullptr;
}

/** Sprite manager destructor. */
SpriteManager::~SpriteManager()
{
	_gui_sprites.Clear();
	this->animations.clear(); // Blocks get deleted through the 'this->blocks' below.
	while (this->blocks != nullptr) {
		RcdBlock *next_block = this->blocks->next;
		delete this->blocks;
		this->blocks = next_block;
	}
	/* Sprite stores will be deleted soon as well. */
}

/**
 * Load sprites from the disk.
 * @param filename Name of the RCD file to load.
 * @return Error message if load failed, else \c nullptr.
 * @todo Try to re-use already loaded blocks.
 * @todo Code will use last loaded surface as grass.
 */
const char *SpriteManager::Load(const char *filename)
{
	RcdFileReader rcd_file(filename);
	if (!rcd_file.CheckFileHeader("RCDF", 2)) return "Bad header";

	ImageMap sprites; // Sprites loaded from this file.
	TextMap  texts;   // Texts loaded from this file.
	TrackPiecesMap track_pieces; // Track pieces loaded from this file.

	/* Load blocks. */
	for (uint blk_num = 1;; blk_num++) {
		if (!rcd_file.ReadBlockHeader()) return nullptr; // End reached.

		/* Skip meta blocks. */
		if (strcmp(rcd_file.name, "INFO") == 0) {
			rcd_file.SkipBytes(rcd_file.size);
			continue;
		}

		if (strcmp(rcd_file.name, "8PXL") == 0 || strcmp(rcd_file.name, "32PX") == 0) {
			ImageData *imd = LoadImage(&rcd_file);
			if (imd == nullptr) {
				return "Image data loading failed";
			}
			std::pair<uint, ImageData *> p(blk_num, imd);
			sprites.insert(p);
			continue;
		}

		if (strcmp(rcd_file.name, "SURF") == 0) {
			if (!this->LoadSURF(&rcd_file, sprites)) {
				return "Surface block loading failed.";
			}
			continue;
		}

		if (strcmp(rcd_file.name, "TSEL") == 0) {
			if (!this->LoadTSEL(&rcd_file, sprites)) {
				return "Tile-selection block loading failed.";
			}
			continue;
		}

		if (strcmp(rcd_file.name, "PATH") == 0) {
			if (!this->LoadPATH(&rcd_file, sprites)) {
				return "Path-sprites block loading failed.";
			}
			continue;
		}

		if (strcmp(rcd_file.name, "TCOR") == 0) {
			if (!this->LoadTCOR(&rcd_file, sprites)) {
				return "Tile-corners block loading failed.";
			}
			continue;
		}

		if (strcmp(rcd_file.name, "FUND") == 0) {
			if (!this->LoadFUND(&rcd_file, sprites)) {
				return "Foundation block loading failed.";
			}
			continue;
		}

		if (strcmp(rcd_file.name, "PLAT") == 0) {
			Platform *block = new Platform;
			if (!block->Load(&rcd_file, sprites)) {
				delete block;
				return "Platform block loading failed.";
			}
			this->AddBlock(block);

			this->store.AddPlatform(block);
			continue;
		}

		if (strcmp(rcd_file.name, "SUPP") == 0) {
			Support *block = new Support;
			if (!block->Load(&rcd_file, sprites)) {
				delete block;
				return "Support block loading failed.";
			}
			this->AddBlock(block);

			this->store.AddSupport(block);
			continue;
		}

		if (strcmp(rcd_file.name, "BDIR") == 0) {
			if (!this->LoadBDIR(&rcd_file, sprites)) {
				return "Build arrows block loading failed.";
			}
			continue;
		}

		if (strcmp(rcd_file.name, "GCHK") == 0) {
			if (!_gui_sprites.LoadGCHK(&rcd_file, sprites)) {
				return "Loading Checkable GUI sprites failed.";
			}
			continue;
		}

		if (strcmp(rcd_file.name, "GBOR") == 0) {
			if (!_gui_sprites.LoadGBOR(&rcd_file, sprites)) {
				return "Loading Border GUI sprites failed.";
			}
			continue;
		}

		if (strcmp(rcd_file.name, "GSLI") == 0) {
			if (!_gui_sprites.LoadGSLI(&rcd_file, sprites)) {
				return "Loading Slider bar GUI sprites failed.";
			}
			continue;
		}

		if (strcmp(rcd_file.name, "GSCL") == 0) {
			if (!_gui_sprites.LoadGSCL(&rcd_file, sprites)) {
				return "Loading Scrollbar GUI sprites failed.";
			}
			continue;
		}

		if (strcmp(rcd_file.name, "GSLP") == 0) {
			if (!_gui_sprites.LoadGSLP(&rcd_file, sprites, texts)) {
				return "Loading slope selection GUI sprites failed.";
			}
			continue;
		}

		if (strcmp(rcd_file.name, "ANIM") == 0) {
			Animation *anim = new Animation;
			if (!anim->Load(&rcd_file)) {
				delete anim;
				return "Animation failed to load.";
			}
			if (anim->person_type == PERSON_INVALID || anim->anim_type == ANIM_INVALID) {
				delete anim;
				return "Unknown animation.";
			}
			this->AddBlock(anim);
			this->AddAnimation(anim);
			this->store.RemoveAnimations(anim->anim_type, (PersonType)anim->person_type);
			continue;
		}

		if (strcmp(rcd_file.name, "ANSP") == 0) {
			AnimationSprites *an_spr = new AnimationSprites;
			if (!an_spr->Load(&rcd_file, sprites)) {
				delete an_spr;
				return "Animation sprites failed to load.";
			}
			if (an_spr->person_type == PERSON_INVALID || an_spr->anim_type == ANIM_INVALID) {
				delete an_spr;
				return "Unknown animation.";
			}
			this->AddBlock(an_spr);
			this->store.AddAnimationSprites(an_spr);
			continue;
		}

		if (strcmp(rcd_file.name, "PRSG") == 0) {
			if (!LoadPRSG(&rcd_file)) {
				return "Graphics Person type data failed to load.";
			}
			continue;
		}

		if (strcmp(rcd_file.name, "TEXT") == 0) {
			TextData *txt = new TextData;
			if (!txt->Load(&rcd_file)) {
				delete txt;
				return "Text block failed to load.";
			}
			this->AddBlock(txt);

			std::pair<uint, TextData *> p(blk_num, txt);
			texts.insert(p);
			continue;
		}

		if (strcmp(rcd_file.name, "SHOP") == 0) {
			ShopType *shop_type = new ShopType;
			if (!shop_type->Load(&rcd_file, sprites, texts)) {
				delete shop_type;
				return "Shop type failed to load.";
			}
			_rides_manager.AddRideType(shop_type);
			continue;
		}

		if (strcmp(rcd_file.name, "TRCK") == 0) {
			auto tp = std::make_shared<TrackPiece>();
			if (!tp->Load(&rcd_file, sprites)) {
				return "Track piece failed to load.";
			}
			track_pieces.insert({blk_num, tp});
			continue;
		}

		if (strcmp(rcd_file.name, "RCST") == 0) {
			CoasterType *ct = new CoasterType;
			if (!ct->Load(&rcd_file, texts, track_pieces)) {
				delete ct;
				return "Coaster type failed to load.";
			}
			_rides_manager.AddRideType(ct);
			continue;
		}

		if (strcmp(rcd_file.name, "CSPL") == 0) {
			if (!LoadCoasterPlatform(&rcd_file, sprites)) {
				return "Coaster platform failed to load.";
			}
			continue;
		}

		if (strcmp(rcd_file.name, "CARS") == 0) {
			CarType *ct = GetNewCarType();
			if (ct == nullptr) return "No room to store a car type.";
			if (!ct->Load(&rcd_file, sprites)) return "Car type failed to load.";
			continue;
		}

		/* Unknown block in the RCD file. Skip the block. */
		fprintf(stderr, "Unknown RCD block '%s', version %i, ignoring it\n", rcd_file.name, rcd_file.version);
		rcd_file.SkipBytes(rcd_file.size);
	}
}

/**
 * Get the sprite storage belonging to a given size of sprites.
 * @param width Tile width of the sprites.
 * @return Sprite storage object for the given width if it exists, else \c nullptr.
 */
SpriteStorage *SpriteManager::GetSpriteStore(uint16 width)
{
	if (width == 64) return &this->store;
	return nullptr;
}

/** Load all useful RCD files found by #_rcd_collection, into the program. */
void SpriteManager::LoadRcdFiles()
{
	for (auto &entry : _rcd_collection.rcdfiles) {
		const char *fname = entry.second.path.c_str();
		const char *mesg = this->Load(fname);
		if (mesg != nullptr) fprintf(stderr, "Error while reading \"%s\": %s\n", fname, mesg);
	}
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
 * Get a sprite store of a given size.
 * @param size Requested size.
 * @return Sprite store with sprites of the requested size, if it exists, else \c nullptr.
 * @todo Add support for other sprite sizes as well.
 */
const SpriteStorage *SpriteManager::GetSprites(uint16 size) const
{
	if (size != 64) return nullptr;
	return &this->store;
}

/**
 * Add an animation to the sprite manager.
 * @param anim %Animation object to add.
 */
void SpriteManager::AddAnimation(Animation *anim)
{
	this->animations.insert(std::make_pair(anim->anim_type, anim));
}

/**
 * Set the size of the rectangle for fitting a range of sprites.
 * @param first First sprite number to fit.
 * @param end One beyond the last sprite number to fit.
 * @param rect [out] Size of the rectangle required for fitting all sprites.
 */
void SpriteManager::SetSpriteSize(uint16 first, uint16 end, Rectangle16 &rect)
{
	for (uint16 i = first; i < end; i++) {
		const ImageData *imd = this->GetTableSprite(i);
		if (imd == nullptr || imd->width == 0 || imd->height == 0) continue;
		rect.AddPoint(imd->xoffset, imd->yoffset);
		rect.AddPoint(imd->xoffset + (int16)imd->width - 1, imd->yoffset + (int16)imd->height - 1);
	}
}

/**
 * Get the size of a GUI image according to the table in <tt>table/gui_sprites.h</tt>.
 * @param number Number of the sprite to get.
 * @return The size of the sprite (which may be a default if there is no sprite).
 * @note Return value is kept until the next call.
 */
const Rectangle16 &SpriteManager::GetTableSpriteSize(uint16 number)
{
	static Rectangle16 result;
	static Rectangle16 slopes;
	static Rectangle16 arrows;
	static Rectangle16 bends;
	static Rectangle16 banks;
	static Rectangle16 platforms;
	static Rectangle16 powers;
	static Rectangle16 compasses;
	static Rectangle16 weathers;

	if (number >= SPR_GUI_COMPASS_START && number < SPR_GUI_COMPASS_END) {
		if (compasses.width == 0) SetSpriteSize(SPR_GUI_COMPASS_START, SPR_GUI_COMPASS_END, compasses);
		return compasses;
	}
	if (number >= SPR_GUI_WEATHER_START && number < SPR_GUI_WEATHER_END) {
		if (weathers.width == 0) SetSpriteSize(SPR_GUI_WEATHER_START, SPR_GUI_WEATHER_END, weathers);
		return weathers;
	}
	if (number >= SPR_GUI_SLOPES_START && number < SPR_GUI_SLOPES_END) {
		if (slopes.width == 0) SetSpriteSize(SPR_GUI_SLOPES_START, SPR_GUI_SLOPES_END, slopes);
		return slopes;
	}
	if (number >= SPR_GUI_BUILDARROW_START && number < SPR_GUI_BUILDARROW_END) {
		if (arrows.width== 0) SetSpriteSize(SPR_GUI_BUILDARROW_START, SPR_GUI_BUILDARROW_END, arrows);
		return arrows;
	}
	if (number >= SPR_GUI_BEND_START && number < SPR_GUI_BEND_END) {
		if (bends.width== 0) SetSpriteSize(SPR_GUI_BEND_START, SPR_GUI_BEND_END, bends);
		return bends;
	}
	if (number >= SPR_GUI_BANK_START && number < SPR_GUI_BANK_END) {
		if (banks.width== 0) SetSpriteSize(SPR_GUI_BANK_START, SPR_GUI_BANK_END, banks);
		return banks;
	}
	if (number >= SPR_GUI_HAS_PLATFORM && number <= SPR_GUI_NO_PLATFORM) {
		if (platforms.width == 0) SetSpriteSize(SPR_GUI_HAS_PLATFORM, SPR_GUI_NO_PLATFORM + 1, platforms);
		return platforms;
	}
	if (number >= SPR_GUI_HAS_POWER && number <= SPR_GUI_NO_POWER) {
		if (powers.width == 0) SetSpriteSize(SPR_GUI_HAS_POWER, SPR_GUI_NO_POWER + 1, powers);
		return powers;
	}

	/* 'Simple' single sprites. */
	const ImageData *imd = this->GetTableSprite(number);
	if (imd != nullptr && imd->width != 0 && imd->height != 0) {
		result.width = 0; result.height = 0;
		result.AddPoint(imd->xoffset, imd->yoffset);
		result.AddPoint(imd->xoffset + (int16)imd->width - 1, imd->yoffset + (int16)imd->height - 1);
		return result;
	}

	/* No useful match, return a dummy size. */
	result.base.x = 0; result.base.y = 0;
	result.width = 10; result.height = 10;
	return result;
}

/**
 * Get the image data for the GUI according to the table in <tt>table/gui_sprites.h</tt>.
 * @param number Number of the sprite to get.
 * @return The sprite if available, else \c nullptr.
 * @todo Add lots of missing sprites.
 * @todo Make this more efficient; linearly trying every entry scales badly.
 */
const ImageData *SpriteManager::GetTableSprite(uint16 number) const
{
	if (number >= SPR_GUI_COMPASS_START && number < SPR_GUI_COMPASS_END) return _gui_sprites.compass[number - SPR_GUI_COMPASS_START];
	if (number >= SPR_GUI_WEATHER_START && number < SPR_GUI_WEATHER_END) return _gui_sprites.weather[number - SPR_GUI_WEATHER_START];
	if (number >= SPR_GUI_SLOPES_START && number < SPR_GUI_SLOPES_END)   return _gui_sprites.slope_select[number - SPR_GUI_SLOPES_START];
	if (number >= SPR_GUI_BEND_START   && number < SPR_GUI_BEND_END) return _gui_sprites.bend_select[number - SPR_GUI_BEND_START];
	if (number >= SPR_GUI_BANK_START   && number < SPR_GUI_BANK_END) return _gui_sprites.bank_select[number - SPR_GUI_BANK_START];

	if (number >= SPR_GUI_BUILDARROW_START && number < SPR_GUI_BUILDARROW_END) {
		return this->store.GetArrowSprite(number - SPR_GUI_BUILDARROW_START, VOR_NORTH);
	}

	switch (number) {
		case SPR_GUI_HAS_PLATFORM:   return _gui_sprites.platform_select[0];
		case SPR_GUI_NO_PLATFORM:    return _gui_sprites.platform_select[1];
		case SPR_GUI_HAS_POWER:      return _gui_sprites.power_select[0];
		case SPR_GUI_NO_POWER:       return _gui_sprites.power_select[1];
		case SPR_GUI_TRIANGLE_LEFT:  return _gui_sprites.triangle_left;
		case SPR_GUI_TRIANGLE_RIGHT: return _gui_sprites.triangle_right;
		case SPR_GUI_TRIANGLE_UP:    return _gui_sprites.triangle_up;
		case SPR_GUI_TRIANGLE_DOWN:  return _gui_sprites.triangle_down;
		case SPR_GUI_ROT2D_POS:      return _gui_sprites.rot_2d_pos;
		case SPR_GUI_ROT2D_NEG:      return _gui_sprites.rot_2d_neg;
		case SPR_GUI_ROT3D_POS:      return _gui_sprites.rot_3d_pos;
		case SPR_GUI_ROT3D_NEG:      return _gui_sprites.rot_3d_neg;
		case SPR_GUI_BULLDOZER:      return _gui_sprites.bulldozer;
		default:                     return nullptr;
	}
}

/**
 * Get the animation frames of the requested animation for the provided type of person.
 * @param anim_type %Animation to retrieve.
 * @param per_type %Animation should feature this type of person.
 * @return The requested animation if it is available, else \c nullptr is returned.
 * @todo Put this in a static array to make rendering people much cheaper.
 */
const Animation *SpriteManager::GetAnimation(AnimationType anim_type, PersonType per_type) const
{
	for (auto iter = this->animations.find(anim_type); iter != this->animations.end(); ++iter) {
		const Animation *anim = iter->second;
		if (anim->anim_type != anim_type) break;
		if (anim->person_type == per_type) return anim;
	}
	return nullptr;
}

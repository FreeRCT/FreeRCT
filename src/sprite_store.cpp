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
#include "gentle_thrill_ride_type.h"
#include "coaster.h"
#include "scenery.h"
#include "string_func.h"

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

/**
 * Check an UTF-8 string.
 * @param rcd_file Input file.
 * @param expected_length Expected length of the string in bytes, including terminating \c NUL byte.
 * @param buffer Buffer to copy the string into.
 * @param buf_length Length of the buffer
 * @param expected_null Number of \c NUL bytes expected to occur within the string.
 * @param[out] used_size Length of the used part of the buffer, only updated on successful reading of the string.
 */
static void ReadUtf8Text(RcdFileReader *rcd_file, size_t expected_length, char *buffer, size_t buf_length, int expected_null, size_t *used_size)
{
	if (buf_length < *used_size + expected_length) rcd_file->Error("UTF8 text overflows buffer");
	rcd_file->GetBlob(buffer + *used_size, expected_length);

	int real_size = expected_length;
	char *start = buffer + *used_size;

	for (;;) {
		uint32 code_point;
		int sz = DecodeUtf8Char(start, real_size, &code_point);
		if (sz == 0 || sz > real_size) rcd_file->Error("UTF8: Invalid character bounds");
		real_size -= sz;
		start += sz;
		if (code_point == 0) {
			if (expected_null > 0) {
				--expected_null;
			} else {
				break;
			}
		}
	}
	if (real_size != 0 || expected_null != 0) rcd_file->Error("UTF8: Wrong string length");

	*used_size += expected_length;
}

/**
 * Load a TEXT data block into the object.
 * @param rcd_file Input file.
 */
void TextData::Load(RcdFileReader *rcd_file)
{
	rcd_file->CheckVersion(3);
	char buffer[64*1024]; // Arbitrary sized block of temporary memory to store the text data.
	size_t used_size = 0;
	uint32 length = rcd_file->size;

	TextString strings[MAX_NUM_TEXT_STRINGS];
	uint used_strings = 0;
	while (length > 0) {
		if (used_strings >= lengthof(strings)) rcd_file->Error("Too many text strings");

		rcd_file->CheckMinLength(length, 3, "string header");
		uint16 str_length   = rcd_file->GetUInt16();
		uint8  ident_length = rcd_file->GetUInt8();

		if (static_cast<uint32>(str_length) > length) rcd_file->Error("String does not fit in the block");
		length -= 3;

		if (ident_length + 2 + 1 >= str_length) rcd_file->Error("No space for translations");
		int trs_length = str_length - (ident_length + 2 + 1);

		/* Read string name. */
		strings[used_strings].name = buffer + used_size;
		ReadUtf8Text(rcd_file, ident_length, buffer, lengthof(buffer), 0, &used_size);
		length -= ident_length;

		while (trs_length > 0) {
			rcd_file->CheckMinLength(length, 3, "translation header");
			uint16 tr_length   = rcd_file->GetUInt16();
			uint8  lang_length = rcd_file->GetUInt8();
			length -= 3;

			if (tr_length > trs_length) rcd_file->Error("Translation lengths mismatch");
			if (lang_length + 2 + 1 + 1 >= tr_length) rcd_file->Error("Trailing bytes after translation");
			int text_length = tr_length - (lang_length + 2 + 1 + 1);

			char lang_buffer[1000]; // Arbitrary sized block to store the language name or a single string.
			size_t used = 0;

			/* Read translation language string. */
			ReadUtf8Text(rcd_file, lang_length, lang_buffer, lengthof(lang_buffer), 0, &used);
			length -= lang_length;

			int plural_forms = rcd_file->GetUInt8();
			length -= 1;
			if (plural_forms < 1) rcd_file->Error("Zero plural forms");

			int lang_idx = GetLanguageIndex(lang_buffer);
			/* Read translation text. */
			if (lang_idx >= 0) {
				const char *split = buffer + used_size;
				ReadUtf8Text(rcd_file, text_length, buffer, lengthof(buffer), plural_forms - 1, &used_size);
				/* Split the string into plural forms. */
				std::vector<const char*> &vector = strings[used_strings].languages[lang_idx];
				vector.resize(plural_forms, nullptr);
				vector.at(0) = split;
				for (int i = 1; i < plural_forms; ++i) {
					int substring_length = strlen(split);
					split += substring_length + 1;
					vector.at(i) = split;
				}
			} else {
				/* Illegal language, read into a dummy buffer. */
				used = 0;
				ReadUtf8Text(rcd_file, text_length, lang_buffer, lengthof(lang_buffer), plural_forms - 1, &used);
			}
			length -= text_length;

			trs_length -= 4 + lang_length + text_length;
		}
		assert(trs_length == 0);
		used_strings++;
	}
	assert (length == 0);

	this->strings.reset(new TextString[used_strings]);
	this->string_count = used_strings;
	this->text_data.reset(new char[used_size]);
	if (this->strings == nullptr || this->text_data == nullptr) rcd_file->Error("Out of heap memory");

	memcpy(this->text_data.get(), buffer, used_size);
	for (uint i = 0; i < used_strings; i++) {
		this->strings[i].name = (strings[i].name == nullptr) ? nullptr : (this->text_data.get() + (strings[i].name - buffer));
		for (uint lng = 0; lng < LANGUAGE_COUNT; lng++) {
			this->strings[i].languages[lng].resize(strings[i].languages[lng].size());
			for (uint s = 0; s < strings[i].languages[lng].size(); ++s) {
				this->strings[i].languages[lng].at(s) = (strings[i].languages[lng].at(s) == nullptr) ?
						nullptr : (this->text_data.get() + (strings[i].languages[lng].at(s) - buffer));
			}
		}
	}
}

/**
 * Get a sprite reference from the \a rcd_file, retrieve the corresponding sprite, and put it in the \a spr destination.
 * @param rcd_file File to load from.
 * @param sprites Sprites already loaded from this file.
 * @param [out] spr Pointer to write the loaded sprite to.
 */
void LoadSpriteFromFile(RcdFileReader *rcd_file, const ImageMap &sprites, ImageData **spr)
{
	uint32 val = rcd_file->GetUInt32();
	if (val == 0) {
		*spr = nullptr;
		return;
	}
	const auto iter = sprites.find(val);
	if (iter == sprites.end()) rcd_file->Error("Sprite block reference not found");
	*spr = iter->second;
}

/**
 * Get a text reference from the \a rcd_file, retrieve the corresponding text data, and put it in the \a txt destination.
 * @param rcd_file File to load from.
 * @param texts Texts already loaded from this file.
 * @param [out] txt Pointer to write the loaded text data reference to.
 */
void LoadTextFromFile(RcdFileReader *rcd_file, const TextMap &texts, TextData **txt)
{
	uint32 val = rcd_file->GetUInt32();
	if (val == 0) {
		*txt = nullptr;
		return;
	}
	const auto iter = texts.find(val);
	if (iter == texts.end()) rcd_file->Error("Text block reference not found");
	*txt = iter->second;
}

SurfaceData::SurfaceData()
{
	for (uint i = 0; i < lengthof(this->surface); i++) this->surface[i] = nullptr;
}

/**
 * Test whether the surface is complete (has all sprites).
 * @return Whether the surface is complete.
 */
bool SurfaceData::HasAllSprites() const
{
	for (uint i = 0; i < lengthof(this->surface); i++) {
		if (this->surface[i] == nullptr) return false;
	}
	return true;
}

/**
 * Load a surface game block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 */
void SpriteManager::LoadSURF(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(6);
	rcd_file->CheckExactLength(rcd_file->size, 2 + 2 + 2 + 4 * NUM_SLOPE_SPRITES, "header");

	uint16 gt = rcd_file->GetUInt16(); // Ground type bytes.
	uint8 type = GTP_INVALID;
	if (gt == 16) type = GTP_GRASS0;
	if (gt == 17) type = GTP_GRASS1;
	if (gt == 18) type = GTP_GRASS2;
	if (gt == 19) type = GTP_GRASS3;
	if (gt == 20) type = GTP_UNDERGROUND;
	if (gt == 32) type = GTP_DESERT;
	if (gt == 48) type = GTP_CURSOR_TEST;
	if (gt == 49) type = GTP_CURSOR_EDGE_TEST;
	if (type == GTP_INVALID) rcd_file->Error("Invalid ground type");

	uint16 width  = rcd_file->GetUInt16();
	rcd_file->GetUInt16(); /// \todo Remove height from RCD block.

	SpriteStorage *ss = this->GetSpriteStore(width);
	if (ss == nullptr || type >= GTP_COUNT) rcd_file->Error("Sprite storage not found");
	SurfaceData *sd = &ss->surface[type];
	for (uint sprnum = 0; sprnum < NUM_SLOPE_SPRITES; sprnum++) {
		LoadSpriteFromFile(rcd_file, sprites, &sd->surface[sprnum]);
	}
}

/**
 * Load a tile selection block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 */
void SpriteManager::LoadTSEL(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(2);
	rcd_file->CheckExactLength(rcd_file->size, 2 + 2 + 4 * NUM_SLOPE_SPRITES, "header");

	uint16 width  = rcd_file->GetUInt16();
	rcd_file->GetUInt16(); /// \todo Remove height from RCD block.

	SpriteStorage *ss = this->GetSpriteStore(width);
	if (ss == nullptr) rcd_file->Error("Sprite storage not found");
	SurfaceData *ts = &ss->tile_select;
	for (uint sprnum = 0; sprnum < NUM_SLOPE_SPRITES; sprnum++) {
		LoadSpriteFromFile(rcd_file, sprites, &ts->surface[sprnum]);
	}
}

Fence::Fence() : type(FENCE_TYPE_INVALID), width(0)
{
	for (uint i = 0; i < lengthof(this->sprites); i++) this->sprites[i] = nullptr;
}

Fence::~Fence()
= default;

FrameSet::FrameSet() : width_x(0), width_y(0), scales(0)
{
}

/**
 * Load a frame set block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 */
void FrameSet::Load(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(2);
	rcd_file->CheckMinLength(rcd_file->size, 3, "header");

	this->scales = rcd_file->GetUInt8();
	this->width_x = rcd_file->GetUInt8();
	this->width_y = rcd_file->GetUInt8();
	if (this->scales < 1) rcd_file->Error("No scales");
	if (this->scales > 10) rcd_file->Error("Too many scales");  // Arbitrary sanity limit.
	if (this->width_x < 1 || this->width_y < 1) rcd_file->Error("Dimension is zero");
	if (this->width_x > 80 || this->width_y > 80) rcd_file->Error("Dimension too big");  // Arbitrary sanity limit.
	rcd_file->CheckExactLength(rcd_file->size, 3 + 16 * this->width_x * this->width_y * this->scales + 2 * this->scales, "frame");

	this->width.reset(new uint16[this->scales]);
	for (int i = 0; i < this->scales; ++i) this->width[i] = rcd_file->GetUInt16();

	for (int i = 0; i < 4; ++i) {
		this->sprites[i].reset(new ImageData*[this->width_x * this->width_y * this->scales]);
		for (int x = 0; x < this->width_x; ++x) {
			for (int y = 0; y < this->width_y; ++y) {
				for (int z = 0; z < this->scales; ++z) {
					ImageData *view;
					LoadSpriteFromFile(rcd_file, sprites, &view);
					this->sprites[i][x * this->width_y * this->scales + y * this->scales + z] = view;
				}
			}
		}
	}
}

/**
 * Get a sprite from the frame set.
 * @param x Relative X coordinate in the object.
 * @param y Relative Y coordinate in the object.
 * @param orientation View orientation.
 * @param zoom Zoom scale index of the image. If the frame set does not contain images at this scale, a correspnidng image from another scale will be resized to create it.
 * @return The image.
 */
ImageData* FrameSet::GetSprite(uint16 x, uint16 y, uint8 orientation, int zoom) const
{
	assert(x < this->width_x && y < this->width_y);
	const int desired_tile_w = TileWidth(zoom);

	int smallest_bigger_index = -1;
	int biggest_smaller_index = -1;

	for (int z = 0; z < this->scales; ++z) {
		if (this->width[z] == desired_tile_w) {
			return this->sprites[orientation][x * this->width_y * this->scales + y * this->scales + z];
		} else if (this->width[z] > desired_tile_w) {
			if (smallest_bigger_index < 0 || this->width[z] < this->width[smallest_bigger_index]) smallest_bigger_index = z;
		} else {
			if (biggest_smaller_index < 0 || this->width[z] > this->width[biggest_smaller_index]) biggest_smaller_index = z;
		}
	}

	/* No match. Downscale an image if possible; otherwise upscale one. */
	int index = smallest_bigger_index >= 0 ? smallest_bigger_index : biggest_smaller_index;
	assert(index >= 0);
	ImageData *img_to_scale = this->sprites[orientation][x * this->width_y * this->scales + y * this->scales + index];
	assert(img_to_scale != nullptr);
	return img_to_scale->Scale(static_cast<float>(desired_tile_w) / this->width[index]);
}

TimedAnimation::TimedAnimation() : frames(0)
{
}

TimedAnimation::~TimedAnimation()
= default;

/**
 * How long this animation needs to play once.
 * @return The duration in milliseconds.
 */
int TimedAnimation::GetTotalDuration() const {
	int total = 0;
	for (int i = 0; i < frames; ++i) total += durations[i];
	return total;
}

/**
 * The frame to display at the given time.
 * @param time The time in milliseconds relative to the animation's begin.
 * @param loop_around Whether the animation should play in an endless loop.
 * @return The current frame, or -1 if the result is invalid.
 */
int TimedAnimation::GetFrame(int time, const bool loop_around) const {
	const int total_length = GetTotalDuration();
	if (total_length <= 0 || (!loop_around && time > total_length)) return -1;
	if (loop_around) time %= total_length;
	for (int i = 0; i < frames; ++i) {
		time -= durations[i];
		if (time <= 0) return i;
	}
	NOT_REACHED();
}

/**
 * Load a frame set block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 */
void TimedAnimation::Load(RcdFileReader *rcd_file, [[maybe_unused]] const ImageMap &sprites)
{
	rcd_file->CheckVersion(1);
	rcd_file->CheckMinLength(rcd_file->size, 4, "header");

	this->frames = rcd_file->GetUInt32();
	rcd_file->CheckExactLength(rcd_file->size, 4 + 8 * this->frames, "timed animation");
	this->durations.reset(new int[this->frames]);
	this->views.reset(new const FrameSet*[this->frames]);
	for (int f = 0; f < this->frames; ++f) this->durations[f] = rcd_file->GetUInt32();
	for (int f = 0; f < this->frames; ++f) {
		this->views[f] = _sprite_manager.GetFrameSet(ImageSetKey(rcd_file->filename, rcd_file->GetUInt32()));
	}
}

/**
 * Load a fence sprites block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 */
void Fence::Load(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(2);
	rcd_file->CheckExactLength(rcd_file->size, 2 + 2 + 4 * FENCE_COUNT, "header");

	this->width  = rcd_file->GetUInt16();
	this->type = rcd_file->GetUInt16();
	if (this->type >= FENCE_TYPE_COUNT) rcd_file->Error("Unknown fence type");

	for (uint sprnum = 0; sprnum < FENCE_COUNT; sprnum++) {
		LoadSpriteFromFile(rcd_file, sprites, &this->sprites[sprnum]);
	}
}

Path::Path() : status(PAS_UNUSED)
{
	for (uint i = 0; i < lengthof(this->sprites); i++) this->sprites[i] = nullptr;
}

/**
 * Load a path sprites block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 */
void SpriteManager::LoadPATH(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(3);
	rcd_file->CheckExactLength(rcd_file->size, 2 + 2 + 2 + 4 * PATH_COUNT, "header");

	uint16 type = rcd_file->GetUInt16();
	PathType pt = PAT_INVALID;
	switch (type & 0x7FFF) {
		case  4: pt = PAT_WOOD; break;
		case  8: pt = PAT_TILED; break;
		case 12: pt = PAT_ASPHALT; break;
		case 16: pt = PAT_CONCRETE; break;
		default: rcd_file->Error("Invalid path type");
	}

	uint16 width = rcd_file->GetUInt16();
	rcd_file->GetUInt16(); /// \todo Remove height from RCD block.

	SpriteStorage *ss = this->GetSpriteStore(width);
	if (ss == nullptr) rcd_file->Error("Sprite storage not found");
	Path *path = &ss->path_sprites[pt];
	for (uint sprnum = 0; sprnum < PATH_COUNT; sprnum++) {
		LoadSpriteFromFile(rcd_file, sprites, &path->sprites[sprnum]);
	}
	path->status = ((type & 0x8000) != 0) ? PAS_QUEUE_PATH : PAS_NORMAL_PATH;
}

PathDecoration::PathDecoration()
{
	/* Initialize sprite pointers. */
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		this->litterbin[edge] = nullptr;
		this->overflow_bin[edge] = nullptr;
		this->demolished_bin[edge] = nullptr;
		this->lamp_post[edge] = nullptr;
		this->demolished_lamp[edge] = nullptr;
		this->bench[edge] = nullptr;
		this->demolished_bench[edge] = nullptr;

		for (int tp = 0; tp < 4; tp++) {
			this->ramp_litter[edge][tp] = nullptr;
			this->ramp_vomit[edge][tp] = nullptr;
		}
	}

	for (int tp = 0; tp < 4; tp++) {
		this->flat_litter[tp] = nullptr;
		this->flat_vomit[tp] = nullptr;
	}

	/* Initialize counts. */
	this->flat_litter_count = 0;
	this->flat_vomit_count = 0;
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		this->ramp_litter_count[edge] = 0;
		this->ramp_vomit_count[edge] = 0;
	}
}

/**
 * Load a path decoration sprites block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 */
void SpriteManager::LoadPDEC(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(1);
	/* Size is 2 byte tile width, 7 groups of sprites at the edges, 2 kinds of (flat+4 ramp) 4 type sprites. */
	rcd_file->CheckExactLength(rcd_file->size, 2 + 7*4*4 + 2*(1+4)*4*4, "header");

	uint16 width = rcd_file->GetUInt16();
	SpriteStorage *ss = this->GetSpriteStore(width);
	if (ss == nullptr) rcd_file->Error("Sprite storage not found");
	PathDecoration *pdec = &ss->path_decoration;

	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		LoadSpriteFromFile(rcd_file, sprites, &pdec->litterbin[edge]);
	}
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		LoadSpriteFromFile(rcd_file, sprites, &pdec->overflow_bin[edge]);
	}
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		LoadSpriteFromFile(rcd_file, sprites, &pdec->demolished_bin[edge]);
	}
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		LoadSpriteFromFile(rcd_file, sprites, &pdec->lamp_post[edge]);
	}
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		LoadSpriteFromFile(rcd_file, sprites, &pdec->demolished_lamp[edge]);
	}
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		LoadSpriteFromFile(rcd_file, sprites, &pdec->bench[edge]);
	}
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		LoadSpriteFromFile(rcd_file, sprites, &pdec->demolished_bench[edge]);
	}

	for (int tp = 0; tp < 4; tp++) {
		LoadSpriteFromFile(rcd_file, sprites, &pdec->flat_litter[tp]);
	}
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		for (int tp = 0; tp < 4; tp++) {
			LoadSpriteFromFile(rcd_file, sprites, &pdec->ramp_litter[edge][tp]);
		}
	}

	for (int tp = 0; tp < 4; tp++) {
		LoadSpriteFromFile(rcd_file, sprites, &pdec->flat_vomit[tp]);
	}
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		for (int tp = 0; tp < 4; tp++) {
			LoadSpriteFromFile(rcd_file, sprites, &pdec->ramp_vomit[edge][tp]);
		}
	}

	/* Data loaded, setup the counts. */
	for (pdec->flat_litter_count = 0; pdec->flat_litter_count < 4; pdec->flat_litter_count++) if (pdec->flat_litter[pdec->flat_litter_count] == nullptr) break;
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		int count = 0;
		while (count < 4 && pdec->ramp_litter[edge][count] != nullptr) count++;
		pdec->ramp_litter_count[edge] = count;
	}

	for (pdec->flat_vomit_count = 0; pdec->flat_vomit_count < 4; pdec->flat_vomit_count++) if (pdec->flat_vomit[pdec->flat_vomit_count] == nullptr) break;
	for (TileEdge edge = EDGE_BEGIN; edge < EDGE_COUNT; edge++) {
		int count = 0;
		while (count < 4 && pdec->ramp_vomit[edge][count] != nullptr) count++;
		pdec->ramp_vomit_count[edge] = count;
	}
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
 */
void SpriteManager::LoadTCOR(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(2);
	rcd_file->CheckExactLength(rcd_file->size, 2 + 2 + 4 * VOR_NUM_ORIENT * NUM_SLOPE_SPRITES, "header");

	uint16 width  = rcd_file->GetUInt16();
	rcd_file->GetUInt16(); /// \todo Remove height from RCD block.

	SpriteStorage *ss = this->GetSpriteStore(width);
	if (ss == nullptr) rcd_file->Error("Sprite storage not found");
	TileCorners *tc = &ss->tile_corners;
	for (uint v = 0; v < VOR_NUM_ORIENT; v++) {
		for (uint sprnum = 0; sprnum < NUM_SLOPE_SPRITES; sprnum++) {
			LoadSpriteFromFile(rcd_file, sprites, &tc->sprites[v][sprnum]);
		}
	}
}

Foundation::Foundation()
{
	for (uint i = 0; i < lengthof(this->sprites); i++) this->sprites[i] = nullptr;
}

/**
 * Load a path sprites block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 */
void SpriteManager::LoadFUND(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(1);
	rcd_file->CheckExactLength(rcd_file->size, 2 + 2 + 2 + 4 * 6, "header");

	uint16 tp = rcd_file->GetUInt16();
	uint16 type = FDT_INVALID;
	if (tp == 16) type = FDT_GROUND;
	if (tp == 32) type = FDT_WOOD;
	if (tp == 48) type = FDT_BRICK;
	if (type == FDT_INVALID) rcd_file->Error("Invalid foundation type");

	uint16 width  = rcd_file->GetUInt16();
	rcd_file->GetUInt16(); /// \todo Remove height from RCD block.

	SpriteStorage *ss = this->GetSpriteStore(width);
	if (ss == nullptr || type >= FDT_COUNT) rcd_file->Error("Sprite storage not found");
	Foundation *fn = &ss->foundation[type];
	for (uint sprnum = 0; sprnum < lengthof(fn->sprites); sprnum++) {
		LoadSpriteFromFile(rcd_file, sprites, &fn->sprites[sprnum]);
	}
}

Platform::Platform()
{
	this->flat[0] = nullptr; this->flat[1] = nullptr;
	for (uint i = 0; i < lengthof(this->ramp); i++) this->ramp[i] = nullptr;
}

/**
 * Load a platform sprites block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 */
void SpriteManager::LoadPLAT(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(2);
	rcd_file->CheckExactLength(rcd_file->size, 2 + 2 + 2 + 2 * 4 + 12 * 4, "header");

	uint16 width  = rcd_file->GetUInt16();
	rcd_file->GetUInt16(); /// \todo Remove height from RCD block.
	uint16 type = rcd_file->GetUInt16();
	if (type != 16) rcd_file->Error("Invalid platform type"); // Only accept type 16 'wood'.

	SpriteStorage *ss = this->GetSpriteStore(width);
	if (ss == nullptr) rcd_file->Error("Sprite storage not found");
	Platform *plat = &ss->platform;
	LoadSpriteFromFile(rcd_file, sprites, &plat->flat[0]);
	LoadSpriteFromFile(rcd_file, sprites, &plat->flat[1]);
	for (uint sprnum = 0; sprnum < lengthof(plat->ramp); sprnum++) {
		LoadSpriteFromFile(rcd_file, sprites, &plat->ramp[sprnum]);
	}
	for (uint sprnum = 0; sprnum < lengthof(plat->right_ramp); sprnum++) {
		LoadSpriteFromFile(rcd_file, sprites, &plat->right_ramp[sprnum]);
	}
	for (uint sprnum = 0; sprnum < lengthof(plat->left_ramp); sprnum++) {
		LoadSpriteFromFile(rcd_file, sprites, &plat->left_ramp[sprnum]);
	}
}

Support::Support()
{
	for (uint sprnum = 0; sprnum < lengthof(this->sprites); sprnum++) this->sprites[sprnum] = nullptr;
}

/**
 * Load a support sprites block from a RCD file.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 */
void SpriteManager::LoadSUPP(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(1);
	rcd_file->CheckExactLength(rcd_file->size, 2 + 2 + 2 + SSP_COUNT * 4, "header");

	uint16 type = rcd_file->GetUInt16();
	if (type != 16) rcd_file->Error("Invalid support type");  // Only accept type 16 'wood'.
	uint16 width  = rcd_file->GetUInt16();
	rcd_file->GetUInt16(); /// \todo Remove height from RCD block.

	SpriteStorage *ss = this->GetSpriteStore(width);
	if (ss == nullptr) rcd_file->Error("Sprite storage not found");
	Support *supp = &ss->support;
	for (uint sprnum = 0; sprnum < lengthof(supp->sprites); sprnum++) {
		LoadSpriteFromFile(rcd_file, sprites, &supp->sprites[sprnum]);
	}
}

DisplayedObject::DisplayedObject()
{
	for (uint i = 0; i < lengthof(this->sprites); i++) this->sprites[i] = nullptr;
}

/**
 * Load a displayed object block.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 */
void SpriteManager::LoadBDIR(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(1);
	rcd_file->CheckExactLength(rcd_file->size, 2 + 4 * 4, "header");

	uint16 width = rcd_file->GetUInt16();

	SpriteStorage *ss = this->GetSpriteStore(width);
	if (ss == nullptr) rcd_file->Error("Sprite storage not found");
	DisplayedObject *dob = &ss->build_arrows;
	for (uint sprnum = 0; sprnum < lengthof(dob->sprites); sprnum++) {
		LoadSpriteFromFile(rcd_file, sprites, &dob->sprites[sprnum]);
	}
}

/** %Animation default constructor. */
Animation::Animation() :
	frame_count(0),
	person_type(PERSON_INVALID),
	anim_type(ANIM_INVALID),
	frames(nullptr)
{
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
		case  8:
		case 16: return PERSON_GUEST;
		case 17: return PERSON_HANDYMAN;
		case 18: return PERSON_MECHANIC;
		case 19: return PERSON_GUARD;
		case 20: return PERSON_ENTERTAINER;
		default: return PERSON_INVALID;
	}
}

/**
 * Load an animation.
 * @param rcd_file RCD file used for loading.
 */
void Animation::Load(RcdFileReader *rcd_file)
{
	rcd_file->CheckVersion(4);
	const uint BASE_LENGTH = 1 + 2 + 2;

	size_t length = rcd_file->size;
	rcd_file->CheckMinLength(length, BASE_LENGTH, "header");
	this->person_type = DecodePersonType(rcd_file->GetUInt8());
	if (this->person_type == PERSON_INVALID) rcd_file->Error("Invalid person type");

	uint16 at = rcd_file->GetUInt16();
	if (at < ANIM_BEGIN || at > ANIM_LAST) rcd_file->Error("Invalid anim type");
	this->anim_type = (AnimationType)at;

	this->frame_count = rcd_file->GetUInt16();
	rcd_file->CheckExactLength(length, BASE_LENGTH + this->frame_count * 6, "frames");
	this->frames.reset(new AnimationFrame[this->frame_count]);
	if (this->frames == nullptr || this->frame_count == 0) rcd_file->Error("Zero frames");

	for (uint i = 0; i < this->frame_count; i++) {
		AnimationFrame *frame = this->frames.get() + i;

		frame->duration = rcd_file->GetUInt16();
		if (frame->duration == 0 || frame->duration >= 5000) rcd_file->Error("Invalid duration");  // Arbitrary sanity limit.

		frame->dx = rcd_file->GetInt16();
		if (frame->dx < -100 || frame->dx > 100) rcd_file->Error("Invalid X step");  // Arbitrary sanity limit.

		frame->dy = rcd_file->GetInt16();
		if (frame->dy < -100 || frame->dy > 100) rcd_file->Error("Invalid Y step");  // Arbitrary sanity limit.
	}
}

/** Animation sprites default constructor. */
AnimationSprites::AnimationSprites() :
	width(0),
	person_type(PERSON_INVALID),
	anim_type(ANIM_INVALID),
	frame_count(0),
	sprites(nullptr)
{
}

/**
 * Load the sprites of an animation.
 * @param rcd_file RCD file used for loading.
 * @param sprites Map of already loaded sprites.
 */
void AnimationSprites::Load(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(3);
	const uint BASE_LENGTH = 2 + 1 + 2 + 2;

	size_t length = rcd_file->size;
	rcd_file->CheckMinLength(length, BASE_LENGTH, "header");
	this->width = rcd_file->GetUInt16();

	this->person_type = DecodePersonType(rcd_file->GetUInt8());
	if (this->person_type == PERSON_INVALID) rcd_file->Error("Invalid person type");

	uint16 at = rcd_file->GetUInt16();
	if (at < ANIM_BEGIN || at > ANIM_LAST) rcd_file->Error("Invalid animation type");
	this->anim_type = (AnimationType)at;

	this->frame_count = rcd_file->GetUInt16();
	rcd_file->CheckExactLength(length, BASE_LENGTH + this->frame_count * 4, "frames");
	this->sprites.reset(new ImageData *[this->frame_count]);
	if (this->sprites == nullptr || this->frame_count == 0) rcd_file->Error("Zero frames");

	for (uint i = 0; i < this->frame_count; i++) {
		LoadSpriteFromFile(rcd_file, sprites, &this->sprites[i]);
	}
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
 */
void GuiSprites::LoadGBOR(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(2);
	rcd_file->CheckExactLength(rcd_file->size, 2 + 8 * 1 + WBS_COUNT * 4, "header");

	/* Select sprites to save to. */
	uint16 tp = rcd_file->GetUInt16(); // Widget type.
	BorderSpriteData *sprdata = nullptr;
	bool pressed = false;
	switch (tp) {
		case 1: sprdata = &this->left_tabbar;  pressed = false; break;
		case 2: sprdata = &this->tab_tabbar;   pressed = true;  break;
		case 3: sprdata = &this->tab_tabbar;   pressed = false; break;
		case 4: sprdata = &this->right_tabbar; pressed = false; break;
		case 5: sprdata = &this->tabbar_panel; pressed = false; break;
		case 6: sprdata = &this->titlebar;     pressed = false; break;
		case 7: sprdata = &this->button;       pressed = false; break;
		case 8: sprdata = &this->button;       pressed = true;  break;
		case 9: sprdata = &this->panel;        pressed = false; break;
		default:
			rcd_file->Error("Invalid widget type");
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
			LoadSpriteFromFile(rcd_file, sprites, &sprdata->pressed[sprnum]);
		} else {
			LoadSpriteFromFile(rcd_file, sprites, &sprdata->normal[sprnum]);
		}
	}
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
 * @todo Load width and height from the RCD file too.
 */
void GuiSprites::LoadGCHK(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(1);
	rcd_file->CheckExactLength(rcd_file->size, 2 + WCS_COUNT * 4, "header");

	/* Select sprites to save to. */
	uint16 tp = rcd_file->GetUInt16(); // Widget type.
	CheckableWidgetSpriteData *sprdata = nullptr;
	switch (tp) {
		case 96:  sprdata = &this->checkbox; break;
		case 112: sprdata = &this->radio_button; break;
		default:
			rcd_file->Error("Invalid widget type");
	}

	sprdata->width = 0;
	sprdata->height = 0;
	for (uint sprnum = 0; sprnum < WCS_COUNT; sprnum++) {
		ImageData *spr;
		LoadSpriteFromFile(rcd_file, sprites, &spr);
		sprdata->sprites[sprnum] = spr;

		if (spr != nullptr) {
			sprdata->width  = std::max(sprdata->width, spr->width);
			sprdata->height = std::max(sprdata->height, spr->height);
		}
	}
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
 * @todo Move widget_type further to the top in the RCD file block.
 */
void GuiSprites::LoadGSLI(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(1);
	rcd_file->CheckExactLength(rcd_file->size, 3 * 1 + 2 + WSS_COUNT * 4, "header");

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
			rcd_file->Error("Invalid widget type");
	}

	sprdata->min_bar_length = min_length;
	sprdata->stepsize = stepsize;
	sprdata->height = height;

	for (uint sprnum = 0; sprnum < WSS_COUNT; sprnum++) {
		if (shaded) {
			LoadSpriteFromFile(rcd_file, sprites, &sprdata->shaded[sprnum]);
		} else {
			LoadSpriteFromFile(rcd_file, sprites, &sprdata->normal[sprnum]);
		}
	}
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
 * @todo Move widget_type further to the top in the RCD file block.
 * @todo Add width of the scrollbar in the RCD file block.
 */
void GuiSprites::LoadGSCL(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(1);
	rcd_file->CheckExactLength(rcd_file->size, 4 * 1 + 2 + WLS_COUNT * 4, "header");

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
			rcd_file->Error("Invalid widget type");
	}

	sprdata->min_length_all = min_length_bar;
	sprdata->stepsize_bar = stepsize_back;
	sprdata->min_length_slider = min_slider;
	sprdata->stepsize_slider = stepsize_slider;

	uint16 max_width = 0;
	uint16 max_height = 0;
	for (uint sprnum = 0; sprnum < WLS_COUNT; sprnum++) {
		ImageData *spr;
		LoadSpriteFromFile(rcd_file, sprites, &spr);

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
}

/**
 * Load GUI slope selection sprites.
 * @param rcd_file RCD file being loaded.
 * @param sprites Sprites loaded from this file.
 * @param texts Texts loaded from this file.
 */
void GuiSprites::LoadGSLP(RcdFileReader *rcd_file, const ImageMap &sprites, const TextMap &texts)
{
	rcd_file->CheckVersion(14);
	const uint8 indices[] = {TSL_STRAIGHT_DOWN, TSL_STEEP_DOWN, TSL_DOWN, TSL_FLAT, TSL_UP, TSL_STEEP_UP, TSL_STRAIGHT_UP};

	/* 'indices' entries of slope sprites, bends, banking, 4 triangle arrows,
	 * 4 entries with rotation sprites, 2 button sprites, one entry with a text block.
	 */
	rcd_file->CheckExactLength(rcd_file->size,
			(lengthof(indices) + TBN_COUNT + TPB_COUNT + 4 + 2 + 2 + 1 + TC_END + 1 + WTP_COUNT +
			4 + 3 + 4 + 2 + 1 + 5 + 3 + lengthof(this->toolbar_images) + 5) * 4,
			"header");

	for (uint i = 0; i < lengthof(indices); i++) {
		LoadSpriteFromFile(rcd_file, sprites, &this->slope_select[indices[i]]);
	}
	for (uint i = 0; i < TBN_COUNT; i++) {
		LoadSpriteFromFile(rcd_file, sprites, &this->bend_select[i]);
	}
	for (uint i = 0; i < TPB_COUNT; i++) {
		LoadSpriteFromFile(rcd_file, sprites, &this->bank_select[i]);
	}
	LoadSpriteFromFile(rcd_file, sprites, &this->triangle_left);
	LoadSpriteFromFile(rcd_file, sprites, &this->triangle_right);
	LoadSpriteFromFile(rcd_file, sprites, &this->triangle_up);
	LoadSpriteFromFile(rcd_file, sprites, &this->triangle_down);
	for (uint i = 0; i < 2; i++) {
		LoadSpriteFromFile(rcd_file, sprites, &this->platform_select[i]);
	}
	for (uint i = 0; i < 2; i++) {
		LoadSpriteFromFile(rcd_file, sprites, &this->power_select[i]);
	}

	LoadSpriteFromFile(rcd_file, sprites, &this->disabled);

	for (uint i = 0; i < TC_END; i++) {
		LoadSpriteFromFile(rcd_file, sprites, &this->compass[i]);
	}
	LoadSpriteFromFile(rcd_file, sprites, &this->bulldozer);
	for (uint i = 0; i < WTP_COUNT; i++) {
		LoadSpriteFromFile(rcd_file, sprites, &this->weather[i]);
	}
	for (uint i = 0; i < 4; i++) {
		LoadSpriteFromFile(rcd_file, sprites, &this->lights_rog[i]);
	}
	for (uint i = 0; i < 3; i++) {
		LoadSpriteFromFile(rcd_file, sprites, &this->lights_rg[i]);
	}

	LoadSpriteFromFile(rcd_file, sprites, &this->rot_2d_pos);
	LoadSpriteFromFile(rcd_file, sprites, &this->rot_2d_neg);
	LoadSpriteFromFile(rcd_file, sprites, &this->rot_3d_pos);
	LoadSpriteFromFile(rcd_file, sprites, &this->rot_3d_neg);

	LoadSpriteFromFile(rcd_file, sprites, &this->close_sprite);
	LoadSpriteFromFile(rcd_file, sprites, &this->dot_sprite);

	LoadSpriteFromFile(rcd_file, sprites, &this->message_goto);
	LoadSpriteFromFile(rcd_file, sprites, &this->message_park);
	LoadSpriteFromFile(rcd_file, sprites, &this->message_guest);
	LoadSpriteFromFile(rcd_file, sprites, &this->message_ride);
	LoadSpriteFromFile(rcd_file, sprites, &this->message_ride_type);

	LoadSpriteFromFile(rcd_file, sprites, &this->loadsave_err);
	LoadSpriteFromFile(rcd_file, sprites, &this->loadsave_warn);
	LoadSpriteFromFile(rcd_file, sprites, &this->loadsave_ok);

	for (ImageData *& t : this->toolbar_images) LoadSpriteFromFile(rcd_file, sprites, &t);

	LoadSpriteFromFile(rcd_file, sprites, &this->speed_0);
	LoadSpriteFromFile(rcd_file, sprites, &this->speed_1);
	LoadSpriteFromFile(rcd_file, sprites, &this->speed_2);
	LoadSpriteFromFile(rcd_file, sprites, &this->speed_4);
	LoadSpriteFromFile(rcd_file, sprites, &this->speed_8);

	LoadTextFromFile(rcd_file, texts, &this->text);
	_language.RegisterStrings(*this->text, _gui_strings_table, STR_GUI_START);
}

/**
 * Load main menu sprites.
 * @param rcd_file RCD file being loaded.
 * @param sprites Sprites loaded from this file.
 */
void GuiSprites::LoadMENU(RcdFileReader *rcd_file, const ImageMap &sprites)
{
	rcd_file->CheckVersion(1);
	rcd_file->CheckExactLength(rcd_file->size, 40 - 12, "header");

	this->mainmenu_splash_duration = rcd_file->GetUInt32();
	LoadSpriteFromFile(rcd_file, sprites, &this->mainmenu_logo);
	LoadSpriteFromFile(rcd_file, sprites, &this->mainmenu_splash);
	LoadSpriteFromFile(rcd_file, sprites, &this->mainmenu_new);
	LoadSpriteFromFile(rcd_file, sprites, &this->mainmenu_load);
	LoadSpriteFromFile(rcd_file, sprites, &this->mainmenu_settings);
	LoadSpriteFromFile(rcd_file, sprites, &this->mainmenu_quit);
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
	this->left_tabbar.Clear();
	this->tab_tabbar.Clear();
	this->right_tabbar.Clear();
	this->tabbar_panel.Clear();
	this->panel.Clear();

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
	this->message_goto = nullptr;
	this->message_park = nullptr;
	this->message_ride = nullptr;
	this->message_guest = nullptr;
	this->message_ride_type = nullptr;
	this->loadsave_err = nullptr;
	this->loadsave_warn = nullptr;
	this->loadsave_ok = nullptr;
	this->speed_0 = nullptr;
	this->speed_1 = nullptr;
	this->speed_2 = nullptr;
	this->speed_4 = nullptr;
	this->speed_8 = nullptr;
	for (ImageData *& t : this->toolbar_images) t = nullptr;
	for (uint i = 0; i < TC_END; i++) this->compass[i] = nullptr;
	for (uint i = 0; i < WTP_COUNT; i++) this->weather[i] = nullptr;
	for (uint i = 0; i < 4; i++) this->lights_rog[i] = nullptr;
	for (uint i = 0; i < 3; i++) this->lights_rg[i] = nullptr;
}

/**
 * Have essential GUI sprites been loaded to be used in a display.
 * This is the minimum number of sprites needed to display an error message.
 * @return Sufficient data has been loaded.
 */
bool GuiSprites::HasSufficientGraphics() const
{
	return this->titlebar.IsLoaded() && this->button.IsLoaded()
			&& this->left_tabbar.IsLoaded()  && this->tab_tabbar.IsLoaded()
			&& this->right_tabbar.IsLoaded() && this->tabbar_panel.IsLoaded()
			&& this->panel.IsLoaded()        && this->checkbox.IsLoaded()
			&& this->radio_button.IsLoaded() && this->hor_scroll.IsLoaded()
			&& this->vert_scroll.IsLoaded()  && this->close_sprite != nullptr;
}

/**
 * Storage constructor for a single size.
 * @param _size Width of the tile stored in this object.
 */
SpriteStorage::SpriteStorage(uint16 _size) : size(_size)
{
	for (uint8 i = 0; i < FENCE_TYPE_COUNT; i++) this->fence[i] = nullptr;
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
		} else {
			++iter;
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

/**
 * Add fence sprites.
 * @param fnc New fence sprites.
 * @pre Width of the fence sprites must match with #size.
 */
void SpriteStorage::AddFence(Fence *fnc)
{
	assert(fnc->width == this->size);
	assert(fnc->type < FENCE_TYPE_COUNT);
	this->fence[fnc->type] = fnc;
}

/** Sprite manager constructor. */
SpriteManager::SpriteManager()
{
	_gui_sprites.Clear();

	this->store.reserve(ZOOM_SCALES_COUNT);
	for (int z = 0; z < ZOOM_SCALES_COUNT; ++z) this->store.emplace_back(TileWidth(z));
	this->store.shrink_to_fit();
}

/** Sprite manager destructor. */
SpriteManager::~SpriteManager()
{
	_gui_sprites.Clear();
	/* Sprite stores will be deleted soon as well. */
}

/**
 * Load sprites from the disk.
 * @param filename Name of the RCD file to load.
 * @todo Try to re-use already loaded blocks.
 * @todo Code will use last loaded surface as grass.
 */
void SpriteManager::Load(const char *filename)
{
	RcdFileReader rcd_file(filename);
	if (!rcd_file.CheckFileHeader("RCDF", 2)) throw LoadingError("Bad header");

	ImageMap sprites; // Sprites loaded from this file.
	TextMap  texts;   // Texts loaded from this file.
	TrackPiecesMap track_pieces; // Track pieces loaded from this file.

	/* Load blocks. */
	for (uint blk_num = 1;; blk_num++) {
		if (!rcd_file.ReadBlockHeader()) return; // End reached.

		/* Skip meta blocks. */
		if (strcmp(rcd_file.name, "INFO") == 0) {
			if (!rcd_file.SkipBytes(rcd_file.size)) throw LoadingError("Invalid INFO block.");
			continue;
		}

		if (strcmp(rcd_file.name, "8PXL") == 0 || strcmp(rcd_file.name, "32PX") == 0) {
			ImageData *imd = LoadImage(&rcd_file);
			if (imd == nullptr) {
				throw LoadingError("Image data loading failed.");
			}
			std::pair<uint, ImageData *> p(blk_num, imd);
			sprites.insert(p);
			continue;
		}

		if (strcmp(rcd_file.name, "SURF") == 0) {
			this->LoadSURF(&rcd_file, sprites);
			continue;
		}

		if (strcmp(rcd_file.name, "TSEL") == 0) {
			this->LoadTSEL(&rcd_file, sprites);
			continue;
		}

		if (strcmp(rcd_file.name, "PATH") == 0) {
			this->LoadPATH(&rcd_file, sprites);
			continue;
		}

		if (strcmp(rcd_file.name, "PDEC") == 0) {
			this->LoadPDEC(&rcd_file, sprites);
			continue;
		}

		if (strcmp(rcd_file.name, "TCOR") == 0) {
			this->LoadTCOR(&rcd_file, sprites);
			continue;
		}

		if (strcmp(rcd_file.name, "FENC") == 0) {
			std::unique_ptr<Fence> block(new Fence);
			block->Load(&rcd_file, sprites);
			SpriteStorage *ss = this->GetSpriteStore(block->width);
			if (ss != nullptr) {
				ss->AddFence(block.get());
				this->AddBlock(std::move(block));
			} else {
				printf("WARNING: Not loading fence with tile width %u\n", block->width);
			}
			continue;
		}

		if (strcmp(rcd_file.name, "FUND") == 0) {
			this->LoadFUND(&rcd_file, sprites);
			continue;
		}

		if (strcmp(rcd_file.name, "PLAT") == 0) {
			this->LoadPLAT(&rcd_file, sprites);
			continue;
		}

		if (strcmp(rcd_file.name, "SUPP") == 0) {
			this->LoadSUPP(&rcd_file, sprites);
			continue;
		}

		if (strcmp(rcd_file.name, "BDIR") == 0) {
			this->LoadBDIR(&rcd_file, sprites);
			continue;
		}

		if (strcmp(rcd_file.name, "GCHK") == 0) {
			_gui_sprites.LoadGCHK(&rcd_file, sprites);
			continue;
		}

		if (strcmp(rcd_file.name, "GBOR") == 0) {
			_gui_sprites.LoadGBOR(&rcd_file, sprites);
			continue;
		}

		if (strcmp(rcd_file.name, "GSLI") == 0) {
			_gui_sprites.LoadGSLI(&rcd_file, sprites);
			continue;
		}

		if (strcmp(rcd_file.name, "GSCL") == 0) {
			_gui_sprites.LoadGSCL(&rcd_file, sprites);
			continue;
		}

		if (strcmp(rcd_file.name, "GSLP") == 0) {
			_gui_sprites.LoadGSLP(&rcd_file, sprites, texts);
			continue;
		}

		if (strcmp(rcd_file.name, "MENU") == 0) {
			_gui_sprites.LoadMENU(&rcd_file, sprites);
			continue;
		}

		if (strcmp(rcd_file.name, "ANIM") == 0) {
			std::unique_ptr<Animation> anim(new Animation);
			anim->Load(&rcd_file);
			if (anim->person_type == PERSON_INVALID || anim->anim_type == ANIM_INVALID) {
				throw LoadingError("ANIM: Unknown animation.");
			}
			this->AddAnimation(anim.get());
			// this->store.RemoveAnimations(anim->anim_type, (PersonType)anim->person_type);  // NOCOM needed?
			this->AddBlock(std::move(anim));
			continue;
		}

		if (strcmp(rcd_file.name, "ANSP") == 0) {
			std::unique_ptr<AnimationSprites> an_spr(new AnimationSprites);
			an_spr->Load(&rcd_file, sprites);
			if (an_spr->person_type == PERSON_INVALID || an_spr->anim_type == ANIM_INVALID) {
				throw LoadingError("ANSP: Unknown animation.");
			}
			// this->store.AddAnimationSprites(an_spr.get());  // NOCOM needed?
			this->AddBlock(std::move(an_spr));
			continue;
		}

		if (strcmp(rcd_file.name, "PRSG") == 0) {
			LoadPRSG(&rcd_file);
			continue;
		}

		if (strcmp(rcd_file.name, "TEXT") == 0) {
			std::unique_ptr<TextData> txt(new TextData);
			txt->Load(&rcd_file);
			texts.insert(std::make_pair(blk_num, txt.get()));
			this->AddBlock(std::move(txt));
			continue;
		}

		if (strcmp(rcd_file.name, "SHOP") == 0) {
			std::unique_ptr<ShopType> shop_type(new ShopType);
			shop_type->Load(&rcd_file, sprites, texts);
			_rides_manager.AddRideType(std::move(shop_type));
			continue;
		}

		if (strcmp(rcd_file.name, "FSET") == 0) {
			std::unique_ptr<FrameSet> fset(new FrameSet);
			fset->Load(&rcd_file, sprites);
			this->frame_sets[ImageSetKey(filename, blk_num)] = std::move(fset);
			continue;
		}

		if (strcmp(rcd_file.name, "TIMA") == 0) {
			std::unique_ptr<TimedAnimation> anim(new TimedAnimation);
			anim->Load(&rcd_file, sprites);
			this->timed_animations[ImageSetKey(filename, blk_num)] = std::move(anim);
			continue;
		}

		if (strcmp(rcd_file.name, "SCNY") == 0) {
			std::unique_ptr<SceneryType> s(new SceneryType);
			s->Load(&rcd_file, sprites, texts);
			_scenery.AddSceneryType(s);
			continue;
		}

		if (strcmp(rcd_file.name, "RIEE") == 0) {
			std::unique_ptr<RideEntranceExitType> e(new RideEntranceExitType);
			e->Load(&rcd_file, texts);
			_rides_manager.AddRideEntranceExitType(e);
			continue;
		}

		if (strcmp(rcd_file.name, "FGTR") == 0) {
			std::unique_ptr<GentleThrillRideType> ride_type(new GentleThrillRideType);
			ride_type->Load(&rcd_file, sprites, texts);
			_rides_manager.AddRideType(std::move(ride_type));
			continue;
		}

		if (strcmp(rcd_file.name, "TRCK") == 0) {
			auto tp = std::make_shared<TrackPiece>();
			tp->Load(&rcd_file, sprites);
			track_pieces.insert({blk_num, tp});
			continue;
		}

		if (strcmp(rcd_file.name, "RCST") == 0) {
			std::unique_ptr<CoasterType> ct(new CoasterType);
			ct->Load(&rcd_file, texts, track_pieces);
			_rides_manager.AddRideType(std::move(ct));
			continue;
		}

		if (strcmp(rcd_file.name, "CSPL") == 0) {
			LoadCoasterPlatform(&rcd_file, sprites);
			continue;
		}

		if (strcmp(rcd_file.name, "CARS") == 0) {
			CarType *ct = GetNewCarType();
			if (ct == nullptr) throw LoadingError("No room to store a car type.");
			ct->Load(&rcd_file, sprites);
			continue;
		}

		/* Unknown block in the RCD file. Skip the block. */
		fprintf(stderr, "Unknown RCD block '%s', version %i, ignoring it\n", rcd_file.name, rcd_file.version);
		if (!rcd_file.SkipBytes(rcd_file.size)) throw LoadingError("Error skipping unknown block.");
	}
}

/**
 * Get the sprite storage belonging to a given size of sprites.
 * @param width Tile width of the sprites.
 * @return Sprite storage object for the given width if it exists, else \c nullptr.
 */
SpriteStorage *SpriteManager::GetSpriteStore(int width)
{
	width = GetZoomScaleByWidth(width);
	if (width < 0) return nullptr;
	return &this->store[width];
}

/** Load all useful RCD files found by #_rcd_collection, into the program. */
void SpriteManager::LoadRcdFiles()
{
	for (auto &entry : _rcd_collection.rcdfiles) {
		const char *fname = entry.second.path.c_str();
		try {
			this->Load(fname);
		} catch (const LoadingError &e) {
			fprintf(stderr, "Error while reading \"%s\": %s\n", fname, e.what());
		}
	}
}

/**
 * Add a RCD data block to the list of managed blocks.
 * @param block New block to add.
 * @note Takes ownership of the pointer and clears the passed smart pointer.
 */
inline void SpriteManager::AddBlock(std::unique_ptr<RcdBlock> block)
{
	this->blocks.push_back(std::move(block));
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
 * Get the size of an image including its origin.
 * @param imd Image to inspect.
 * @return Size of the image, including its origin.
 */
Rectangle16 GetSpriteSize(const ImageData *imd)
{
	Rectangle16 rect;

	if (imd != nullptr && imd->width != 0 && imd->height != 0) {
		rect.AddPoint(imd->xoffset, imd->yoffset);
		rect.AddPoint(imd->xoffset + (int16)imd->width - 1, imd->yoffset + (int16)imd->height - 1);
	}
	return rect;
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
		rect.MergeArea(GetSpriteSize(imd));
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
	static Rectangle16 lights;

	if (number >= SPR_GUI_COMPASS_START && number < SPR_GUI_COMPASS_END) {
		if (compasses.width == 0) SetSpriteSize(SPR_GUI_COMPASS_START, SPR_GUI_COMPASS_END, compasses);
		return compasses;
	}
	if (number >= SPR_GUI_WEATHER_START && number < SPR_GUI_WEATHER_END) {
		if (weathers.width == 0) SetSpriteSize(SPR_GUI_WEATHER_START, SPR_GUI_WEATHER_END, weathers);
		return weathers;
	}
	if ((number >= SPR_GUI_ROG_LIGHTS_START && number < SPR_GUI_ROG_LIGHTS_END) || (number >= SPR_GUI_RG_LIGHTS_START && number < SPR_GUI_RG_LIGHTS_END) ) {
		if (lights.width == 0) {
			SetSpriteSize(SPR_GUI_ROG_LIGHTS_START, SPR_GUI_ROG_LIGHTS_END, lights);
			SetSpriteSize(SPR_GUI_RG_LIGHTS_START, SPR_GUI_RG_LIGHTS_END, lights);
		}
		return lights;
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
		result = GetSpriteSize(imd);
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
	if (number >= SPR_GUI_ROG_LIGHTS_START && number < SPR_GUI_ROG_LIGHTS_END) return _gui_sprites.lights_rog[number - SPR_GUI_ROG_LIGHTS_START];
	if (number >= SPR_GUI_RG_LIGHTS_START && number < SPR_GUI_RG_LIGHTS_END)   return _gui_sprites.lights_rg[number - SPR_GUI_RG_LIGHTS_START];
	if (number >= SPR_GUI_SLOPES_START && number < SPR_GUI_SLOPES_END)   return _gui_sprites.slope_select[number - SPR_GUI_SLOPES_START];
	if (number >= SPR_GUI_BEND_START   && number < SPR_GUI_BEND_END) return _gui_sprites.bend_select[number - SPR_GUI_BEND_START];
	if (number >= SPR_GUI_BANK_START   && number < SPR_GUI_BANK_END) return _gui_sprites.bank_select[number - SPR_GUI_BANK_START];
	if (number >= SPR_GUI_TOOLBAR_BEGIN && number < SPR_GUI_TOOLBAR_END) return _gui_sprites.toolbar_images[number - SPR_GUI_TOOLBAR_BEGIN];

	// NOCOM build arrows don't belong here
	if (number >= SPR_GUI_BUILDARROW_START && number < SPR_GUI_BUILDARROW_END) {
		return this->store[DEFAULT_ZOOM /* NOCOM */].GetArrowSprite(number - SPR_GUI_BUILDARROW_START, VOR_NORTH);
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
		case SPR_GUI_MESSAGE_GOTO:       return _gui_sprites.message_goto;
		case SPR_GUI_MESSAGE_PARK:       return _gui_sprites.message_park;
		case SPR_GUI_MESSAGE_GUEST:      return _gui_sprites.message_guest;
		case SPR_GUI_MESSAGE_RIDE:       return _gui_sprites.message_ride;
		case SPR_GUI_MESSAGE_RIDE_TYPE:  return _gui_sprites.message_ride_type;
		case SPR_GUI_LOADSAVE_ERR:       return _gui_sprites.loadsave_err;
		case SPR_GUI_LOADSAVE_WARN:      return _gui_sprites.loadsave_warn;
		case SPR_GUI_LOADSAVE_OK:        return _gui_sprites.loadsave_ok;
		case SPR_GUI_SPEED_0:            return _gui_sprites.speed_0;
		case SPR_GUI_SPEED_1:            return _gui_sprites.speed_1;
		case SPR_GUI_SPEED_2:            return _gui_sprites.speed_2;
		case SPR_GUI_SPEED_4:            return _gui_sprites.speed_4;
		case SPR_GUI_SPEED_8:            return _gui_sprites.speed_8;
		case SPR_GUI_BENCH:              return this->store[DEFAULT_ZOOM].path_decoration.bench    [0];
		case SPR_GUI_BIN:                return this->store[DEFAULT_ZOOM].path_decoration.litterbin[0];
		case SPR_GUI_LAMP:               return this->store[DEFAULT_ZOOM].path_decoration.lamp_post[0];
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

/**
 * Get the fence rcd data for a given fence type
 * @param fence_type The fence type (@see FenceType)
 * @param zoom Zoom scale.
 * @return Fence object or nullptr
 */
const Fence *SpriteManager::GetFence(FenceType fence_type, int zoom) const
{
	assert(fence_type < FENCE_TYPE_COUNT);
	return this->store[zoom].fence[fence_type];
}

/**
 * Get the frame set rcd data at a given frame set index
 * @param key The FSET block's index in the rcd file.
 * @return FrameSet object or nullptr.
 */
const FrameSet *SpriteManager::GetFrameSet(const ImageSetKey &key) const
{
	auto it = this->frame_sets.find(key);
	return it == this->frame_sets.end() ? nullptr : it->second.get();
}

/**
 * Get the timed animation rcd data at a given timed animation index
 * @param key The TIMA block's index in the rcd file.
 * @return TimedAnimation object or nullptr.
 */
const TimedAnimation *SpriteManager::GetTimedAnimation(const ImageSetKey &key) const
{
	auto it = this->timed_animations.find(key);
	return it == this->timed_animations.end() ? nullptr : it->second.get();
}

/**
 * Get the status of a path type.
 * @param path_type %Path type to query.
 * @return Status of the path type.
 */
PathStatus SpriteManager::GetPathStatus(PathType path_type)
{
	// NOCOM static info really doesn't belong in zoom-specific data!
	const Path &path = this->store[DEFAULT_ZOOM].path_sprites[path_type];
	return path.status;
}

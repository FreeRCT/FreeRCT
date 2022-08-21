/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file nodes.cpp Code of the RCD file nodes. */

#include "../stdafx.h"
#include <cmath>
#include "ast.h"
#include "nodes.h"
#include "string_storage.h"
#include "file_writing.h"

/**
 * Get a subnode for the given \a row and \a col.
 * @param row Row of the sub node.
 * @param col Column of the sub node.
 * @param name Variable name to receive the sub node (for error reporting only).
 * @param pos %Position containing the variable name (for error reporting only).
 * @return The requested sub node.
 * @note The default implementation always fails, override to get the feature.
 */
std::shared_ptr<BlockNode> BlockNode::GetSubNode(int row, int col, const char *name, const Position &pos)
{
	/* By default, fail. */
	fprintf(stderr, "Error at %s: Cannot assign sub node (row=%d, column=%d) to variable \"%s\"\n", pos.ToString(), row, col, name);
	exit(1);
}

/**
 * Constructor of a named block.
 * @param blk_name Name of the block (4 letters string).
 * @param version Version number of the block.
 */
NamedBlock::NamedBlock(const char *blk_name, int version) : BlockNode()
{
	this->blk_name = blk_name;
	this->version = version;
}

/**
 * \fn NamedBlock::Write(FileWriter *fw)
 * Write the block to the file.
 * @param fw File to write to.
 * @return Block number of this named block in the file.
 */

/**
 * Constructor of a named game block.
 * @param blk_name Name of the block (4 letters string).
 * @param version Version number of the block.
 */
GameBlock::GameBlock(const char *blk_name, int version) : NamedBlock(blk_name, version)
{
}

/**
 * Constructor of a named meta block.
 * @param blk_name Name of the block (4 letters string).
 * @param version Version number of the block.
 */
MetaBlock::MetaBlock(const char *blk_name, int version) : NamedBlock(blk_name, version)
{
}

/**
 * Constructor of the file node.
 * @param file_name Name of the file to write to.
 */
FileNode::FileNode(const std::string &file_name) : BlockNode()
{
	this->file_name = file_name;
}

/**
 * Output the content to \a fw, for writing it to a file.
 * @param fw Store of file content.
 */
void FileNode::Write(FileWriter *fw)
{
	for (auto iter : this->blocks) {
		iter->Write(fw);
	}
}

/**
 * Write an 8PXL block.
 * @param fw File to write to.
 * @return Block number in the written file for this sprite.
 */
int SpriteBlock::Write(FileWriter *fw)
{
	if (this->sprite_image.data_size == 0) return 0; // Don't make empty sprites.

	FileBlock *fb = new FileBlock;
	int length = 4 * 2 + this->sprite_image.data_size;
	fb->StartSave(this->sprite_image.block_name, this->sprite_image.block_version, length);

	fb->SaveUInt16(this->sprite_image.width);
	fb->SaveUInt16(this->sprite_image.height);
	fb->SaveUInt16(this->sprite_image.xoffset);
	fb->SaveUInt16(this->sprite_image.yoffset);
	fb->SaveBytes(this->sprite_image.data, this->sprite_image.data_size);
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

FilePattern::FilePattern()
{
	this->length = -1;
}

/**
 * Find the next \c '{' character in the provided and return a pointer to it. If not available, return \c nullptr.
 * @param p String to search.
 * @return Pointer to \c '{' or \c nullptr.
 */
static const char *SkipToCurOpen(const char *p)
{
	for (;;) {
		if (*p == '\0') return nullptr;
		if (*p == '{') return p;
		p++;
	}
}

/**
 * Advance the pointer over white space, and the word \c "seq". Return \c nullptr if the word is not available.
 * @param p String to search.
 * @return Pointer into the string, behind the \c "seq" word, or \c nullptr
 */
static const char *SkipSeq(const char *p)
{
	while (*p == ' ') p++;
	if (p[0] == 's' && p[1] == 'e' && p[2] == 'q') return p + 3;
	return nullptr;
}

/**
 * Advance the pointer over white space, and the provided character \a q. Return \c nullptr if the character is not available.
 * @param p String to search.
 * @param q Character to match.
 * @return Pointer into the string, behind the \a q character, or \c nullptr
 */
static const char *SkipChar(const char *p, char q)
{
	while (*p == ' ') p++;
	if (*p == q) return p + 1;
	return nullptr;
}

/**
 * Advance the pointer over white space, and read the number behind it. Return \c nullptr if no number is available.
 * @param p String to search.
 * @param num [out] Value of the read number, if any.
 * @return Pointer into the string, behind the number, or \c nullptr
 */
static const char *SkipNumber(const char *p, int *num)
{
	while (*p == ' ') p++;
	*num = 0;
	bool found_number = false;
	while (*p >= '0' && *p <= '9') {
		*num = *num * 10 + *p - '0';
		found_number = true;
		p++;
	}
	if (!found_number) return nullptr;
	if (*num < 0) return nullptr; // Overflow.
	return p;
}

/**
 * Advance the pointer over white space, and the word \c "..". Return \c nullptr if the word is not available.
 * @param p String to search.
 * @return Pointer into the string, behind the \c ".." word, or \c nullptr
 */
static const char *SkipDotDot(const char *p)
{
	while (*p == ' ') p++;
	if (p[0] == '.' && p[1] == '.') return p + 2;
	return nullptr;
}

/**
 * Store the filename in the file pattern class, and parse the name for the "{seq(first..last,length)}" pattern.
 * @param fname Filename to store.
 */
void FilePattern::SetFilename(const std::string &fname)
{
	this->prefix = fname;
	this->length = 0; // Default, just the filename.

	const char *txt = fname.c_str();
	const char *p = txt;

	p = SkipToCurOpen(p);      if (p == nullptr) return;

	const char *q = p + 1;
	q = SkipSeq(q);            if (q == nullptr) return;
	q = SkipChar(q, '(');      if (q == nullptr) return;

	int fnum;
	q = SkipNumber(q, &fnum);  if (q == nullptr) return;
	q = SkipDotDot(q);         if (q == nullptr) return;
	int flast;
	q = SkipNumber(q, &flast); if (q == nullptr) return;

	q = SkipChar(q, ',');      if (q == nullptr) return;
	int flen;
	q = SkipNumber(q, &flen);  if (q == nullptr) return;
	q = SkipChar(q, ')');      if (q == nullptr) return;
	q = SkipChar(q, '}');      if (q == nullptr) return;

	if (flast < fnum || flen <= 0 || flen > 4) return;

	this->prefix = fname.substr(0, p - txt);
	this->suffix = fname.substr(q - txt);
	this->first = fnum;
	this->last = flast;
	this->length = flen;
}

/**
 * Get the number of filenames represented by the class.
 * @return Number of unique filenames available in the class.
 */
int FilePattern::GetCount() const
{
	if (this->length < 0) return 0;
	if (this->length == 0) return 1;
	return this->last - this->first + 1;
}

/**
 * Construct the \a index-th filename.
 * @param index Index number of the filename to generate from the pattern.
 * @return The filename expressed by 'seq' at the provided index.
 */
std::string FilePattern::MakeFilename(int index) const
{
	char buffer[8];

	assert(this->length >= 0);
	if (this->length == 0) return this->prefix;

	int val = this->first + index;
	snprintf(buffer, lengthof(buffer), "%d", val);
	buffer[lengthof(buffer) - 1] = '\0';

	std::string s = this->prefix;
	for (int i = strlen(buffer); i < this->length; i++) s += "0";
	return s + buffer + this->suffix;
}

/**
 * Constructor of a sprite sheet.
 * @param pos %Position of the sheet node.
 */
SheetBlock::SheetBlock(const Position &pos) : pos(pos)
{
	this->imf = nullptr;
	this->img_sheet = nullptr;
	this->rmf = nullptr;
	this->rim = nullptr;
}

SheetBlock::~SheetBlock()
{
	delete this->imf;
	delete this->img_sheet;
	delete this->rmf;
	delete this->rim;
}

/**
 * Get the sprite sheet. Loads the sheet from the disk on the first call.
 * @return The loaded image.
 */
Image *SheetBlock::GetSheet()
{
	if (this->img_sheet != nullptr) return this->img_sheet;

	this->imf = new ImageFile;
	const char *err = this->imf->LoadFile(this->file);
	if (err != nullptr) {
		fprintf(stderr, "Error at %s, loading of the sheet-image failed: %s\n", this->pos.ToString(), err);
		exit(1);
	}
	BitMaskData *bmd = (this->mask == nullptr) ? nullptr : &this->mask->data;
	if (this->imf->Is8bpp()) {
		this->img_sheet = new Image8bpp(this->imf, bmd);
		if (this->recolour != "") fprintf(stderr, "Error at %s, cannot recolour an 8bpp image, ignoring the file.\n", this->pos.ToString());
	} else {
		Image32bpp *im = new Image32bpp(this->imf, bmd);
		this->img_sheet = im;
		if (this->recolour != "") {
			this->rmf = new ImageFile;
			const char *err = this->rmf->LoadFile(this->recolour);
			if (err != nullptr) {
				fprintf(stderr, "Error at %s, loading of the recolour file failed: %s\n", this->pos.ToString(), err);
				exit(1);
			}
			if (!this->rmf->Is8bpp()) {
				fprintf(stderr, "Error at %s, recolour file must be an 8bpp image.\n", this->pos.ToString());
				exit(1);
			}
			this->rim = new Image8bpp(this->rmf, nullptr);
			im->SetRecolourImage(this->rim);
		}
	}
	return this->img_sheet;
}

std::shared_ptr<BlockNode> SheetBlock::GetSubNode(int row, int col, const char *name, const Position &pos)
{
	Image *img = this->GetSheet();
	std::shared_ptr<SpriteBlock> spr_blk(new SpriteBlock);
	const char *err = nullptr;
	if (this->y_count >= 0 && row >= this->y_count) err = "No sprite available at the queried row.";
	if (err == nullptr && this->x_count >= 0 && col >= this->x_count) err = "No sprite available at the queried column.";
	if (err == nullptr) {
		err = spr_blk->sprite_image.CopySprite(img, this->x_offset, this->y_offset,
				this->x_base + this->x_step * col, this->y_base + this->y_step * row, this->width, this->height, this->crop);
	}
	if (err != nullptr) {
		fprintf(stderr, "Error at %s, loading of the sprite for \"%s\" failed: %s\n", pos.ToString(), name, err);
		exit(1);
	}
	return spr_blk;
}

std::shared_ptr<BlockNode> SpriteFilesBlock::GetSubNode(int row, int col, const char *name, const Position &pos)
{
	const char *err = nullptr;
	if (row >= 1) err = "No sprites available at this row.";
	if (err == nullptr && col >= this->file.GetCount()) err = "No sprite available at the queried column.";

	if (err != nullptr) {
report_error:
		fprintf(stderr, "Error at %s, loading of the sprite for \"%s\" failed: %s\n", pos.ToString(), name, err);
		exit(1);
	}

	ImageFile *imf = nullptr;
	ImageFile *rmf = nullptr;
	Image *img = nullptr;
	Image8bpp *rim = nullptr;

	imf = new ImageFile;
	err = imf->LoadFile(this->file.MakeFilename(col));
	if (err != nullptr) goto report_error;

	BitMaskData *bmd = (this->mask == nullptr) ? nullptr : &this->mask->data;
	if (imf->Is8bpp()) {
		img = new Image8bpp(imf, bmd);
		if (this->recolour.length >= 0) fprintf(stderr, "Error at %s, cannot recolour an 8bpp image, ignoring the file.\n", this->pos.ToString());
	} else {
		Image32bpp *im32 = new Image32bpp(imf, bmd);
		img = im32;
		if (this->recolour.length >= 0) {
			rmf = new ImageFile;
			err = rmf->LoadFile(this->recolour.MakeFilename(col));
			if (err != nullptr) goto report_error;
			if (!rmf->Is8bpp()) {
				err = "Recolour file is not an 8bpp image.\n";
				goto report_error;
			}
			rim = new Image8bpp(rmf, nullptr);
			im32->SetRecolourImage(rim);
		}
	}

	std::shared_ptr<SpriteBlock> spr_blk(new SpriteBlock);
	err = spr_blk->sprite_image.CopySprite(img, this->xoffset, this->yoffset, this->xbase, this->ybase, this->width, this->height, this->crop);
	if (err != nullptr) goto report_error;

	delete imf;
	delete rmf;
	delete img;
	delete rim;
	return spr_blk;
}

TSELBlock::TSELBlock() : GameBlock("TSEL", 2)
{
}

int TSELBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 108 - 12);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt16(this->z_height);
	for (int i = 0; i < SURFACE_COUNT; i++) {
		fb->SaveUInt32(this->sprites[i]->Write(fw));
	}
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

TCORBlock::TCORBlock() : GameBlock("TCOR", 2)
{
}

int TCORBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 384 - 12);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt16(this->z_height);
	for (int i = 0; i < SURFACE_COUNT; i++) fb->SaveUInt32(this->north[i]->Write(fw));
	for (int i = 0; i < SURFACE_COUNT; i++) fb->SaveUInt32(this->east[i]->Write(fw));
	for (int i = 0; i < SURFACE_COUNT; i++) fb->SaveUInt32(this->south[i]->Write(fw));
	for (int i = 0; i < SURFACE_COUNT; i++) fb->SaveUInt32(this->west[i]->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

SURFBlock::SURFBlock() : GameBlock("SURF", 6)
{
}

int SURFBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 110 - 12);
	fb->SaveUInt16(this->surf_type);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt16(this->z_height);
	for (int i = 0; i < SURFACE_COUNT; i++) {
		fb->SaveUInt32(this->sprites[i]->Write(fw));
	}
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

FUNDBlock::FUNDBlock() : GameBlock("FUND", 1)
{
}

int FUNDBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 42 - 12);
	fb->SaveUInt16(this->found_type);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt16(this->z_height);
	for (int i = 0; i < FOUNDATION_COUNT; i++) {
		fb->SaveUInt32(this->sprites[i]->Write(fw));
	}
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

Recolouring::Recolouring()
{
	this->orig = COLOUR_COUNT; // Invalid recolour by default.
	this->replace = 0;
}

/**
 * Encode a colour range remapping.
 * @return The encoded colour range remapping.
 */
uint32 Recolouring::Encode() const
{
	return (((uint32)this->orig) << 24) | (this->replace & 0xFFFFFF);
}

/**
 * Add a recolour mapping to the person graphics.
 * @param orig Colour range to replace.
 * @param replace Bitset of colour range that may be used instead.
 * @return Whether adding was successful.
 */
bool PersonGraphics::AddRecolour(uint8 orig, uint32 replace)
{
	if (orig >= COLOUR_COUNT || replace == 0) return true; // Invalid recolouring can always be stored.

	for (int i = 0; i < 3; i++) {
		if (this->recol[i].orig >= COLOUR_COUNT) {
			this->recol[i].orig = orig;
			this->recol[i].replace = replace;
			return true;
		}
	}
	return false;
}

PRSGBlock::PRSGBlock() : GameBlock("PRSG", 2)
{
}

int PRSGBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 1 + this->person_graphics.size() * 13);
	fb->SaveUInt8(this->person_graphics.size());

	for (const auto &pg : this->person_graphics) {
		fb->SaveUInt8(pg->person_type);
		fb->SaveUInt32(pg->recol[0].Encode());
		fb->SaveUInt32(pg->recol[1].Encode());
		fb->SaveUInt32(pg->recol[2].Encode());
	}
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

ANIMBlock::ANIMBlock() : GameBlock("ANIM", 4)
{
}

int ANIMBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 1 + 2 + 2 + this->frames.size() * 6);
	fb->SaveUInt8(this->person_type);
	fb->SaveUInt16(this->anim_type);
	fb->SaveUInt16(this->frames.size());

	for (const auto &fd : this->frames) {
		fb->SaveUInt16(fd->duration);
		fb->SaveInt16(fd->change_x);
		fb->SaveInt16(fd->change_y);
	}
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

ANSPBlock::ANSPBlock() : GameBlock("ANSP", 3)
{
}

int ANSPBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 2 + 1 + 2 + 2 + this->frames.size() * 4);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt8(this->person_type);
	fb->SaveUInt16(this->anim_type);
	fb->SaveUInt16(this->frames.size());
	for (auto iter : this->frames) {
		fb->SaveUInt32(iter->Write(fw));
	}
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

PATHBlock::PATHBlock() : GameBlock("PATH", 3)
{
}

int PATHBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 2 + 2 + 2 + PTS_COUNT * 4);
	fb->SaveUInt16(this->path_type);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt16(this->z_height);
	for (int i = 0; i < PTS_COUNT; i++) fb->SaveUInt32(this->sprites[i]->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

PDECBlock::PDECBlock() : GameBlock("PDEC", 1)
{
}

/**
 * Write the sprite if available.
 * @param spr Sprite to wite (if not \c nullptr).
 * @param fw File to write to.
 * @return \c 0 if no sprite available, else the block number of the written sprite.
 */
static uint32 WriteSprite(std::shared_ptr<SpriteBlock> &spr, FileWriter *fw)
{
	if (spr == nullptr) return 0;
	return spr->Write(fw);
}

int PDECBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 286 - 12);
	fb->SaveUInt16(this->tile_width);
	for (int i = 0; i < 4; i++) fb->SaveUInt32(WriteSprite(this->litter_bin[i], fw));
	for (int i = 0; i < 4; i++) fb->SaveUInt32(WriteSprite(this->overflow_bin[i], fw));
	for (int i = 0; i < 4; i++) fb->SaveUInt32(WriteSprite(this->demolished_bin[i], fw));
	for (int i = 0; i < 4; i++) fb->SaveUInt32(WriteSprite(this->lamp_post[i], fw));
	for (int i = 0; i < 4; i++) fb->SaveUInt32(WriteSprite(this->demolished_post[i], fw));
	for (int i = 0; i < 4; i++) fb->SaveUInt32(WriteSprite(this->bench[i], fw));
	for (int i = 0; i < 4; i++) fb->SaveUInt32(WriteSprite(this->demolished_bench[i], fw));
	for (int i = 0; i < 4; i++) fb->SaveUInt32(WriteSprite(this->litter_flat[i], fw));
	for (int i = 0; i < 4; i++) fb->SaveUInt32(WriteSprite(this->litter_ne[i], fw));
	for (int i = 0; i < 4; i++) fb->SaveUInt32(WriteSprite(this->litter_se[i], fw));
	for (int i = 0; i < 4; i++) fb->SaveUInt32(WriteSprite(this->litter_sw[i], fw));
	for (int i = 0; i < 4; i++) fb->SaveUInt32(WriteSprite(this->litter_nw[i], fw));
	for (int i = 0; i < 4; i++) fb->SaveUInt32(WriteSprite(this->vomit_flat[i], fw));
	for (int i = 0; i < 4; i++) fb->SaveUInt32(WriteSprite(this->vomit_ne[i], fw));
	for (int i = 0; i < 4; i++) fb->SaveUInt32(WriteSprite(this->vomit_se[i], fw));
	for (int i = 0; i < 4; i++) fb->SaveUInt32(WriteSprite(this->vomit_sw[i], fw));
	for (int i = 0; i < 4; i++) fb->SaveUInt32(WriteSprite(this->vomit_nw[i], fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

PLATBlock::PLATBlock() : GameBlock("PLAT", 2)
{
}

int PLATBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 2 + 2 + 2 + PLA_COUNT * 4);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt16(this->z_height);
	fb->SaveUInt16(this->platform_type);
	for (int i = 0; i < PLA_COUNT; i++) fb->SaveUInt32(this->sprites[i]->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

SUPPBlock::SUPPBlock() : GameBlock("SUPP", 1)
{
}

int SUPPBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 2 + 2 + 2 + SPP_COUNT * 4);
	fb->SaveUInt16(this->support_type);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt16(this->z_height);
	for (int i = 0; i < SPP_COUNT; i++) fb->SaveUInt32(this->sprites[i]->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

/** Names of the known languages. */
static const char *_languages[] = {
	"da_DK", // LNG_DA_DK
	"de_DE", // LNG_DE_DE
	"en_GB", // LNG_EN_GB (default)
	"en_US", // LNG_EN_US
	"es_ES", // LNG_ES_ES
	"fr_FR", // LNG_FR_FR
	"nds_DE", // LNG_NDS_DE
	"nl_NL", // LNG_NL_NL
	"sv_SE", // LNG_SV_SE
};

/**
 * Get the index of a language from its name.
 * @param lname Name of the language.
 * @param pos %Position stating the name.
 * @return Index of the given language.
 */
int GetLanguageIndex(const char *lname, const Position &pos)
{
	assert(LNG_COUNT == sizeof(_languages) / sizeof(char *));
	for (int i = 0; i < LNG_COUNT; i++) {
		if (strcmp(_languages[i], lname) == 0) return i;
	}
	fprintf(stderr, "Error at %s: Language \"%s\" is not known\n", pos.ToString(), lname);
	exit(1);
}

StringNode::StringNode() : BlockNode()
{
	this->lang_index = -1;
}

StringsNode::StringsNode() : BlockNode()
{
}

/**
 * Add a string to the collection of strings.
 * @param node String to add.
 * @param pos Position of the string node, for error reporting.
 */
void StringsNode::Add(const StringNode &node, const Position &pos)
{
	for (const auto &iter : this->strings) {
		/* Check for duplicates: No same names with same language and same key. */
		if (iter.name != node.name) continue;
		if (iter.lang_index >= 0 && node.lang_index >= 0 && iter.lang_index != node.lang_index) continue;
		if (iter.key == "" || node.key == "" || iter.key != node.key) continue;
		fprintf(stderr, "Error at %s: ", node.text_pos.ToString());
		fprintf(stderr, "\"string node\" conflicts with node at %s\n", iter.text_pos.ToString());
		exit(1);
	}
	this->strings.push_back(node);
	if (this->key != "") {
		if (node.key == "") {
			this->strings.back().key = this->key;
		} else if (this->key != node.key) {
			fprintf(stderr, "Error at %s: String \"%s\" already has key \"%s\".\n", pos.ToString(), node.name.c_str(), node.key.c_str());
			exit(1);

		}
	}
}

/**
 * Set the key of the strings collection. Can only be done if it is not set already.
 * @param key New key of the bundle that the collection belongs to.
 * @param pos Position of setting.
 */
void StringsNode::SetKey(const std::string &key, const Position &pos)
{
	if (this->key != "") {
		fprintf(stderr, "Error at %s: Strings already have key \"%s\".\n", pos.ToString(), this->key.c_str());
		exit(1);
	}
	this->key = key;
	for (auto &str : this->strings) {
		if (str.key != "" && str.key != key) {
			fprintf(stderr, "Error at %s: String \"%s\" already has key \"%s\".\n", pos.ToString(), str.name.c_str(), str.key.c_str());
			exit(1);
		}
	}
}

/**
 * Retrieve the key from the stored strings.
 * @return Key of all strings that have one,if available, else \c "".
 * @todo This looks like it could be moved to \c this->key.
 */
std::string StringsNode::GetKey() const
{
	std::string key;
	bool set_key = false;
	for (auto &str : this->strings) {
		if (!set_key) {
			if (str.key != "") {
				key = str.key;
				set_key = true;
			}
			continue;
		}
		if (str.key != "" && key != str.key) return ""; // Two or more different keys, no sane way to decide what it is.
	}
	if (set_key) return key;
	return this->key; // Can only give a result when there are no strings in this node.
}

/**
 * Constructor of a #TextNode.
 * @param name Name of the string stored in the text node.
 */
TextNode::TextNode(const std::string &name) : name(name)
{
	for (int i = 0; i < LNG_COUNT; i++) this->pos[i] = Position("", -1);
}

/**
 * Merge text node from storage into this text node. Already present strings have precedence over storage strings.
 * @param storage Text to merge.
 */
void TextNode::MergeStorage(const TextNode &storage)
{
	assert(this->name == storage.name);
	for (int i = 0; i < LNG_COUNT; i++) {
		if (this->pos[i].line < 0 && storage.pos[i].line >= 0) {
			this->pos[i] = storage.pos[i];
			this->texts[i] = storage.texts[i];
		}
	}
}

/**
 * Compute the number of bytes needed to store this text node in an RCD file.
 * @return Size needed to store this string.
 */
int TextNode::GetSize() const
{
	int length = 2 + 1 + this->name.size() + 1;
	for (int i = 0; i < LNG_COUNT; i++) {
		if (this->pos[i].line < 0) continue;
		length += 2 + (1 + strlen(_languages[i]) + 1) + 1;
		for (const std::string &text : this->texts[i]) length += text.size() + 1;
	}
	return length;
}

/**
 * Write the string and its translations into a file block.
 * @param fb File block to write to.
 */
void TextNode::Write(FileBlock *fb) const
{
	int length = this->GetSize();
	fb->SaveUInt16(length);
	length -= 2;

	assert(this->name.size() + 1 < 256);
	fb->SaveUInt8(this->name.size() + 1);
	fb->SaveBytes((uint8 *)this->name.c_str(), this->name.size());
	fb->SaveUInt8(0);
	length -= 1 + this->name.size() + 1;
	for (int i = 0; i < LNG_COUNT; i++) {
		if (this->pos[i].line < 0) continue;
		int lname_length = strlen(_languages[i]);
		int lng_size = 2 + (1 + lname_length + 1) + 1;
		for (const std::string &str : this->texts[i]) lng_size += str.size() + 1;

		fb->SaveUInt16(lng_size);
		fb->SaveUInt8(lname_length + 1);
		fb->SaveBytes((uint8 *)_languages[i], lname_length);
		fb->SaveUInt8(0);
		fb->SaveUInt8(this->texts[i].size());
		length -= 2 + 1 + lname_length + 1 + 1;

		for (const std::string &str : this->texts[i]) {
			fb->SaveBytes((uint8 *)str.c_str(), str.size());
			fb->SaveUInt8(0);
			length -= str.size() + 1;
		}
	}
	assert(this->pos[LNG_EN_GB].line >= 0 && length == 0);
}

/**
 * Copy the strings into the bundle, ordered by the string name.
 * @param strs Strings to copy.
 * @param pos Position of the string bundle, for error reporting.
 */
void StringBundle::Fill(std::shared_ptr<StringsNode> strs, const Position &pos)
{
	std::string strs_key = strs->GetKey();
	if (this->key == "") {
		this->key = strs_key;
	} else if (strs_key != "" && this->key != strs_key) {
		fprintf(stderr, "Error at %s: Bundle gets key \"%s\" but already has key \"%s\".\n", pos.ToString(), this->key.c_str(), strs_key.c_str());
		exit(1);
	}
	for (StringNode &str : strs->strings) {
		if (str.lang_index < 0) {
			fprintf(stderr, "Error at %s: String does not have a language.\n", str.text_pos.ToString());
			exit(1);
		}

		/* Get the text node associated with the string name. */
		auto iter = this->texts.find(str.name);
		if (iter == this->texts.end()) {
			std::pair<std::string, TextNode> p(str.name, TextNode(str.name));
			iter = this->texts.insert(p).first;
		}
		TextNode &tn = iter->second;
		if (tn.pos[str.lang_index].line >= 0) { // String with same name and same language already exists.
			fprintf(stderr, "Error at %s: ", str.text_pos.ToString());
			fprintf(stderr, "Text for language %s already defined at %s.\n", _languages[str.lang_index], tn.pos[str.lang_index].ToString());
			exit(1);
		}
		tn.texts[str.lang_index] = str.text;
		tn.pos[str.lang_index] = str.text_pos;
	}
}

/**
 * Merge bundle strings from the storage with bundle. Already available strings take precedence.
 * @param storage Bundle from storage to merge.
 */
void StringBundle::MergeStorage(const StringBundle &storage)
{
	assert(this->key == "" || storage.key == "" || this->key == storage.key);
	for (const auto &stor : storage.texts) {
		auto iter = this->texts.find(stor.first);
		if (iter == this->texts.end()) {
			std::pair<std::string, TextNode> p(stor.first, stor.second);
			this->texts.insert(p);
			continue;
		}
		iter->second.MergeStorage(stor.second);
	}
}

/**
 * Verify whether the strings are all valid.
 * @param names Expected string names.
 * @param name_count Number of names in \a names.
 * @param pos %Position of the surrounding node (for error reporting).
 */
void StringBundle::CheckTranslations(const char *names[], int name_count, const Position &pos)
{
	if (this->key != "") {	// Merge strings from storage, if available.
		const StringBundle *stored = _strings_storage.GetBundle(this->key);
		if (stored != nullptr) this->MergeStorage(*stored);
	}

	/* Check that the bundle has no extra strings. */
	for (const auto &text : this->texts) {
		bool found = false;
		for (int i = 0; i < name_count; i++) {
			if (names[i] == text.first) {
				found = true;
				break;
			}
		}
		if (!found) printf("Warning at %s: String \"%s\" is not needed.\n", pos.ToString(), text.first.c_str());
	}

	/* Check that all necessary strings exist. */
	for (int i = 0; i < name_count; i++) {
		if (this->texts.find(names[i]) == this->texts.end()) {
			fprintf(stderr, "Error at %s: String \"%s\" is not defined\n", pos.ToString(), names[i]);
			exit(1);
		}
	}

	/* Check that all strings have a British English text, and count the missing translations. */
	std::array<int, LNG_COUNT> missing_count{{}};

	for (const auto &iter : this->texts) {
		if (iter.second.pos[LNG_EN_GB].line < 0) {
			fprintf(stderr, "Error at %s: String \"%s\" has no British English language text\n", pos.ToString(), iter.first.c_str());
			exit(1);
		}
		for (int i = 0; i < LNG_COUNT; i++) {
			if (iter.second.pos[i].line < 0) missing_count[i]++;
		}
	}

	for (uint i = 0; i < missing_count.size(); i++) {
		if (i == LNG_EN_GB) continue; // There are no "missing" British English strings.
		if (missing_count[i] > 0) {
			printf("Language %s has %i missing translations in %s\n", _languages[i], missing_count[i], pos.filename.c_str());
		}
	}
}

/**
 * Write the strings in a 'TEXT' block.
 * @param fw File to write to.
 * @return Block number where the data was saved.
 */
int StringBundle::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	int length = 0;
	for (const auto &iter : this->texts) length += iter.second.GetSize();

	fb->StartSave("TEXT", 3, length);
	for (const auto &iter : this->texts) iter.second.Write(fb);
	fb->CheckEndSave();

	return fw->AddBlock(fb);
}

SHOPBlock::SHOPBlock() : GameBlock("SHOP", 6)
{
}

int SHOPBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 52 - 12);
	fb->SaveUInt8(this->height);
	fb->SaveUInt8(this->flags);
	fb->SaveUInt32(this->views->Write(fw));
	fb->SaveUInt32(this->recol[0].Encode());
	fb->SaveUInt32(this->recol[1].Encode());
	fb->SaveUInt32(this->recol[2].Encode());
	fb->SaveUInt32(this->item_cost[0]);
	fb->SaveUInt32(this->item_cost[1]);
	fb->SaveUInt32(this->ownership_cost);
	fb->SaveUInt32(this->opened_cost);
	fb->SaveUInt8(this->item_type[0]);
	fb->SaveUInt8(this->item_type[1]);
	fb->SaveUInt32(this->shop_text->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

FSETBlock::FSETBlock() : GameBlock("FSET", 1)
{
	this->unrotated_views_only = false;
	this->unrotated_views_only_allowed = false;
}

int FSETBlock::Write(FileWriter *fw)
{
	if (!this->unrotated_views_only_allowed && this->unrotated_views_only) {
		fprintf(stderr, "Error: FSET block which requires all views to be specified has specified only the unrotated views");
		::exit(1);
	}
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 16 + (16 * this->width_x * this->width_y) - 12);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt8(this->width_x);
	fb->SaveUInt8(this->width_y);
	for (int x = 0; x < this->width_x; ++x) {
		for (int y = 0; y < this->width_y; ++y) {
			fb->SaveUInt32(this->ne_views[x * this->width_y + y]->Write(fw));
		}
	}
	for (int x = 0; x < this->width_x; ++x) {
		for (int y = 0; y < this->width_y; ++y) {
			fb->SaveUInt32(this->se_views[x * this->width_y + y]->Write(fw));
		}
	}
	for (int x = 0; x < this->width_x; ++x) {
		for (int y = 0; y < this->width_y; ++y) {
			fb->SaveUInt32(this->sw_views[x * this->width_y + y]->Write(fw));
		}
	}
	for (int x = 0; x < this->width_x; ++x) {
		for (int y = 0; y < this->width_y; ++y) {
			fb->SaveUInt32(this->nw_views[x * this->width_y + y]->Write(fw));
		}
	}
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

TIMABlock::TIMABlock() : GameBlock("TIMA", 1)
{
	this->unrotated_views_only = false;
	this->unrotated_views_only_allowed = false;
}

int TIMABlock::Write(FileWriter *fw)
{
	if (!this->unrotated_views_only_allowed && this->unrotated_views_only) {
		fprintf(stderr, "Error: TIMA block which requires all views to be specified has specified only the unrotated views");
		::exit(1);
	}
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 16 + (8 * this->frames) - 12);
	fb->SaveUInt32(this->frames);
	for (int f = 0; f < this->frames; ++f) fb->SaveUInt32(this->durations[f]);
	for (int f = 0; f < this->frames; ++f) {
		this->views[f]->unrotated_views_only_allowed = this->unrotated_views_only_allowed;
		fb->SaveUInt32(this->views[f]->Write(fw));
	}
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

RIEEBlock::RIEEBlock() : GameBlock("RIEE", 1)
{
}

int RIEEBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 63 - 12);
	fb->SaveUInt8(this->is_entrance ? 1 : 0);
	fb->SaveUInt32(this->texts->Write(fw));
	fb->SaveUInt16(this->tile_width);
	for (std::shared_ptr<SpriteBlock>& v : this->ne_views) fb->SaveUInt32(v->Write(fw));
	for (std::shared_ptr<SpriteBlock>& v : this->se_views) fb->SaveUInt32(v->Write(fw));
	for (std::shared_ptr<SpriteBlock>& v : this->sw_views) fb->SaveUInt32(v->Write(fw));
	for (std::shared_ptr<SpriteBlock>& v : this->nw_views) fb->SaveUInt32(v->Write(fw));
	for (Recolouring& r : this->recol) fb->SaveUInt32(r.Encode());
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

FGTRBlock::FGTRBlock() : GameBlock("FGTR", 4)
{
}

int FGTRBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 123 + (this->ride_width_x * this->ride_width_y) - 12);
	fb->SaveUInt8(this->is_thrill_ride ? 1 : 0);
	fb->SaveUInt8(this->ride_width_x);
	fb->SaveUInt8(this->ride_width_y);
	for (int8 x = 0; x < this->ride_width_x; ++x) {
		for (int8 y = 0; y < this->ride_width_y; ++y) {
			fb->SaveUInt8(this->heights[x * ride_width_y + y]);
		}
	}
	fb->SaveUInt32(this->idle_animation->Write(fw));
	fb->SaveUInt32(this->starting_animation->Write(fw));
	fb->SaveUInt32(this->working_animation->Write(fw));
	fb->SaveUInt32(this->stopping_animation->Write(fw));
	for (auto& preview : this->previews) fb->SaveUInt32(preview->Write(fw));
	fb->SaveUInt32(this->recol[0].Encode());
	fb->SaveUInt32(this->recol[1].Encode());
	fb->SaveUInt32(this->recol[2].Encode());
	fb->SaveUInt32(this->entrance_fee);
	fb->SaveUInt32(this->ownership_cost);
	fb->SaveUInt32(this->opened_cost);
	fb->SaveUInt32(this->number_of_batches);
	fb->SaveUInt32(this->guests_per_batch);
	fb->SaveUInt32(this->idle_duration);
	fb->SaveUInt32(this->working_duration);
	fb->SaveUInt16(this->working_cycles_min);
	fb->SaveUInt16(this->working_cycles_max);
	fb->SaveUInt16(this->working_cycles_default);
	fb->SaveUInt16(this->reliability_max);
	fb->SaveUInt16(this->reliability_decrease_daily);
	fb->SaveUInt16(this->reliability_decrease_monthly);
	fb->SaveUInt32(this->intensity_base);
	fb->SaveUInt32(this->nausea_base);
	fb->SaveUInt32(this->excitement_base);
	fb->SaveUInt32(this->excitement_increase_cycle);
	fb->SaveUInt32(this->excitement_increase_scenery);
	fb->SaveUInt32(this->ride_text->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

SCNYBlock::SCNYBlock() : GameBlock("SCNY", 2)
{
}

int SCNYBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 64 - 12 + (this->width_x * this->width_y));
	fb->SaveUInt8(this->width_x);
	fb->SaveUInt8(this->width_y);
	for (int8 x = 0; x < this->width_x; ++x) {
		for (int8 y = 0; y < this->width_y; ++y) {
			fb->SaveUInt8(this->heights[x * width_y + y]);
		}
	}
	fb->SaveUInt32(this->watering_interval);
	fb->SaveUInt32(this->min_watering_interval);
	fb->SaveUInt32(this->main_animation->Write(fw));
	fb->SaveUInt32(this->dry_animation->Write(fw));
	for (auto& preview : this->previews) fb->SaveUInt32(preview->Write(fw));
	fb->SaveInt32(this->buy_cost);
	fb->SaveInt32(this->return_cost);
	fb->SaveInt32(this->return_cost_dry);
	fb->SaveUInt8(this->symmetric ? 1 : 0);
	fb->SaveUInt8(this->category);
	fb->SaveUInt32(this->texts->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

GBORBlock::GBORBlock() : GameBlock("GBOR", 2)
{
}

int GBORBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 58 - 12);
	fb->SaveUInt16(this->widget_type);
	fb->SaveUInt8(this->border_top);
	fb->SaveUInt8(this->border_left);
	fb->SaveUInt8(this->border_right);
	fb->SaveUInt8(this->border_bottom);
	fb->SaveUInt8(this->min_width);
	fb->SaveUInt8(this->min_height);
	fb->SaveUInt8(this->h_stepsize);
	fb->SaveUInt8(this->v_stepsize);
	fb->SaveUInt32(this->tl->Write(fw));
	fb->SaveUInt32(this->tm->Write(fw));
	fb->SaveUInt32(this->tr->Write(fw));
	fb->SaveUInt32(this->ml->Write(fw));
	fb->SaveUInt32(this->mm->Write(fw));
	fb->SaveUInt32(this->mr->Write(fw));
	fb->SaveUInt32(this->bl->Write(fw));
	fb->SaveUInt32(this->bm->Write(fw));
	fb->SaveUInt32(this->br->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

GCHKBlock::GCHKBlock() : GameBlock("GCHK", 1)
{
}

int GCHKBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 38 - 12);
	fb->SaveUInt16(this->widget_type);
	fb->SaveUInt32(this->empty->Write(fw));
	fb->SaveUInt32(this->filled->Write(fw));
	fb->SaveUInt32(this->empty_pressed->Write(fw));
	fb->SaveUInt32(this->filled_pressed->Write(fw));
	fb->SaveUInt32(this->shaded_empty->Write(fw));
	fb->SaveUInt32(this->shaded_filled->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

GSLIBlock::GSLIBlock() : GameBlock("GSLI", 1)
{
}

int GSLIBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 33 - 12);
	fb->SaveUInt8(this->min_length);
	fb->SaveUInt8(this->step_size);
	fb->SaveUInt8(this->width);
	fb->SaveUInt16(this->widget_type);
	fb->SaveUInt32(this->left->Write(fw));
	fb->SaveUInt32(this->middle->Write(fw));
	fb->SaveUInt32(this->right->Write(fw));
	fb->SaveUInt32(this->slider->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

GSCLBlock::GSCLBlock() : GameBlock("GSCL", 1)
{
}

int GSCLBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 70 - 12);
	fb->SaveUInt8(this->min_length);
	fb->SaveUInt8(this->step_back);
	fb->SaveUInt8(this->min_bar_length);
	fb->SaveUInt8(this->bar_step);
	fb->SaveUInt16(this->widget_type);
	fb->SaveUInt32(this->left_button->Write(fw));
	fb->SaveUInt32(this->right_button->Write(fw));
	fb->SaveUInt32(this->left_pressed->Write(fw));
	fb->SaveUInt32(this->right_pressed->Write(fw));
	fb->SaveUInt32(this->left_bottom->Write(fw));
	fb->SaveUInt32(this->middle_bottom->Write(fw));
	fb->SaveUInt32(this->right_bottom->Write(fw));
	fb->SaveUInt32(this->left_top->Write(fw));
	fb->SaveUInt32(this->middle_top->Write(fw));
	fb->SaveUInt32(this->right_top->Write(fw));
	fb->SaveUInt32(this->left_top_pressed->Write(fw));
	fb->SaveUInt32(this->middle_top_pressed->Write(fw));
	fb->SaveUInt32(this->right_top_pressed->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

BDIRBlock::BDIRBlock() : GameBlock("BDIR", 1)
{
}

int BDIRBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 30 - 12);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt32(this->sprite_ne->Write(fw));
	fb->SaveUInt32(this->sprite_se->Write(fw));
	fb->SaveUInt32(this->sprite_sw->Write(fw));
	fb->SaveUInt32(this->sprite_nw->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

GSLPBlock::GSLPBlock() : GameBlock("GSLP", 14)
{
}

int GSLPBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 300 - 12);
	fb->SaveUInt32(this->vert_down->Write(fw));
	fb->SaveUInt32(this->steep_down->Write(fw));
	fb->SaveUInt32(this->gentle_down->Write(fw));
	fb->SaveUInt32(this->level->Write(fw));
	fb->SaveUInt32(this->gentle_up->Write(fw));
	fb->SaveUInt32(this->steep_up->Write(fw));
	fb->SaveUInt32(this->vert_up->Write(fw));
	fb->SaveUInt32(this->wide_left->Write(fw));
	fb->SaveUInt32(this->normal_left->Write(fw));
	fb->SaveUInt32(this->tight_left->Write(fw));
	fb->SaveUInt32(this->no_bend->Write(fw));
	fb->SaveUInt32(this->tight_right->Write(fw));
	fb->SaveUInt32(this->normal_right->Write(fw));
	fb->SaveUInt32(this->wide_right->Write(fw));
	fb->SaveUInt32(this->no_banking->Write(fw));
	fb->SaveUInt32(this->bank_left->Write(fw));
	fb->SaveUInt32(this->bank_right->Write(fw));
	fb->SaveUInt32(this->triangle_right->Write(fw));
	fb->SaveUInt32(this->triangle_left->Write(fw));
	fb->SaveUInt32(this->triangle_up->Write(fw));
	fb->SaveUInt32(this->triangle_bottom->Write(fw));
	fb->SaveUInt32(this->has_platform->Write(fw));
	fb->SaveUInt32(this->no_platform->Write(fw));
	fb->SaveUInt32(this->has_power->Write(fw));
	fb->SaveUInt32(this->no_power->Write(fw));
	fb->SaveUInt32(this->disabled->Write(fw));
	for (uint i = 0; i < lengthof(this->compass); i++) {
		fb->SaveUInt32(this->compass[i]->Write(fw));
	}
	fb->SaveUInt32(this->bulldozer->Write(fw));
	for (uint i = 0; i < lengthof(this->weather); i++) {
		fb->SaveUInt32(this->weather[i]->Write(fw));
	}
	for (uint i = 0; i < lengthof(this->rog_lights); i++) {
		fb->SaveUInt32(this->rog_lights[i]->Write(fw));
	}
	for (uint i = 0; i < lengthof(this->rg_lights); i++) {
		fb->SaveUInt32(this->rg_lights[i]->Write(fw));
	}
	fb->SaveUInt32(this->pos_2d->Write(fw));
	fb->SaveUInt32(this->neg_2d->Write(fw));
	fb->SaveUInt32(this->pos_3d->Write(fw));
	fb->SaveUInt32(this->neg_3d->Write(fw));
	fb->SaveUInt32(this->close_button->Write(fw));
	fb->SaveUInt32(this->terraform_dot->Write(fw));
	fb->SaveUInt32(this->message_goto->Write(fw));
	fb->SaveUInt32(this->message_park->Write(fw));
	fb->SaveUInt32(this->message_guest->Write(fw));
	fb->SaveUInt32(this->message_ride->Write(fw));
	fb->SaveUInt32(this->message_ride_type->Write(fw));
	fb->SaveUInt32(this->loadsave_err->Write(fw));
	fb->SaveUInt32(this->loadsave_warn->Write(fw));
	fb->SaveUInt32(this->loadsave_ok->Write(fw));
	fb->SaveUInt32(this->toolbar_main->Write(fw));
	fb->SaveUInt32(this->toolbar_speed->Write(fw));
	fb->SaveUInt32(this->toolbar_path->Write(fw));
	fb->SaveUInt32(this->toolbar_ride->Write(fw));
	fb->SaveUInt32(this->toolbar_fence->Write(fw));
	fb->SaveUInt32(this->toolbar_scenery->Write(fw));
	fb->SaveUInt32(this->toolbar_terrain->Write(fw));
	fb->SaveUInt32(this->toolbar_staff->Write(fw));
	fb->SaveUInt32(this->toolbar_inbox->Write(fw));
	fb->SaveUInt32(this->toolbar_finances->Write(fw));
	fb->SaveUInt32(this->toolbar_objects->Write(fw));
	fb->SaveUInt32(this->toolbar_view->Write(fw));
	fb->SaveUInt32(this->toolbar_park->Write(fw));
	fb->SaveUInt32(this->gui_text->Write(fw));
	fb->SaveUInt32(this->meta_text->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

MENUBlock::MENUBlock() : GameBlock("MENU", 1)
{
}

int MENUBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 40 - 12);
	fb->SaveUInt32(this->splash_duration);
	fb->SaveUInt32(this->logo->Write(fw));
	fb->SaveUInt32(this->splash->Write(fw));
	fb->SaveUInt32(this->new_game->Write(fw));
	fb->SaveUInt32(this->load_game->Write(fw));
	fb->SaveUInt32(this->settings->Write(fw));
	fb->SaveUInt32(this->quit->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

CubicSpline::CubicSpline() : BlockNode()
{
	this->a = 0;
	this->b = 0;
	this->c = 0;
	this->d = 0;
	this->steps = 0;
	this->first = 0;
	this->last = 0;
}

Curve::Curve() : BlockNode()
{
}

/**
 * \fn double Curve::GetValue(int index, int step)
 * Get the value of the curve at the provided distance index.
 * @param index Distance index to retrieve.
 * @param step Step value in the curve (of CubicSpline::steps steps).
 * @return Value of the curve at the queried point.
 */

CubicSplines::CubicSplines() : Curve()
{
}

double CubicSplines::GetValue(int index, int step)
{
	const auto spline = this->curve.at(index);
	assert(step < spline->steps);
	double t = ((double)step) / (spline->steps - 1);
	double tt = t * t;
	double t1 = 1 - t;
	double tt11 = t1 * t1;

	double ta = tt11 * t1;
	double tb = 3 * tt11 * t;
	double tc = 3 * t1 * tt;
	double td = tt * t;

	return ta * spline->a + tb * spline->b + tc * spline->c + td * spline->d;
}

FixedTable::FixedTable() : Curve()
{
	this->value = 0;
}

double FixedTable::GetValue(int index, int step)
{
	return this->value;
}

TrackVoxel::TrackVoxel() : BlockNode()
{
	this->dx = 0;
	this->dy = 0;
	this->dz = 0;
	this->flags = 0;
}

/**
 * Rotate relative coordinate (\a dx, \a dy) \a count times -90 degrees.
 * @param dx [inout] X coordinate to rotate.
 * @param dy [inout] Y coordinate to rotate.
 * @param count Number of quarter rotations.
 */
static void RotateXY(int *dx, int *dy, int count)
{
	for (; count > 0; count--) {
		int ny = -(*dx);
		*dx = *dy;
		*dy = ny;
	}
}

/**
 * Rotate the track voxel flags.
 * @param flags Original flags.
 * @param count Number of quarter rotations.
 * @return The rotated flags.
 */
static int RotateFlags(int flags, int count)
{
	int rot_flags = 0;
	/* Rotate the space requirements. */
	for (int i = 0; i < 4; i++) {
		if ((flags & (1 << i)) == 0) continue;
		int j = (i + count) & 3;
		rot_flags |= 1 << j;
	}

	/* Rotate the platform. */
	flags = (flags >> 4) & 7;
	if (flags != 0) flags = 1 + ((flags - 1 + count) & 3);
	return rot_flags | (flags << 4);
}

/**
 * Write a track voxel (after rotating it \a rot times first).
 * @param fw Output stream to write the graphics to.
 * @param fb File block to write to.
 * @param rot Number of -90 degrees rotation to apply first.
 */
void TrackVoxel::Write(FileWriter *fw, FileBlock *fb, int rot)
{
	for (int i = 0; i < 4; i++) {
		int j = (i + 4 - rot) & 3;
		if (this->back[j] == nullptr) {
			fb->SaveUInt32(0);
		} else {
			fb->SaveUInt32(this->back[j]->Write(fw));
		}
	}
	for (int i = 0; i < 4; i++) {
		int j = (i + 4 - rot) & 3;
		if (this->front[j] == nullptr) {
			fb->SaveUInt32(0);
		} else {
			fb->SaveUInt32(this->front[j]->Write(fw));
		}
	}
	int nx = this->dx;
	int ny = this->dy;
	RotateXY(&nx, &ny, rot);
	fb->SaveInt8(nx);
	fb->SaveInt8(ny);
	fb->SaveInt8(this->dz);
	fb->SaveUInt8(RotateFlags(this->flags, rot));
}

Connection::Connection() : BlockNode()
{
	this->name = "?";
	this->direction = 0;
}

/**
 * Copy constructor.
 * @param c %Connection to copy.
 */
Connection::Connection(const Connection &c)
{
	this->name = c.name;
	this->direction = c.direction;
}

/**
 * Assignment operator
 * @param c %Connection to copy.
 * @return The copy.
 */
Connection &Connection::operator=(const Connection &c)
{
	if (&c != this) {
		this->name = c.name;
		this->direction = c.direction;
	}
	return *this;
}

/**
 * Constructor of a %Connection.
 * @param name Name of the connection.
 * @param direction Direction of the connection.
 */
Connection::Connection(const std::string &name, int direction)
{
	this->name = name;
	this->direction = direction;
}

/**
 * Encode a connection after rotating it.
 * @param connections Map of connection names to their id.
 * @param rot Number of -90 degrees rotation to apply first.
 * @return The encoded rotated connection code.
 */
uint8 Connection::Encode(const std::map<std::string, int> &connections, int rot)
{
	const auto iter = connections.find(this->name);
	assert(iter != connections.end());
	return (iter->second << 2) | ((this->direction + rot) & 3);
}

/**
 * Verify that the connection names of the track voxel are present in the \a connections. If not, add them.
 * @param connections [inout] Map of connection names to their id.
 */
void TrackPieceNode::UpdateConnectionMap(std::map<std::string, int> *connections)
{
	if (connections->find(this->entry->name) == connections->end()) {
		std::pair<std::string, int> pp(this->entry->name, connections->size());
		connections->insert(pp);
	}

	if (connections->find(this->exit->name) == connections->end()) {
		std::pair<std::string, int> pp(this->exit->name, connections->size());
		connections->insert(pp);
	}
}

/**
 * Set the start or the end of a bezier spline.
 * @param index Index in the curve denoting the bezier spline to change.
 * @param splines Curve that may be contain bezier splines, \c nullptr is that is not the case.
 * @param set_start If set, set the start, else set the end.
 * @param value Value to set.
 */
static void SetStartEnd(int index, std::shared_ptr<CubicSplines> splines, bool set_start, int value)
{
	if (splines == nullptr) return;
	std::shared_ptr<CubicSpline> spline = splines->curve[index];
	if (set_start) {
		spline->first = value;
	} else {
		spline->last = value;
	}
}

/**
 * Set the start or the end of a bezier spline in all track piece curves.
 * @param index Index in the curve denoting the bezier spline to change.
 * @param set_start If set, set the start, else set the end.
 * @param value Value to set.
 */
void TrackPieceNode::SetStartEnd(int index, bool set_start, int value)
{
	::SetStartEnd(index, std::dynamic_pointer_cast<CubicSplines>(this->car_xpos),  set_start, value);
	::SetStartEnd(index, std::dynamic_pointer_cast<CubicSplines>(this->car_ypos),  set_start, value);
	::SetStartEnd(index, std::dynamic_pointer_cast<CubicSplines>(this->car_zpos),  set_start, value);
	::SetStartEnd(index, std::dynamic_pointer_cast<CubicSplines>(this->car_pitch), set_start, value);
	::SetStartEnd(index, std::dynamic_pointer_cast<CubicSplines>(this->car_roll),  set_start, value);
	::SetStartEnd(index, std::dynamic_pointer_cast<CubicSplines>(this->car_yaw),   set_start, value);
}

/**
 * Check the number of bezier splines against expectations, and report an error if the count is incorrect.
 * @param count Expected number of bezier splines.
 * @param splines Curve that may be contain bezier splines, \c nullptr is that is not the case.
 * @param name Name of the curve to check.
 * @param pos %Position of the track piece in the source file.
 */
static void CheckBezierCount(int count, const std::shared_ptr<CubicSplines> splines, const char *name, const Position &pos)
{
	if (splines == nullptr || (int)splines->curve.size() == count) return;
	fprintf(stderr, "Error at %s: Track piece curve '%s' has %d curves instead of the expected %d.\n", pos.ToString(), name, (int)splines->curve.size(), count);
	::exit(1);
}

/**
 * Check the number of bezier splines against expectations for all possible curves in the track piece, and report an error if the count is incorrect.
 * @param count Expected number of bezier splines.
 * @param pos %Position of the track piece in the source file.
 */
void TrackPieceNode::CheckBezierCount(int count, const Position &pos)
{
	::CheckBezierCount(count, std::dynamic_pointer_cast<CubicSplines>(this->car_xpos),  "car_xpos",  pos);
	::CheckBezierCount(count, std::dynamic_pointer_cast<CubicSplines>(this->car_ypos),  "car_ypos",  pos);
	::CheckBezierCount(count, std::dynamic_pointer_cast<CubicSplines>(this->car_zpos),  "car_zpos",  pos);
	::CheckBezierCount(count, std::dynamic_pointer_cast<CubicSplines>(this->car_pitch), "car_pitch", pos);
	::CheckBezierCount(count, std::dynamic_pointer_cast<CubicSplines>(this->car_roll),  "car_roll",  pos);
	::CheckBezierCount(count, std::dynamic_pointer_cast<CubicSplines>(this->car_yaw),   "car_yaw",   pos);
}

/**
 * Check whether the number of steps in a bezier spline matches with the expected count.
 * @param index Index in the curve denoting the bezier spline to check.
 * @param steps Expected number of steps in the bezier spline.
 * @param splines Curve that may be contain bezier splines, \c nullptr is that is not the case.
 * @param name Name of the curve to check.
 * @param pos %Position of the track piece in the source file.
 */
static void CheckSplineStepCount(int index, int steps, const std::shared_ptr<CubicSplines> splines, const char *name, const Position &pos)
{
	if (splines == nullptr) return;
	std::shared_ptr<CubicSpline> spline = splines->curve[index];
	if (spline->steps == steps) return;
	fprintf(stderr, "Error at %s: Track piece curve number %d of '%s' has %d steps instead of the expected %d.\n",
			pos.ToString(), index + 1, name, spline->steps, steps);
	::exit(1);
}

/**
 * Check whether the length of the curved splines is the same for the given index number.
 * @param index Index in the curve denoting the bezier spline to check.
 * @param steps Expected number of steps in the bezier spline.
 * @param pos %Position of the track piece in the source file.
 */
void TrackPieceNode::CheckSplineStepCount(int index, int steps, const Position &pos)
{
	::CheckSplineStepCount(index, steps, std::dynamic_pointer_cast<CubicSplines>(this->car_xpos),  "car_xpos",  pos);
	::CheckSplineStepCount(index, steps, std::dynamic_pointer_cast<CubicSplines>(this->car_ypos),  "car_ypos",  pos);
	::CheckSplineStepCount(index, steps, std::dynamic_pointer_cast<CubicSplines>(this->car_zpos),  "car_zpos",  pos);
	::CheckSplineStepCount(index, steps, std::dynamic_pointer_cast<CubicSplines>(this->car_pitch), "car_pitch", pos);
	::CheckSplineStepCount(index, steps, std::dynamic_pointer_cast<CubicSplines>(this->car_roll),  "car_roll",  pos);
	::CheckSplineStepCount(index, steps, std::dynamic_pointer_cast<CubicSplines>(this->car_yaw),   "car_yaw",   pos);
}

/**
 * Compute the length of the track from the #car_xpos, #car_ypos, and #car_zpos curves and setup CubicSpline::first and CubicSpline::last values.
 * @param pos %Position of the track piece in the source file.
 */
void TrackPieceNode::ComputeTrackLength(const Position &pos)
{
	/* Find a spline curve. At least one position should have one. */
	std::shared_ptr<CubicSplines> splines = nullptr;
	if (splines == nullptr) splines = std::dynamic_pointer_cast<CubicSplines>(this->car_xpos);
	if (splines == nullptr) splines = std::dynamic_pointer_cast<CubicSplines>(this->car_ypos);
	if (splines == nullptr) splines = std::dynamic_pointer_cast<CubicSplines>(this->car_zpos);
	if (splines == nullptr) {
		fprintf(stderr, "Error at %s: Track piece must have at least one 'splines' node in its positions.\n", pos.ToString());
		::exit(1);
	}

	/* Check splines count. All splines should have the same number of bezier curves. */
	this->CheckBezierCount(splines->curve.size(), pos);

	double distance = 0.0; // Cumulative distance of this track piece.
	for (int index = 0; index < (int)splines->curve.size(); index++) {
		this->SetStartEnd(index, true, distance * 256); // Set start position of all splines at this index.

		int steps = splines->curve[index]->steps;
		this->CheckSplineStepCount(index, steps, pos); // Check step count for all splines at this index.

		/* Compute distance of this bezier spline. */
		double xp = this->car_xpos->GetValue(index, 0);
		double yp = this->car_ypos->GetValue(index, 0);
		double zp = this->car_zpos->GetValue(index, 0);

		for (int step = 0; step < steps; step++) {
			double xp2 = this->car_xpos->GetValue(index, step);
			double yp2 = this->car_ypos->GetValue(index, step);
			double zp2 = this->car_zpos->GetValue(index, step);
			if (xp2 == xp && yp2 == yp && zp2 == zp) continue;
			distance += sqrt((xp2 - xp) * (xp2 - xp) + (yp2 - yp) * (yp2 - yp) + (zp2 - zp) * (zp2 - zp));
			xp = xp2;
			yp = yp2;
			zp = zp2;
		}

		this->SetStartEnd(index, false, distance * 256); // Set last position of all splines at this index.
	}
	if (distance < 127.5) { // A vertical track of one voxel is 128 pixels high, with 0.5 for rounding errors to prevent false positives.
		fprintf(stderr, "Error at %s: Track piece is too short (should be at least 128 pixels long).\n", pos.ToString());
		::exit(1);
	}

	this->total_distance = distance * 256; // Set total distance of the track piece.
}

/**
 * Get the number of bytes needed for saving a curve entry.
 * @param entry Entry to save.
 * @return Number of bytes needed.
 */
static int GetCarEntrySize(std::shared_ptr<Curve> entry)
{
	if (entry == nullptr) return 1;
	std::shared_ptr<FixedTable> fixed = std::dynamic_pointer_cast<FixedTable>(entry);
	if (fixed != nullptr) return 1 + 2;
	std::shared_ptr<CubicSplines> splines = std::dynamic_pointer_cast<CubicSplines>(entry);
	assert(splines != nullptr && splines->curve.size() < 256);
	return 1 + 1 + splines->curve.size() * 16; // first, last, a, b, c, d
}

/**
 * Rotate the \a abcd array \a rot times, given its type.
 * @param abcd [inout] Array to rotate.
 * @param rot Number of rotations to perform.
 * @param type Type of data (either \c 'x' or \c 'y').
 */
static void RotateArray(int *abcd, int rot, int type)
{
	for (; rot > 0; rot--) {
		if (type == 'x') {
			type = 'y';
			for (int i = 0; i < 4; i++) abcd[i] = (128 - abcd[i]) + 128;
		} else {
			type = 'x';
		}
	}
}

/**
 * Save a curve entry.
 * @param fb Block to save in.
 * @param entry Entry to save.
 * @param rot Number of rotations.
 * @param type of this entry (\c 'x' or \c 'y', or \c '-'.
 */
static void SaveCarEntry(FileBlock *fb, std::shared_ptr<Curve> entry, int rot, int type)
{
	if (entry == nullptr) {
		fb->SaveUInt8(0);
		return;
	}
	std::shared_ptr<FixedTable> fixed = std::dynamic_pointer_cast<FixedTable>(entry);
	if (fixed != nullptr) {
		fb->SaveUInt8(1);
		fb->SaveUInt16(fixed->value);
		return;
	}
	std::shared_ptr<CubicSplines> splines = std::dynamic_pointer_cast<CubicSplines>(entry);
	fb->SaveUInt8(2);
	fb->SaveUInt8(splines->curve.size());
	for (const auto &spline : splines->curve) {
		fb->SaveUInt32(spline->first);
		fb->SaveUInt32(spline->last);
		int abcd[4] = {spline->a, spline->b, spline->c, spline->d};
		if (rot != 0 && type != '-') RotateArray(abcd, rot, type);
		fb->SaveInt16(abcd[0]);
		fb->SaveInt16(abcd[1]);
		fb->SaveInt16(abcd[2]);
		fb->SaveInt16(abcd[3]);
	}
}

/**
 * Write a TRCK game node for all 4 orientations.
 * @param connections Map of connection names to their id.
 * @param fw Output stream to write the blocks to.
 * @param parent_fb Parent file block to write the game block numbers to.
 */
void TrackPieceNode::Write(const std::map<std::string, int> &connections, FileWriter *fw, FileBlock *parent_fb)
{
	for (int rot = 0; rot < 4; rot++) {
		FileBlock *fb = new FileBlock;
		int size = 26 - 12 + 36 * this->track_voxels.size() + 4;
		size += GetCarEntrySize(this->car_xpos) + GetCarEntrySize(this->car_ypos) + GetCarEntrySize(this->car_zpos);
		size += GetCarEntrySize(this->car_pitch) + GetCarEntrySize(this->car_roll) + GetCarEntrySize(this->car_yaw);
		fb->StartSave("TRCK", 5, size);
		fb->SaveUInt8(this->entry->Encode(connections, rot));
		fb->SaveUInt8(this->exit->Encode(connections, rot));
		int nx = this->exit_dx;
		int ny = this->exit_dy;
		RotateXY(&nx, &ny, rot);
		fb->SaveInt8(nx);
		fb->SaveInt8(ny);
		fb->SaveInt8(this->exit_dz);
		fb->SaveInt8(this->speed);
		int platform = this->track_flags & 7;
		if ((platform & 1) != 0) platform = (platform + rot * (1 << 1)) & 7;
		int initial = (this->track_flags >> 3) & 7;
		if ((initial & 1) != 0) initial = (initial + rot * (1 << 1)) & 7;
		int flags = platform | (initial << 3);
		flags |= (this->banking & 3) << 6;
		flags |= (this->slope & 7) << 8;
		flags |= (this->bend & 7) << 11;
		fb->SaveUInt16(flags);
		fb->SaveUInt32(this->cost);
		fb->SaveUInt16(this->track_voxels.size());
		for (auto iter : this->track_voxels) {
			iter->Write(fw, fb, rot);
		}
		fb->SaveUInt32(this->total_distance);
		/* Write x curve. */
		if (rot == 0 || rot == 2) {
			SaveCarEntry(fb, this->car_xpos, rot, 'x');
		} else {
			SaveCarEntry(fb, this->car_ypos, rot, 'y');
		}
		/* Write y curve. */
		if (rot == 0 || rot == 2) {
			SaveCarEntry(fb, this->car_ypos, rot, 'y');
		} else {
			SaveCarEntry(fb, this->car_xpos, rot, 'x');
		}
		SaveCarEntry(fb, this->car_zpos,  0, '-');
		SaveCarEntry(fb, this->car_pitch, 0, '-');
		SaveCarEntry(fb, this->car_roll,  0, '-');
		SaveCarEntry(fb, this->car_yaw,   0, '-');
		fb->CheckEndSave();
		parent_fb->SaveUInt32(fw->AddBlock(fb));
	}
}

RCSTBlock::RCSTBlock() : GameBlock("RCST", 6)
{
}

int RCSTBlock::Write(FileWriter *fw)
{
	/* Collect connection names, and give each a number. */
	std::map<std::string, int> connections;
	for (auto iter : this->track_blocks) {
		iter->UpdateConnectionMap(&connections);
	}

	/* Write the data. */
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 29 - 12 + 4 * 4 * this->track_blocks.size());
	fb->SaveUInt16(this->coaster_type);
	fb->SaveUInt8(this->platform_type);
	fb->SaveUInt8(this->number_trains);
	fb->SaveUInt8(this->number_cars);
	fb->SaveUInt16(this->reliability_max);
	fb->SaveUInt16(this->reliability_decrease_daily);
	fb->SaveUInt16(this->reliability_decrease_monthly);
	fb->SaveUInt32(this->text->Write(fw));
	fb->SaveUInt16(4 * this->track_blocks.size());
	for (auto iter : this->track_blocks) {
		iter->Write(connections, fw, fb);
	}
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

CARSBlock::CARSBlock() : GameBlock("CARS", 3)
{
}

int CARSBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 2 + 2 + 4 + 4 + 2 + 2 + 16 * 16 * 16 * 4 * (1 + this->num_passengers) + 4 * 3);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt16(this->z_height);
	fb->SaveUInt32(this->length);
	fb->SaveUInt32(this->inter_length);
	fb->SaveUInt16(this->num_passengers);
	fb->SaveUInt16(this->num_entrances);
	for (auto &index : this->sprites) fb->SaveUInt32(index->Write(fw));
	for (uint32 index = 0; index < this->num_passengers * 16*16*16; index++) fb->SaveUInt32(this->guest_overlays[index]->Write(fw));
	fb->SaveUInt32(this->recol[0].Encode());
	fb->SaveUInt32(this->recol[1].Encode());
	fb->SaveUInt32(this->recol[2].Encode());
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

CSPLBlock::CSPLBlock() : GameBlock("CSPL", 2)
{
}

int CSPLBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 2 + 1 + 8 * 4);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt8(this->type);
	fb->SaveUInt32(this->ne_sw_back->Write(fw));
	fb->SaveUInt32(this->ne_sw_front->Write(fw));
	fb->SaveUInt32(this->se_nw_back->Write(fw));
	fb->SaveUInt32(this->se_nw_front->Write(fw));
	fb->SaveUInt32(this->sw_ne_back->Write(fw));
	fb->SaveUInt32(this->sw_ne_front->Write(fw));
	fb->SaveUInt32(this->nw_se_back->Write(fw));
	fb->SaveUInt32(this->nw_se_front->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

INFOBlock::INFOBlock() : MetaBlock("INFO", 1)
{
}

int INFOBlock::Write(FileWriter *fw)
{
	int length = this->build.size() + 1;
	length += this->name.size() + 1;
	length += this->uri.size() + 1;
	length += this->website.size() + 1;
	length += this->description.size() + 1;

	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, length);
	fb->SaveBytes((uint8*)this->build.c_str(),       this->build.size());       fb->SaveUInt8(0);
	fb->SaveBytes((uint8*)this->name.c_str(),        this->name.size());        fb->SaveUInt8(0);
	fb->SaveBytes((uint8*)this->uri.c_str(),         this->uri.size());         fb->SaveUInt8(0);
	fb->SaveBytes((uint8*)this->website.c_str(),     this->website.size());     fb->SaveUInt8(0);
	fb->SaveBytes((uint8*)this->description.c_str(), this->description.size()); fb->SaveUInt8(0);
	fb->CheckEndSave();
	return fw->AddBlock(fb);

}

FENCBlock::FENCBlock() : GameBlock("FENC", 2)
{
}

int FENCBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 64 - 12);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt16(this->type);
	fb->SaveUInt32(this->ne_hor->Write(fw));
	fb->SaveUInt32(this->ne_n->Write(fw));
	fb->SaveUInt32(this->ne_e->Write(fw));
	fb->SaveUInt32(this->se_hor->Write(fw));
	fb->SaveUInt32(this->se_e->Write(fw));
	fb->SaveUInt32(this->se_s->Write(fw));
	fb->SaveUInt32(this->sw_hor->Write(fw));
	fb->SaveUInt32(this->sw_s->Write(fw));
	fb->SaveUInt32(this->sw_w->Write(fw));
	fb->SaveUInt32(this->nw_hor->Write(fw));
	fb->SaveUInt32(this->nw_w->Write(fw));
	fb->SaveUInt32(this->nw_n->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

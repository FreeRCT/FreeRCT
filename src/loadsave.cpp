/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file loadsave.cpp Savegame loading and saving code. */

#include "stdafx.h"
#include "dates.h"
#include "random.h"
#include "finances.h"
#include "map.h"

/**
 * Constructor of the loader class.
 * @param fp Input file stream. Use \c nullptr for initialization to default.
 */
Loader::Loader(FILE *fp)
{
	this->fail_msg = nullptr;
	this->blk_name = nullptr;
	this->fp = fp;
	this->cache_count = 0;
}

/**
 * Test whether a block with the given name is being opened.
 * @param name Name of the expected block.
 * @param may_fail Whether it is allowed not to find the expected block.
 * @return Version number of the found block, \c 0 for default initialization, #UINT32_MAX for failing to find the block (only if \a may_fail was set).
 * @note If the block was not found, bytes already read of the block name are pushed back onto the stream.
 */
uint32 Loader::OpenBlock(const char *name, bool may_fail)
{
	assert(strlen(name) == 4);

	if (this->fp == nullptr || this->IsFail()) return 0;

	assert(this->blk_name == nullptr);
	this->blk_name = name;
	int i = 0;
	while (i < 4) {
		uint8 val = this->GetByte();
		if (val != this->blk_name[i]) {
			this->PutByte(val);
			while (i > 0) {
				i--;
				this->PutByte(this->blk_name[i]);
			}
			if (may_fail) return UINT32_MAX;
			this->SetFailMessage("Missing block name");
			return 0;
		}
		i++;
	}

	uint32 version = this->GetLong();
	if (version == 0 || version == UINT32_MAX) {
		this->SetFailMessage("Incorrect version number");
		return 0;
	}
	return version;
}

/** Test whether the current block is closed. */
void Loader::CloseBlock()
{
	if (this->fp == nullptr || this->IsFail()) return;

	assert(this->blk_name != nullptr);
	if (this->GetByte() != this->blk_name[3] || this->GetByte() != this->blk_name[2] ||
			this->GetByte() != this->blk_name[1] || this->GetByte() != this->blk_name[0]) {
		this->SetFailMessage("CloseBlock got unexpected data");
	}
	this->blk_name = nullptr;
}

/**
 * Get the next byte from the stream (or the cache).
 * @return The read next byte.
 */
uint8 Loader::GetByte()
{
	if (this->fp == nullptr || this->IsFail()) return 0;
	
	if (this->cache_count > 0) {
		this->cache_count--;
		return this->cache[this->cache_count];
	}
	int k = getc(this->fp);
	if (k == EOF) {
		this->SetFailMessage("EOF encountered");
		return 0;
	}
	return k;
}

/**
 * Put a byte in the temporary cache.
 * @param val Value to save.
 */
void Loader::PutByte(uint8 val)
{
	if (this->IsFail()) return;
	assert(this->cache_count < (int)lengthof(this->cache));
	this->cache[this->cache_count] = val;
	this->cache_count++;
}

/**
 * Get the next word from the stream (or the cache).
 * @return The read next word.
 */
uint16 Loader::GetWord()
{
	uint16 v = this->GetByte();
	uint16 w = this->GetByte();
	return v | (w << 8);
}

/**
 * Get the next long word from the stream (or the cache).
 * @return The read next long word.
 */
uint32 Loader::GetLong()
{
	uint32 v = this->GetWord();
	uint32 w = this->GetWord();
	return v | (w << 16);
}

/**
 * Get the next long long word from the stream (or the cache).
 * @return The read next long long word.
 */
uint64 Loader::GetLongLong()
{
	uint64 v = this->GetLong();
	uint64 w = this->GetLong();
	return v | (w << 32);
}

/**
 * Denote loading as being failed.
 * @param fail_msg Message to explain what failed. Caller must preserve the message text.
 * @note Message is mostly for internal and debugging use.
 */
void Loader::SetFailMessage(const char *fail_msg)
{
	if (this->IsFail()) return; // Do not overwrite the first message.
	this->fail_msg = fail_msg;
}

/**
 * Get the failure message of the loader.
 * @return Message why loading failed.
 * @note Message is mostly for internal and debugging use.
 */
const char *Loader::GetFailMessage() const
{
	return this->fail_msg;
}

/**
 * Has loading failed?
 * @return Whether loading has failed.
 */
bool Loader::IsFail() const
{
	return this->fail_msg != nullptr;
}

/**
 * Constructor for the saver.
 * @param fp Output file stream to write to.
 */
Saver::Saver(FILE *fp)
{
	this->fp = fp;
	this->blk_name = nullptr;
}

/**
 * Write the start of a block to the output.
 * @param name Name of the block to write.
 * @param version Version number of the block.
 */
void Saver::StartBlock(const char *name, uint32 version)
{
	assert(strlen(name) == 4);
	assert(this->blk_name == nullptr);
	this->blk_name = name;
	for (int i = 0; i < 4; i++) this->PutByte(name[i]);
	assert(version != 0 && version != UINT32_MAX);
	this->PutLong(version);
}

/** Write the end of the block to the output. */
void Saver::EndBlock()
{
	assert(this->blk_name != nullptr);
	for (int i = 3; i >= 0; i--) this->PutByte(this->blk_name[i]);
	this->blk_name = nullptr;
}

/**
 * Write a byte to the output stream.
 * @param val Value to write.
 */
void Saver::PutByte(uint8 val)
{
	putc(val, this->fp);
}

/**
 * Write a word to the output stream.
 * @param val Value to write.
 */
void Saver::PutWord(uint16 val)
{
	this->PutByte(val);
	this->PutByte(val >> 8);
}

/**
 * Write a long word to the output stream.
 * @param val Value to write.
 */
void Saver::PutLong(uint32 val)
{
	this->PutWord(val);
	this->PutWord(val >> 16);
}

/**
 * Write a long long word to the output stream.
 * @param val Value to write.
 */
void Saver::PutLongLong(uint64 val)
{
	this->PutLong(val);
	this->PutLong(val >> 32);
}

/**
 * Load the game elements from the input stream.
 * @param ldr Input stream to load from.
 * @note Order of loading should be the same as in #SaveElements.
 */
static void LoadElements(Loader &ldr)
{
	uint32 version = ldr.OpenBlock("FCTS");
	if (version > 3) ldr.SetFailMessage("Bad file header");
	ldr.CloseBlock();

	Loader reset_loader(nullptr);

	LoadDate(ldr);
	_world.Load((version >= 3) ? ldr : reset_loader);
	Random::Load(ldr);
	_finances_manager.Load((version >= 2) ? ldr : reset_loader);

	if (reset_loader.IsFail()) ldr.SetFailMessage(reset_loader.GetFailMessage());
}

/**
 * Write the game elements to the output stream.
 * @param svr Output stream to write to.
 * @note Order of saving should be the same as in #LoadElements.
 */
static void SaveElements(Saver &svr)
{
	svr.StartBlock("FCTS", 3);
	svr.EndBlock();

	SaveDate(svr);
	_world.Save(svr);
	Random::Save(svr);
	_finances_manager.Save(svr);
}

/**
 * Load a file as saved game. Loading from \c nullptr means initializing to default.
 * @param fname Name of the file to load. Use \c nullptr to initialize to default.
 * @return Whether loading was successful.
 */
bool LoadGame(const char *fname)
{
	FILE *fp;

	if (fname == nullptr) {
		fp = nullptr;
	} else {
		fp = fopen(fname, "rb");
		if (fp == nullptr) return false;
	}
	Loader ldr(fp);
	LoadElements(ldr);
	if (fp != nullptr) fclose(fp);
	if (!ldr.IsFail()) return true;

	Loader reset(nullptr);
	LoadElements(reset); // Loading failed, initialize everything to default.
	return false;
}

/**
 * Save the current game state to file.
 * @param fname Name of the file to write.
 * @return Whether saving was successful.
 */
bool SaveGame(const char *fname)
{
	FILE *fp = fopen(fname, "wb");
	if (fp == nullptr) return false;
	Saver svr(fp);
	SaveElements(svr);
	fclose(fp);
	return true;
}


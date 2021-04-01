/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file loadsave.cpp Savegame loading and saving code. */

#include <cstdarg>

#include "stdafx.h"
#include "dates.h"
#include "random.h"
#include "finances.h"
#include "map.h"
#include "messages.h"
#include "scenery.h"
#include "string_func.h"
#include "person.h"
#include "people.h"

/**
 * Constructor.
 * @param fmt Error message (may use printf-style placeholders).
 */
LoadingError::LoadingError(const char *fmt, ...)
{
	char buffer[1024];
	va_list va;
	va_start(va, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);
	this->message = buffer;
}

/**
 * Retrieve the description of the error.
 * @return The error message.
 */
const char* LoadingError::what() const noexcept
{
	return this->message.c_str();
}

/**
 * Constructor of the loader class.
 * @param fp Input file stream. Use \c nullptr for initialization to default.
 */
Loader::Loader(FILE *fp)
{
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

	if (this->fp == nullptr) return 0;

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
			throw LoadingError("Missing block name for %s", name);
			return 0;
		}
		i++;
	}

	uint32 version = this->GetLong();
	if (version == 0 || version == UINT32_MAX) {
		throw LoadingError("Invalid version number for %s: %u", name, version);
		return 0;
	}
	return version;
}

/** Test whether the current block is closed. */
void Loader::CloseBlock()
{
	if (this->fp == nullptr) return;

	assert(this->blk_name != nullptr);
	if (this->GetByte() != this->blk_name[3] || this->GetByte() != this->blk_name[2] ||
			this->GetByte() != this->blk_name[1] || this->GetByte() != this->blk_name[0]) {
		throw LoadingError("CloseBlock got unexpected data");
	}
	this->blk_name = nullptr;
}

/**
 * Get the next byte from the stream (or the cache).
 * @return The read next byte.
 */
uint8 Loader::GetByte()
{
	if (this->fp == nullptr) return 0;
	
	if (this->cache_count > 0) {
		this->cache_count--;
		return this->cache[this->cache_count];
	}
	int k = getc(this->fp);
	if (k == EOF) {
		throw LoadingError("EOF encountered");
	}
	return k;
}

/**
 * Put a byte in the temporary cache.
 * @param val Value to save.
 */
void Loader::PutByte(uint8 val)
{
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
 * Load a text from the save into memory.
 * @return Utf-8 encoded string. Caller must delete the memory after use.
 */
uint8 *Loader::GetText()
{
	uint32 length = this->GetLong();
	if (length == 0) return nullptr;

	uint32 *cpoints = new uint32[length];

	for (uint32 i = 0; i < length; i++) cpoints[i] = this->GetLong();

	size_t enc_length = 0;
	for (uint32 i = 0; i < length; i++) enc_length += EncodeUtf8Char(cpoints[i], nullptr);
	uint8 *txt = new uint8[enc_length + 1];
	uint8 *p = txt;
	for (uint32 i = 0; i < length; i++) {
		size_t len = EncodeUtf8Char(cpoints[i], p);
		p += len;
	}
	txt[enc_length] = '\0';
	delete[] cpoints;
	return txt;
}

/**
 * Can be called while loading to notify the %Loader that loading failed because a block has an unsupported version number.
 * @param name Name of the code path where the error occured.
 * @param saved_version Version in the savegame.
 * @param current_version Most recent currently supported version.
 */
void Loader::version_mismatch(const char* name, uint saved_version, uint current_version)
{
	throw LoadingError("Version mismatch in %s block: Saved version is %u, supported version is %u", name, saved_version, current_version);
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
 * Save an utf-8 string, \a length is optional.
 * @param str String to save.
 * @param length Number of bytes in the string if specified, else negative.
 */
void Saver::PutText(const uint8 *str, int length)
{
	if (str == nullptr) str = (const uint8 *)"";

	/* Get size of the string in code points. */
	uint32 count = 0;
	const uint8 *p = str;
	size_t size = (length >= 0) ? static_cast<size_t>(length) : 4; // 4 is max utf-8 character length.
	for (;;) {
		uint32 cpoint;
		int len = DecodeUtf8Char(p, size, &cpoint);
		if (len == 0 || cpoint == 0) break;
		p += len;
		if (length >= 0) size -= len;
		count++;
	}

	this->PutLong(count);
	size = (length >= 0) ? static_cast<size_t>(length) : 4; // 4 is max utf-8 character length.
	while (count > 0) {
		uint32 cpoint;
		int len = DecodeUtf8Char(str, size, &cpoint);
		if (len == 0 || cpoint == 0) break;
		str += len;
		if (length >= 0) size -= len;
		this->PutLong(cpoint);
		count--;
	}
	assert(count == 0);
}

static const uint32 CURRENT_VERSION_FCTS = 9;   ///< Currently supported version of the FCTS block.

/**
 * Load the game elements from the input stream.
 * @param ldr Input stream to load from.
 * @note Order of loading should be the same as in #SaveElements.
 */
static void LoadElements(Loader &ldr)
{
	uint32 version = ldr.OpenBlock("FCTS");
	if (version > CURRENT_VERSION_FCTS) ldr.version_mismatch("FCTS", version, CURRENT_VERSION_FCTS);
	ldr.CloseBlock();

	Loader reset_loader(nullptr);

	LoadDate(ldr);
	_world.Load((version >= 3) ? ldr : reset_loader);
	Random::Load(ldr);
	_finances_manager.Load((version >= 2) ? ldr : reset_loader);
	_weather.Load((version >= 4) ? ldr : reset_loader);
	_rides_manager.Load((version >= 6) ? ldr : reset_loader);
	_scenery.Load((version >= 9) ? ldr : reset_loader);
	_guests.Load((version >= 5) ? ldr : reset_loader);
	_staff.Load((version >= 7) ? ldr : reset_loader);
	_inbox.Load((version >= 8) ? ldr : reset_loader);
}

/**
 * Write the game elements to the output stream.
 * @param svr Output stream to write to.
 * @note Order of saving should be the same as in #LoadElements.
 */
static void SaveElements(Saver &svr)
{
	svr.StartBlock("FCTS", CURRENT_VERSION_FCTS);
	svr.EndBlock();

	SaveDate(svr);
	_world.Save(svr);
	Random::Save(svr);
	_finances_manager.Save(svr);
	_weather.Save(svr);
	_rides_manager.Save(svr);
	_scenery.Save(svr);
	_guests.Save(svr);
	_staff.Save(svr);
	_inbox.Save(svr);
}

/**
 * Load a file as saved game. Loading from \c nullptr means initializing to default.
 * @param fname Name of the file to load. Use \c nullptr to initialize to default.
 * @return Whether loading was successful.
 */
bool LoadGameFile(const char *fname)
{
	try {
		FILE *fp = nullptr;
		if (fname != nullptr) {
			fp = fopen(fname, "rb");
			if (fp == nullptr) throw LoadingError("Cannot open file '%s' for reading", fname);
		}

		Loader ldr(fp);
		LoadElements(ldr);

		if (fp != nullptr) fclose(fp);
		return true;
	} catch (const std::exception &e) {
		if (fname != nullptr) {
			printf("ERROR: Loading '%s' failed: %s\n", fname, e.what());
			LoadGameFile(nullptr);
			return false;
		}

		printf("FATAL ERROR: The reset loader failed to default-initialize the game!\n");
		printf("This should not happen. Please consider submiting a bug report.\n");
		printf("FreeRCT will terminate now.\n");
		::exit(1);
	}
}

/**
 * Save the current game state to file.
 * @param fname Name of the file to write.
 * @return Whether saving was successful.
 */
bool SaveGameFile(const char *fname)
{
	FILE *fp = fopen(fname, "wb");
	if (fp == nullptr) return false;
	Saver svr(fp);
	SaveElements(svr);
	fclose(fp);
	return true;
}


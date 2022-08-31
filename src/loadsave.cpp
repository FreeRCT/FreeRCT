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
#include "messages.h"
#include "scenery.h"
#include "string_func.h"
#include "person.h"
#include "people.h"
#include "gamelevel.h"
#include "gameobserver.h"
#include "rev.h"

/** Whether savegame files should automatically be resaved after loading. */
bool _automatically_resave_files = false;

/**
 * Constructor of the loader class.
 * @param file Input file stream. Use \c nullptr for initialization to default.
 */
Loader::Loader(FILE *file) : fp(file), cache_count(0)
{
}

/**
 * Test whether a pattern with the given name is being opened.
 * @param name Name of the expected pattern.
 * @param may_fail Whether it is allowed not to find the expected pattern.
 * @return Version number of the found pattern, \c 0 for default initialization, #UINT32_MAX for failing to find the pattern (only if \a may_fail was set).
 * @note If the pattern was not found, bytes already read of the pattern name are pushed back onto the stream.
 */
uint32 Loader::OpenPattern(const char *name, bool may_fail)
{
	assert(strlen(name) == 4);
	this->pattern_names.push_back(name);
	if (this->fp == nullptr) return 0;

	int i = 0;
	while (i < 4) {
		uint8 val = this->GetByte();
		if (val != name[i]) {
			this->PutByte(val);
			while (i > 0) {
				i--;
				this->PutByte(name[i]);
			}
			if (may_fail) return UINT32_MAX;
			throw LoadingError("Missing pattern name for %s", name);
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

/** Test whether the current pattern is closed. */
void Loader::ClosePattern()
{
	if (this->fp == nullptr) return;
	assert(!this->pattern_names.empty());
	const std::string &blk_name = this->pattern_names.back();
	for (int i = 0; i < static_cast<int>(blk_name.size()); ++i) {
		if (this->GetByte() != blk_name.at(3 - i)) {
			throw LoadingError("ClosePattern (%s) got unexpected data", blk_name.c_str());
		}
	}
	this->pattern_names.pop_back();
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
 * @return Utf-8 encoded string.
 */
std::string Loader::GetText()
{
	uint32 length = this->GetLong();
	if (length == 0) return std::string();

	std::unique_ptr<uint32[]> cpoints(new uint32[length]);

	for (uint32 i = 0; i < length; i++) cpoints[i] = this->GetLong();

	size_t enc_length = 0;
	for (uint32 i = 0; i < length; i++) enc_length += EncodeUtf8Char(cpoints[i], nullptr);
	std::unique_ptr<char[]> txt(new char[enc_length + 1]);
	char *p = txt.get();
	for (uint32 i = 0; i < length; i++) {
		size_t len = EncodeUtf8Char(cpoints[i], p);
		p += len;
	}
	txt[enc_length] = '\0';
	return txt.get();
}

/**
 * Can be called while loading to notify the %Loader that loading failed
 * because the current pattern has an unsupported version number.
 * @param saved_version Version in the savegame.
 * @param current_version Most recent currently supported version.
 */
void Loader::VersionMismatch(uint saved_version, uint current_version)
{
	assert(!this->pattern_names.empty());
	throw LoadingError("Version mismatch in %s pattern: Saved version is %u, supported version is %u",
			this->pattern_names.back().c_str(), saved_version, current_version);
}

/**
 * Constructor for the saver.
 * @param file Output file stream to write to.
 */
Saver::Saver(FILE *file) : fp(file)
{
}

/** Checks that no patterns are currently open. */
void Saver::CheckNoOpenPattern() const
{
	if (!this->pattern_names.empty()) {
		throw LoadingError("Saver still has %d open pattern(s) (last is %s)",
				static_cast<int>(this->pattern_names.size()), this->pattern_names.back().c_str());
	}
}

/**
 * Write the start of a pattern to the output.
 * @param name Name of the pattern to write.
 * @param version Version number of the pattern.
 */
void Saver::StartPattern(const char *name, uint32 version)
{
	assert(strlen(name) == 4);
	assert(version != 0 && version != UINT32_MAX);
	for (int i = 0; i < 4; i++) this->PutByte(name[i]);
	this->PutLong(version);
	this->pattern_names.push_back(name);
}

/** Write the end of the pattern to the output. */
void Saver::EndPattern()
{
	assert(!this->pattern_names.empty());
	const char *blk_name = this->pattern_names.back().c_str();
	for (int i = 3; i >= 0; i--) this->PutByte(blk_name[i]);
	this->pattern_names.pop_back();
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
void Saver::PutText(const std::string &str, int length)
{
	/* Get size of the string in code points. */
	uint32 count = 0;
	const char *p = str.c_str();
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
	p = str.c_str();
	while (count > 0) {
		uint32 cpoint;
		int len = DecodeUtf8Char(p, size, &cpoint);
		if (len == 0 || cpoint == 0) break;
		p += len;
		if (length >= 0) size -= len;
		this->PutLong(cpoint);
		count--;
	}
	assert(count == 0);
}

/* When making any changes to saveloading code, don't forget to update the file 'doc/savegame.rst'! */

static const uint32 CURRENT_VERSION_FCTS = 12;  ///< Currently supported version of the FCTS pattern.

/**
 * Load basic information from the start of a savegame file.
 * @param ldr Input stream to load from.
 * @return Basic information.
 */
PreloadData Preload(Loader &ldr)
{
	uint32 version = ldr.OpenPattern("FCTS");
	if (version != 0 && (version < 10 || version > CURRENT_VERSION_FCTS)) ldr.VersionMismatch(version, CURRENT_VERSION_FCTS);

	PreloadData result;
	result.scenario.reset(new Scenario);
	result.fcts_version = version;

	if (version >= 11) {
		result.timestamp = ldr.GetLongLong();
		result.revision = ldr.GetText();
		if (version >= 12) {
			result.scenario->Load(ldr);
		} else {
			result.scenario->name = ldr.GetText();
		}
	} else {
		result.timestamp = 0;
		result.revision = "?";
		result.scenario->name = _language.GetSgText(GUI_NOT_AVAILABLE);
	}

	ldr.ClosePattern();
	result.load_success = true;
	return result;
}

/**
 * Load the game elements from the input stream.
 * @param ldr Input stream to load from.
 * @param preload Result of the preload step.
 * @note Order of loading should be the same as in #SaveElements.
 */
static void LoadElements(Loader &ldr, const PreloadData &preload)
{
	_scenario = *preload.scenario;
	LoadDate(ldr);
	_world.Load(ldr);
	_finances_manager.Load(ldr);
	_weather.Load(ldr);
	if (preload.fcts_version >= 12) {
		_game_observer.Load(ldr);
	} else {
		_game_observer.Initialize();
	}
	_rides_manager.Load(ldr);
	_scenery.Load(ldr);
	_guests.Load(ldr);
	_staff.Load(ldr);
	_inbox.Load(ldr);
	Random::Load(ldr);
}

/**
 * Write the game elements to the output stream.
 * @param svr Output stream to write to.
 * @note Order of saving should be the same as in #LoadElements.
 */
static void SaveElements(Saver &svr)
{
	svr.StartPattern("FCTS", CURRENT_VERSION_FCTS);
	svr.PutLongLong(std::time(nullptr));
	svr.PutText(_freerct_revision);
	_scenario.Save(svr);
	svr.EndPattern();

	SaveDate(svr);
	_world.Save(svr);
	_finances_manager.Save(svr);
	_weather.Save(svr);
	_game_observer.Save(svr);
	_rides_manager.Save(svr);
	_scenery.Save(svr);
	_guests.Save(svr);
	_staff.Save(svr);
	_inbox.Save(svr);
	Random::Save(svr);

	svr.CheckNoOpenPattern();
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
		PreloadData pd = Preload(ldr);
		LoadElements(ldr, pd);

		if (fp != nullptr) {
			fclose(fp);
			if (_automatically_resave_files) SaveGameFile(fname);
		}
		return true;
	} catch (const LoadingError &e) {
		if (fname != nullptr) {
			printf("ERROR: Loading '%s' failed: %s\n", fname, e.what());
			LoadGameFile(nullptr);
			return false;
		}

		error("FATAL ERROR: The reset loader failed to default-initialize the game!\n"
				"This should not happen. Please consider submitting a bug report.\n"
				"Error message: %s\n"
				"FreeRCT will terminate now.\n", e.what());
	}
}

/**
 * Load basic data from a savegame file.
 * @param fname Name of the file to load.
 * @return Basic information about the savegame.
 * @note Check the return value's #load_success attribute to see whether preloading was successful.
 */
PreloadData PreloadGameFile(const char *fname)
{
	PreloadData result;
	if (fname == nullptr) return result;

	try {
		FILE *fp = fopen(fname, "rb");
		if (fp == nullptr) return result;
		Loader ldr(fp);
		result = Preload(ldr);
		fclose(fp);
	} catch (const LoadingError&) {
		result.load_success = false;
	}

	result.filename = fname;
	result.filename = result.filename.substr(result.filename.find_last_of(DIR_SEP) + 1);
	return result;
}

/**
 * Save the current game state to file.
 * @param fname Name of the file to write.
 * @return Whether saving was successful.
 * @todo Show an error message if this fails.
 */
bool SaveGameFile(const char *fname)
{
	FILE *fp = fopen(fname, "wb");
	if (fp == nullptr) return false;

	Saver svr(fp);
	SaveElements(svr);
	fclose(fp);

#ifdef WEBASSEMBLY
	printf("WEBASSEMBLY: Game saved to %s\n", fname);
#endif

	return true;
}


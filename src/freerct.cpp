/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file freerct.cpp Main program. */

#include <cstdarg>
#include "stdafx.h"
#include "freerct.h"
#include "video.h"
#include "viewport.h"
#include "rcdfile.h"
#include "sprite_data.h"
#include "sprite_store.h"
#include "window.h"
#include "config_reader.h"
#include "language.h"
#include "getoptdata.h"
#include "fileio.h"
#include "gamecontrol.h"
#include "ride_type.h"
#include "string_func.h"
#include "rev.h"

#ifdef WEBASSEMBLY
#include <emscripten.h>
#endif

GameControl _game_control; ///< Game controller.

/**
 * Error handling for fatal non-user errors.
 * @param str the string to print.
 * @note \b Never returns.
 */
void error(const char *str, ...)
{
	va_list va;

	va_start(va, str);
	vfprintf(stderr, str, va);
	va_end(va);

	abort();
}

/** Command-line options of the program. */
static const OptionData _options[] = {
	GETOPT_NOVAL('h', "--help"),
	GETOPT_NOVAL('v', "--version"),
	GETOPT_VALUE('l', "--load"),
	GETOPT_NOVAL('r', "--resave"),
	GETOPT_VALUE('a', "--language"),
	GETOPT_VALUE('i', "--installdir"),
	GETOPT_VALUE('u', "--userdatadir"),
	GETOPT_END()
};

/** Output command-line help. */
static void PrintUsage()
{
	printf("Usage: freerct [options]\n");
	printf("Options:\n");
	printf("  -h, --help             Display this help text and exit.\n");
	printf("  -v, --version          Display version and build info and exit.\n");
	printf("  -l, --load FILE        Load game from specified file.\n");
	printf("  -r, --resave           Automatically resave games after loading.\n");
	printf("  -a, --language LANG    Use the specified language.\n");
	printf("  -i, --installdir DIR   Use the specified installation directory.\n");
	printf("  -u, --userdatadir DIR  Use the specified user data directory.\n");

	printf("\nValid languages are:\n   ");
	int length = 0;
	for (int i = 0; i < LANGUAGE_COUNT; ++i) {
		length += strlen(_all_languages[i].name) + 1;
		if (length > 50) {  // Linewrap after an arbitrary number of characters.
			printf("\n   ");
			length = strlen(_all_languages[i].name);
		}
		printf(" %s", _all_languages[i].name);
	}
	printf("\n");
}

/** Output command-line version info. */
static void PrintVersion()
{
	printf("FreeRCT\n\n");

	printf("Version                : %s\n",   _freerct_revision);
	printf("Build ID               : %s\n",   _freerct_build_date);
	printf("Installation directory : %s\n",   freerct_install_prefix().c_str());
	printf("User data directory    : %s\n\n", freerct_userdata_prefix().c_str());

	printf("Homepage: https://freerct.net\n\n");

	printf(
		"FreeRCT is free software; you can redistribute it and/or\n"
		"modify it under the terms of the GNU General Public\n"
		"License as published by the Free Software Foundation,\n"
		"version 2. FreeRCT is distributed in the hope that it\n"
		"will be useful, but WITHOUT ANY WARRANTY; without even\n"
		"the implied warranty of MERCHANTABILITY or FITNESS FOR A\n"
		"PARTICULAR PURPOSE. See the GNU General Public License\n"
		"for more details. You should have received a copy of the\n"
		"GNU General Public License along with FreeRCT. If not,\n"
		"see <http://www.gnu.org/licenses/>\n");
}

/**
 * Look for savegames and a config file in various locations we used before we implemented
 * the XDG basedir specification, and move our findings to the new-style directories.
 * Does not overwrite existing files.
 * @todo Can be removed some time in the future (after v0.1) when nobody should have any unmigrated files anymore.
 */
static void MigrateOldFiles()
{
	const std::string &userdata = freerct_userdata_prefix();
	const std::string &homedir = GetUserHomeDirectory();

	for (const std::string &old_directory :
			{homedir + "" + DIR_SEP + ".config" + DIR_SEP + "freerct",
			homedir + DIR_SEP + ".local" + DIR_SEP + "share" + DIR_SEP + "freerct",
			homedir + DIR_SEP + ".freerct"}) {
		std::unique_ptr<DirectoryReader> dr(MakeDirectoryReader());
		dr->OpenPath(old_directory.c_str());
		for (;;) {
			const char *filename = dr->NextEntry();
			if (filename == nullptr) break;

			std::string name(filename);
			name = name.substr(name.find_last_of(DIR_SEP) + 1);

			std::string destination;
			if (name == "freerct.cfg") {
				destination = userdata;
				destination += DIR_SEP;
				destination += name;
			} else if (name.size() > 4 && name.compare(name.size() - 4, 4, ".fct") == 0) {
				destination = SavegameDirectory();
				destination += name;
			} else {
				continue;
			}

			if (PathIsFile(destination.c_str())) continue;  // Do not overwrite existing files.
			printf("Migrating file from %s to %s\n", filename, destination.c_str());
			CopyBinaryFile(filename, destination.c_str());
		}
		dr->ClosePath();
	}
}

/**
 * Main entry point of our FreeRCT game.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return The exit code of the program.
 */
int freerct_main(int argc, char **argv)
{
	GetOptData opt_data(argc - 1, argv + 1, _options);

	int opt_id;
	std::string file_name;
	std::string preferred_language;
	do {
		opt_id = opt_data.GetOpt();
		switch (opt_id) {
			case 'h':
				PrintUsage();
				return 0;
			case 'v':
				PrintVersion();
				return 0;
			case 'i':
				OverrideInstallPrefix(opt_data.opt);
				break;
			case 'u':
				OverrideUserdataPrefix(opt_data.opt);
				break;
			case 'a':
				preferred_language = opt_data.opt;
				break;
			case 'r':
				_automatically_resave_files = true;
				break;
			case 'l':
				if (opt_data.opt != nullptr) file_name = opt_data.opt;
				break;

			case -1:
				break;

			default:
				/* -2 or some other weird thing happened. */
				fprintf(stderr, "ERROR while processing the command-line\n");
				return 1;
		}
	} while (opt_id != -1);

	/* Create the data directories on startup if they did not exist yet. */
	MakeDirectory(SavegameDirectory());
	MakeDirectory(TrackDesignDirectory());

	/* Scan for savegames and config files in outdated locations. */
	MigrateOldFiles();

	/* Load RCD files. */
	InitImageStorage();
	_rcd_collection.ScanDirectories();
	_sprite_manager.LoadRcdFiles();
	_rides_manager.LoadDesigns();

	InitLanguage();

	if (!_gui_sprites.HasSufficientGraphics()) {
		fprintf(stderr, "Insufficient graphics loaded.\n");
		return 1;
	}

	std::string cfg_file_path = freerct_userdata_prefix();
	cfg_file_path += DIR_SEP;
	cfg_file_path += "freerct.cfg";
	ConfigFile cfg_file(cfg_file_path);

	std::string font_path = cfg_file.GetValue("font", "medium-path");
	int font_size = cfg_file.GetNum("font", "medium-size");
	if (cfg_file.GetNum("saveloading", "auto-resave") > 0) _automatically_resave_files = true;

	{
		int autosaves = cfg_file.GetNum("saveloading", "max_autosaves");
		if (autosaves >= 0) _max_autosaves = autosaves;
	}

	/* Use default values if no font has been set. */
	if (font_path.empty()) font_path = FindDataFile(std::string("data") + DIR_SEP + "font" + DIR_SEP + "FreeSans.ttf");
	if (font_size < 1) font_size = 15;

	/* Overwrite the default language settings if the user specified a custom language on the command line or in the config file. */
	bool language_set = false;
	if (!preferred_language.empty()) {
		int index = GetLanguageIndex(preferred_language);
		if (index < 0) {
			fprintf(stderr, "The language '%s' set on the command line is not known.\n", preferred_language.c_str());
			preferred_language = GetSimilarLanguage(preferred_language);
			if (!preferred_language.empty()) fprintf(stderr, "Did you perhaps mean '%s'?\n", preferred_language.c_str());
			fprintf(stderr, "Type 'freerct --help' for a list of all supported languages.\n");
		} else {
			_current_language = index;
			language_set = true;
		}
	}
	if (!language_set) {
		preferred_language = cfg_file.GetValue("language", "language");  // The pointer is owned by cfg_file.
		if (!preferred_language.empty()) {
			int index = GetLanguageIndex(preferred_language);
			if (index < 0) {
				fprintf(stderr, "The language '%s' set in the configuration file (freerct.cfg) is not known.\n", preferred_language.c_str());
				preferred_language = GetSimilarLanguage(preferred_language);
				if (!preferred_language.empty()) fprintf(stderr, "Did you perhaps mean '%s'?\n", preferred_language.c_str());
				fprintf(stderr, "Type 'freerct --help' for a list of all supported languages.\n");
			} else {
				_current_language = index;
			}
		}
	}

	/* Initialize video. */
	_video.Initialize(font_path, font_size);

	_game_control.Initialize(file_name);

	/* Loops until told not to. */
#ifdef WEBASSEMBLY
	emscripten_set_main_loop(VideoSystem::MainLoopCycle, 0 /* set FPS automatically */, 1 /* repeat as endless loop */);
#else
	_video.MainLoop();
#endif

	_game_control.Uninitialize();

	UninitLanguage();
	DestroyImageStorage();
	_video.Shutdown();
	return 0;
}

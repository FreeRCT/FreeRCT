/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file freerct.cpp Main program. */

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
#include "string_func.h"
#include "rev.h"

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
	GETOPT_VALUE('a', "--language"),
	GETOPT_NOVAL('r', "--resave"),
	GETOPT_END()
};

/** Output command-line help. */
static void PrintUsage()
{
	printf("Usage: freerct [options]\n");
	printf("Options:\n");
	printf("  -h, --help           Display this help text and exit.\n");
	printf("  -v, --version        Display version and build info and exit.\n");
	printf("  -l, --load [file]    Load game from specified file.\n");
	printf("  -r, --resave         Automatically resave games after loading.\n");
	printf("  -a, --language lang  Use the specified language.\n");

	printf("\nValid languages are:\n   ");
	int length = 0;
	for (int i = 0; i < LANGUAGE_COUNT; ++i) {
		length += strlen(_lang_names[i]) + 1;
		if (length > 50) {  // Linewrap after an arbitrary number of characters.
			printf("\n   ");
			length = strlen(_lang_names[i]);
		}
		printf(" %s", _lang_names[i]);
	}
	printf("\n");
}

/** Output command-line version info. */
static void PrintVersion()
{
	printf("FreeRCT\n\n");

	printf("Version               : %s\n",   _freerct_revision);
	printf("Build ID              : %s\n",   _freerct_build_date);
	printf("Installation directory: %s\n",   _freerct_install_prefix);
	printf("User data directory   : %s\n\n", freerct_userdata_prefix());

	printf("Homepage: https://github.com/FreeRCT/FreeRCT\n\n");

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
 * Main entry point of our FreeRCT game.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return The exit code of the program.
 */
int freerct_main(int argc, char **argv)
{
	GetOptData opt_data(argc - 1, argv + 1, _options);

	int opt_id;
	const char *file_name = nullptr;
	const char *preferred_language = nullptr;
	do {
		opt_id = opt_data.GetOpt();
		switch (opt_id) {
			case 'h':
				PrintUsage();
				return 0;
			case 'v':
				PrintVersion();
				return 0;
			case 'a':
				preferred_language = StrDup(opt_data.opt);
				break;
			case 'r':
				_automatically_resave_files = true;
				break;
			case 'l':
				if (opt_data.opt != nullptr) {
					file_name = StrDup(opt_data.opt);
				}
				break;

			case -1:
				break;

			default:
				/* -2 or some other weird thing happened. */
				fprintf(stderr, "ERROR while processing the command-line\n");
				return 1;
		}
	} while (opt_id != -1);

	/* Create the data directory on startup if it did not exist yet. */
	MakeDirectory(freerct_userdata_prefix());

	ConfigFile cfg_file;

	/* Load RCD files. */
	InitImageStorage();
	_rcd_collection.ScanDirectories();
	_sprite_manager.LoadRcdFiles();

	InitLanguage();

	if (!_gui_sprites.HasSufficientGraphics()) {
		fprintf(stderr, "Insufficient graphics loaded.\n");
		return 1;
	}

	{
		std::unique_ptr<DirectoryReader> dr(MakeDirectoryReader());
		std::string path = freerct_userdata_prefix();
		path += dr->dir_sep;
		path += "freerct.cfg";
		cfg_file.Load(path.c_str());
	}

	const char *font_path = cfg_file.GetValue("font", "medium-path");
	int font_size = cfg_file.GetNum("font", "medium-size");
	/* Use default values if no font has been set. */
	std::string font_path_fallback;
	if (font_path == nullptr) {
		font_path_fallback = FindDataFile("data/font/Ubuntu-L.ttf");
		font_path = font_path_fallback.c_str();
	}
	if (font_size < 1) font_size = 15;
	if (cfg_file.GetNum("saveloading", "auto-resave") > 0) _automatically_resave_files = true;

	/* Overwrite the default language settings if the user specified a custom language on the command line or in the config file. */
	bool language_set = false;
	if (preferred_language != nullptr) {
		int index = GetLanguageIndex(preferred_language);
		if (index < 0) {
			fprintf(stderr, "The language '%s' set on the command line is not known.\n", preferred_language);
			const char *similar = GetSimilarLanguage(preferred_language);
			if (similar != nullptr) fprintf(stderr, "Did you perhaps mean '%s'?\n", similar);
			fprintf(stderr, "Type 'freerct --help' for a list of all supported languages.\n");
		} else {
			_current_language = index;
			language_set = true;
		}
		delete[] preferred_language;
		preferred_language = nullptr;
	}
	if (!language_set) {
		preferred_language = cfg_file.GetValue("language", "language");
		if (preferred_language != nullptr) {
			int index = GetLanguageIndex(preferred_language);
			if (index < 0) {
				fprintf(stderr, "The language '%s' set in the configuration file (freerct.cfg) is not known.\n", preferred_language);
				const char *similar = GetSimilarLanguage(preferred_language);
				if (similar != nullptr) fprintf(stderr, "Did you perhaps mean '%s'?\n", similar);
				fprintf(stderr, "Type 'freerct --help' for a list of all supported languages.\n");
			} else {
				_current_language = index;
			}
		}
	}

	/* Initialize video. */
	std::string err = _video.Initialize(font_path, font_size);
	if (!err.empty()) {
		fprintf(stderr, "Failed to initialize window or the font (%s), aborting\n", err.c_str());
		return 1;
	}

	_game_control.Initialize(file_name);

	delete[] file_name;

	/* Loops until told not to. */
	_video.MainLoop();

	_game_control.Uninitialize();

	UninitLanguage();
	DestroyImageStorage();
	_video.Shutdown();
	return 0;
}

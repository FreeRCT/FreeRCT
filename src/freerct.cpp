/* $Id$ */

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

void InitMouseModes();

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
	GETOPT_END()
};

/** Output command-line help. */
static void PrintUsage()
{
	printf("Usage: freerct [options]\n");
	printf("Options:\n");
	printf("  -h, --help     Display this help text and exit\n");
}

/** Show that there are missing sprites. */
void ShowGraphicsErrorMessage()
{
	/* Enough sprites are available for displaying an error message,
	 * as this was checked in GuiSprites::HasSufficientGraphics. */
	ShowErrorMessage(GUI_ERROR_MESSAGE_SPRITE);
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
	do {
		opt_id = opt_data.GetOpt();
		switch (opt_id) {
			case 'h':
				PrintUsage();
				return 0;

			case -1:
				break;

			default:
				/* -2 or some other weird thing happened. */
				fprintf(stderr, "ERROR while processing the command-line\n");
				return 1;
		}
	} while (opt_id != -1);

	ConfigFile cfg_file;

	ChangeWorkingDirectoryToExecutable(argv[0]);

	/* Load RCD files. */
	InitImageStorage();
	_rcd_collection.ScanDirectories();
	_sprite_manager.LoadRcdFiles();

	InitLanguage();

	if (!_gui_sprites.HasSufficientGraphics()) {
		fprintf(stderr, "Insufficient graphics loaded.\n");
		return 1;
	}

	cfg_file.Load("freerct.cfg");
	const char *font_path = cfg_file.GetValue("font", "medium-path");
	int font_size = cfg_file.GetNum("font", "medium-size");
	if (font_path == nullptr || *font_path == '\0' || font_size == -1) {
		fprintf(stderr, "Failed to find font settings. Did you make a 'freerct.cfg' file next to the 'freerct' program?\n");
		fprintf(stderr, "Example content (you may need to change the path and.or the size):\n"
		                "[font]\n"
		                "medium-size = 12\n"
		                "medium-path = /usr/share/fonts/gnu-free/FreeSans.ttf\n");
		return 1;
	}

	/* Initialize video. */
	std::string err = _video.Initialize(font_path, font_size);
	if (!err.empty()) {
		fprintf(stderr, "Failed to initialize window or the font (%s), aborting\n", err.c_str());
		return 1;
	}

	InitMouseModes();

	StartNewGame();

	/* Loops until told not to. */
	_video.MainLoop();

	/* Closing down. */
	ShutdownGame();
	UninitLanguage();
	DestroyImageStorage();
	_video.Shutdown();
	return 0;
}

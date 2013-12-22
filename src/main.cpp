/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file main.cpp Main program. */

#include "stdafx.h"
#include "freerct.h"
#include "video.h"
#include "map.h"
#include "viewport.h"
#include "sprite_store.h"
#include "window.h"
#include "config_reader.h"
#include "language.h"
#include "dates.h"
#include "ride_type.h"
#include "person.h"
#include "people.h"
#include "finances.h"
#include "gamelevel.h"
#include "getoptdata.h"

static bool _finish; ///< Finish execution of the program.

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

/** End the program. */
void QuitProgram()
{
	_finish = true;
}

/**
 * Callback from the SDL timer.
 * @param interval Timer interval.
 * @param param Dummy parameter.
 * @return New interval.
 */
static uint32 NextFrame(uint32 interval, void *param)
{
	SDL_Event event;

	/* Push a 'UserEvent' onto the queue to denote time passage. */
	event.type = SDL_USEREVENT; // Other fields are "don't care".
	SDL_PushEvent(&event);

	return interval;
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

/**
 * Main entry point.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return The exit code of the program.
 */
int main(int argc, char **argv)
{
	GetOptData opt_data(argc - 1, argv + 1, _options);

	int opt_id;
	do {
		opt_id = opt_data.GetOpt();
		switch (opt_id) {
			case 'h':
				PrintUsage();
				exit(0);

			case -1:
				break;

			default:
				/* -2 or some other weird thing happened. */
				fprintf(stderr, "ERROR while processing the command-line\n");
				exit(1);
		}
	} while (opt_id != -1);

	VideoSystem vid;
	ConfigFile cfg_file;

	/* Load RCD files. */
	if (!_sprite_manager.LoadRcdFiles()) exit(1);

	InitLanguage();

	if (!_gui_sprites.HasSufficientGraphics()) {
		fprintf(stderr, "Insufficient graphics loaded.\n");
		exit(1);
	}

	cfg_file.Load("freerct.cfg");
	const char *font_path = cfg_file.GetValue("font", "medium-path");
	const char *font_size_text = cfg_file.GetValue("font", "medium-size");
	if (font_path == nullptr || *font_path == '\0' || font_size_text == nullptr || *font_size_text == '\0') {
		fprintf(stderr, "Failed to find font settings. Did you make a 'freerct.cfg' file next to the 'freerct' program?\n");
		fprintf(stderr, "Example content (you may need to change the path and.or the size):\n"
		                "[font]\n"
		                "medium-size = 12\n"
		                "medium-path = /usr/share/fonts/gnu-free/FreeSans.ttf\n");
		exit(1);
	}

	/* Initialize video. */
	if (!vid.Initialize(font_path, atoi(font_size_text))) {
		fprintf(stderr, "Failed to initialize window or the font, aborting\n");
		exit(1);
	}
	vid.SetPalette();
	_video = &vid;

	_finish = false;

	InitMouseModes();

	_world.SetWorldSize(20, 21);
	_world.MakeFlatWorld(8);
	_world.SetTileOwnerRect(2, 2, 16, 15, OWN_PARK);
	_world.SetTileOwnerRect(8, 0, 4, 2, OWN_PARK); // Allow building path to map edge in north west.
	_world.SetTileOwnerRect(2, 18, 16, 2, OWN_FOR_SALE);
	_finances_manager.SetScenario(_scenario);
	_guests.Initialize();

	ShowToolbar();
	ShowBottomToolbar();
	Viewport *w = ShowMainDisplay();

	bool missing_sprites_check = false;

	SDL_TimerID timer_id = SDL_AddTimer(30, &NextFrame, nullptr);

	while (!_finish) {
		/* For every frame do... */
		_manager.Tick();
		_guests.DoTick();
		DateOnTick();
		_guests.OnAnimate(30); // Fixed rate animation.
		_rides_manager.OnAnimate(30);

		bool next_frame = false;
		while (!next_frame && !_finish) {
			SDL_Event event;
			if (!SDL_WaitEvent(&event)) {
				QuitProgram(); // Error in the event stream, quit.
				break;
			}
			switch(event.type) {
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						case SDLK_q:
							QuitProgram();
							break;

						case SDLK_LEFT:
							w->Rotate(-1);
							break;

						case SDLK_RIGHT:
							w->Rotate(1);
							break;

						default:
							break;
					}
					break;

				case SDL_MOUSEMOTION: {
					Point16 p;
					p.x = event.button.x;
					p.y = event.button.y;
					_manager.MouseMoveEvent(p);
					break;
				}

				case SDL_MOUSEBUTTONUP:
				case SDL_MOUSEBUTTONDOWN:
					switch (event.button.button) {
						case SDL_BUTTON_WHEELDOWN: {
							Point16 p;
							p.x = event.button.x;
							p.y = event.button.y;
							_manager.MouseMoveEvent(p);
							if (event.type == SDL_MOUSEBUTTONDOWN) _manager.MouseWheelEvent(-1);
							break;
						}

						case SDL_BUTTON_WHEELUP: {
							Point16 p;
							p.x = event.button.x;
							p.y = event.button.y;
							_manager.MouseMoveEvent(p);
							if (event.type == SDL_MOUSEBUTTONDOWN) _manager.MouseWheelEvent(1);
							break;
						}

						case SDL_BUTTON_LEFT: {
							Point16 p;
							p.x = event.button.x;
							p.y = event.button.y;
							_manager.MouseMoveEvent(p);
							_manager.MouseButtonEvent(MB_LEFT, (event.type == SDL_MOUSEBUTTONDOWN));
							break;
						}

						case SDL_BUTTON_MIDDLE: {
							Point16 p;
							p.x = event.button.x;
							p.y = event.button.y;
							_manager.MouseMoveEvent(p);
							_manager.MouseButtonEvent(MB_MIDDLE, (event.type == SDL_MOUSEBUTTONDOWN));
							break;
						}

						case SDL_BUTTON_RIGHT: {
							Point16 p;
							p.x = event.button.x;
							p.y = event.button.y;
							_manager.MouseMoveEvent(p);
							_manager.MouseButtonEvent(MB_RIGHT, (event.type == SDL_MOUSEBUTTONDOWN));
							break;
						}

						default:
							break;
					}
					break;

				case SDL_USEREVENT:
					next_frame = true;
					break;

				case SDL_VIDEOEXPOSE:
					vid.MarkDisplayDirty();
					UpdateWindows();
					break;

				case SDL_QUIT:
					QuitProgram();
					break;

				default:
					break; // Ignore other events.
			}
		}

		if (!missing_sprites_check && _video->missing_sprites) {
			/* Enough sprites are available for displaying an error message,
			 * as this was checked in GuiSprites::HasSufficientGraphics. */
			 ShowErrorMessage(GUI_ERROR_MESSAGE_SPRITE);
			 missing_sprites_check = true;
		}
	}

	SDL_RemoveTimer(timer_id); // Drop the timer.

	_mouse_modes.SetMouseMode(MM_INACTIVE);
	_manager.CloseAllWindows();
	UninitLanguage();
	vid.Shutdown();
	exit(0);
}

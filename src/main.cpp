/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file main.cpp Main program. */

#include "stdafx.h"
#include "video.h"
#include "map.h"
#include "window.h"
#include "sprite_store.h"
#include "input.h"

/**
 * Error handling for fatal non-user errors.
 * @param s the string to print.
 * @note \b Never returns.
 */
void CDECL error(const char *s, ...)
{
	va_list va;

	va_start(va, s);
	vfprintf(stderr, s, va);
	va_end(va);

	abort();
}

/** Main entry point. */
int main(void)
{
	VideoSystem vid;

	/* Load RCD file. */
	const char *err_msg = _sprite_store.Load("../rcd/groundtile_8bpp64.rcd");
	if (err_msg != NULL) {
		fprintf(stderr, "Failed to load '../rcd/groundtile_8bpp64.rcd'\n(%s)\n", err_msg);
		exit(1);
	}

	/* Initialize video. */
	if (!vid.Initialize()) {
		fprintf(stderr, "Failed to initialize window, aborting\n");
		exit(1);
	}
	vid.SetPalette();

	_world.SetWorldSize(20, 21);
	_world.MakeFlatWorld(0);
	_world.MakeBump(4, 6, 0);
	SetVideo(&vid);
	/* TEMP */
	uint16 width  = vid.GetXSize();
	uint16 height = vid.GetYSize();
	assert(width >= 20 && height >= 20);
	Viewport *w = new Viewport(5, 5, width - 10, height - 10);

	_input.SetMouseMode(MM_TILE_TERRAFORM);

	bool finished = false;
	while (!finished) {
		UpdateWindows();

		SDL_Event event;
		while (!finished && SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						case SDLK_q:
							finished = true;
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

				case SDL_MOUSEMOTION:
					_input.MouseMoveEvent(event.motion.x, event.motion.y);
					break;

				case SDL_MOUSEBUTTONUP:
				case SDL_MOUSEBUTTONDOWN:
					switch (event.button.button) {
						case SDL_BUTTON_WHEELDOWN:
							_input.MouseMoveEvent(event.button.x, event.button.y);
							_input.MouseWheelEvent(1);
							break;

						case SDL_BUTTON_WHEELUP:
							_input.MouseMoveEvent(event.button.x, event.button.y);
							_input.MouseWheelEvent(-1);
							break;

						case SDL_BUTTON_LEFT:
							_input.MouseMoveEvent(event.button.x, event.button.y);
							_input.MouseButtonEvent(MB_LEFT, (event.type == SDL_MOUSEBUTTONDOWN));
							break;

						default:
							break;
					}
					break;

				case SDL_USEREVENT:
					break;

				case SDL_VIDEOEXPOSE:
					vid.MarkDisplayDirty();
					UpdateWindows();
					break;

				case SDL_QUIT:
					finished = true;
					break;

				default:
					break; // Ignore other events.
			}
		}
	}

	vid.Shutdown();
	exit(0);
}

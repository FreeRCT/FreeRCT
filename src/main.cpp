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
#include "viewport.h"
#include "sprite_store.h"

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
	event.type = SDL_USEREVENT; // Other fields are "don't care"
	SDL_PushEvent(&event);

	return interval;
}

/** Main entry point. */
int main(void)
{
	VideoSystem vid;

	/* Load RCD file. */
	const char *err_msg = _sprite_manager.LoadRcdFiles();
	if (err_msg != NULL) {
		fprintf(stderr, "Failed to load RCD files\n(%s)\n", err_msg);
		exit(1);
	}

	if (!_sprite_manager.HaveSufficientGraphics()) {
		fprintf(stderr, "Insufficient graphics loaded, some parts may not be displayed correctly.\n");
	}

	/* Initialize video. */
	if (!vid.Initialize()) {
		fprintf(stderr, "Failed to initialize window, aborting\n");
		exit(1);
	}
	vid.SetPalette();

	_world.SetWorldSize(20, 21);
	_world.MakeFlatWorld(8);
	_manager.video = &vid;
	/* TEMP */
	uint16 width  = vid.GetXSize();
	uint16 height = vid.GetYSize();
	assert(width >= 120 && height >= 120);
	Viewport *w = new Viewport(50, 50, width - 100, height - 100);

	w->SetMouseMode(MM_TILE_TERRAFORM);

	SDL_TimerID timer_id = SDL_AddTimer(30, &NextFrame, NULL);

	bool finished = false;
	while (!finished) {
		/* For every frame do... */
		UpdateWindows();

		bool next_frame = false;
		while (!next_frame) {
			SDL_Event event;
			if (!SDL_WaitEvent(&event)) {
				finished = true; // Error in the event stream, quit.
				break;
			}
			switch(event.type) {
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						case SDLK_q:
							next_frame = true;
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
					next_frame = true;
					finished = true;
					break;

				default:
					break; // Ignore other events.
			}
		}
	}

	SDL_RemoveTimer(timer_id); // Drop the timer.

	vid.Shutdown();
	exit(0);
}

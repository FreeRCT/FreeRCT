/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file Main program. */

#include "stdafx.h"
#include "video.h"
#include "voxel_map.h"
#include "window.h"

int main(void)
{
	VideoSystem vid;

	if (!vid.Initialize()) {
		fprintf(stderr, "Failed to initialize window, aborting\n");
		exit(1);
	}

	_world.SetWorldSize(32, 32);
	SetVideo(&vid);
	/* TEMP */
	uint16 width  = vid.GetXSize();
	uint16 height = vid.GetYSize();
	assert(width >= 20 && height >= 20);
	Window *w = new Window(5, 5, width - 10, height - 10);

	bool finished = false;
	while (!finished) {
		SDL_Event event;
		if (SDL_WaitEvent(&event) != 1) break; // Error detected while waiting.

		switch(event.type) {
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym) {
					case SDLK_q:
						finished = true;
						break;

					case SDLK_LEFT:
						break;

					case SDLK_RIGHT:
						break;

					default:
						break;
				}
				break;

			case SDL_USEREVENT:
				break;

			case SDL_VIDEOEXPOSE: {
				UpdateWindows();
				break;
			}

			case SDL_QUIT:
				finished = true;
				break;

			default:
				break; // Ignore other events.
		}
	}

	vid.Shutdown();
	exit(0);
}

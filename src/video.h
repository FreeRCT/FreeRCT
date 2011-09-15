/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file video.h Video handling. */

#ifndef VIDEO_H
#define VIDEO_H

#include "SDL.h"

/**
 * Class representing the video system.
 */
class VideoSystem {
public:
	VideoSystem();
	~VideoSystem();

	bool Initialize();
	void Shutdown();

private:
	bool initialized;   ///< Video system is initialized.

	SDL_Surface *video; ///< Video surface.
};

#endif

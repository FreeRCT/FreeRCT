/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file video.cpp Video handling. */

#include "stdafx.h"
#include "video.h"

/**
 * Default constructor, does nothing, never goes wrong.
 * Call #Initialize to initialize the system.
 */
VideoSystem::VideoSystem()
{
	this->initialized = false;
}

/** Destructor. */
VideoSystem::~VideoSystem()
{
	if (this->initialized) this->Shutdown();
}

/**
 * Initialize the video system, preparing it for use.
 * @return Whether initialization was a success.
 */
bool VideoSystem::Initialize()
{
	if (this->initialized) return true;
	if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;

	this->video = SDL_SetVideoMode(800, 600, 8, SDL_HWSURFACE);
	if (this->video == NULL) {
		SDL_Quit();
		return false;
	}

	this->initialized = true;
	return true;
}

/** Close down the video system. */
void VideoSystem::Shutdown()
{
	if (this->initialized) {
		SDL_Quit();
		this->initialized = false;
	}
}

/**
 * Get horizontal size of the screen.
 * @return Number of pixels of the screen horizontally.
 */
uint16 VideoSystem::GetXSize() const
{
	assert(this->initialized);

	int w = SDL_GetVideoSurface()->w;
	assert(w >= 0 && w <= 0xFFFF);
	return (uint16)w;
}

/**
 * Get vertical size of the screen.
 * @return Number of pixels of the screen vertically.
 */
uint16 VideoSystem::GetYSize() const
{
	assert(this->initialized);

	int h = SDL_GetVideoSurface()->w;
	assert(h >= 0 && h <= 0xFFFF);
	return (uint16)h;
}


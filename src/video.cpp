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
#include "sprite_store.h"
#include "palette.h"

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

	SDL_WM_SetCaption("FreeRCT", "FreeRCT");

	this->initialized = true;
	this->dirty = true; // Ensure it gets painted.
	return true;
}

/**
 * Set up the palette for the video surface.
 * @param pd Palette to use.
 */
void VideoSystem::SetPalette()
{
	assert(this->initialized);

	SDL_Color colours[256];
	for (uint i = 0; i < 256; i++) {
		colours[i].r = _palette[i][0];
		colours[i].g = _palette[i][1];
		colours[i].b = _palette[i][2];
	}
	SDL_SetColors(SDL_GetVideoSurface(), colours, 0, 256);
}

/** Close down the video system. */
void VideoSystem::Shutdown()
{
	if (this->initialized) {
		SDL_Quit();
		this->initialized = false;
		this->dirty = false;
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

	int h = SDL_GetVideoSurface()->h;
	assert(h >= 0 && h <= 0xFFFF);
	return (uint16)h;
}

/**
 * Make the whole surface getting copied to the display.
 * @todo A somewhat less crude system would be nice, but it will have to wait
 *       until the window system is fleshed out more.
 *
 * @warning The #VideoSystem::dirty flag is not updated here!!
 */
static void UpdateScreen()
{
	SDL_Surface *s = SDL_GetVideoSurface();
	SDL_UpdateRect(s, 0, 0, s->w, s->h);
}

/** Lock the video surface to allow safe access to the pixel data. */
void VideoSystem::LockSurface()
{
	SDL_LockSurface(SDL_GetVideoSurface());
}

/** Release the video surface lock. */
void VideoSystem::UnlockSurface()
{
	SDL_UnlockSurface(SDL_GetVideoSurface());
	UpdateScreen();
	this->dirty = false;
}

/**
 * Fill the entire surface with a single colour.
 * @param colour Colour to fill with.
 * @pre Surface must be locked.
 */
void VideoSystem::FillSurface(uint8 colour)
{
	SDL_Surface *s = SDL_GetVideoSurface();
	uint8 *pixels = (uint8 *)s->pixels;
	for (int y = 0; y < s->h; y++) {
		memset(pixels, colour, s->w);
		pixels += s->pitch;
	}
}


/**
 * Blit pixels from the \a spr relative to \a img_base into the rectangle \a rect.
 * @param img_base Base coordinate of the sprite data.
 * @param spr The sprite to blit.
 * @param rect Coordinates to stay within.
 * @pre Surface must be locked.
 */
void VideoSystem::BlitImage(const Point &img_base, const Sprite *spr, const Rectangle & rect)
{
	SDL_Surface *s = SDL_GetVideoSurface();
	ImageData *img_data = spr->img_data;

	/* Check that rect is completely inside the screen. */
	assert(rect.base.x >= 0 && rect.base.y >= 0 && rect.width <= s->w && rect.height < s->h);

	/* Image is entirely outside the rectangle. */
	if (img_base.x >= rect.base.x + rect.width) return;
	if (img_base.x + img_data->width <= rect.base.x) return;
	if (img_base.y >= rect.base.y + rect.height) return;
	if (img_base.y + img_data->height <= rect.base.y) return;

	int width  = (img_base.x + img_data->width  <= rect.base.x + rect.width)  ? img_data->width  : rect.base.x + rect.width  - img_base.x;
	int height = (img_base.y + img_data->height <= rect.base.y + rect.height) ? img_data->height : rect.base.y + rect.height - img_base.y;

	int xoff = (img_base.x >= rect.base.x) ? 0 : rect.base.x - img_base.x;
	int xlen = (width > xoff) ? width - xoff : 0;
	int yoff = (img_base.y >= rect.base.y) ? 0 : rect.base.y - img_base.y;
	int ylen = (height > yoff) ? height - yoff : 0;

	int xend = xoff + xlen;
	while (ylen > 0) {
		uint32 offset = img_data->table[yoff];
		if (offset != ImageData::INVALID_JUMP) {
			uint16 xpos = 0;
			for (;;) {
				uint8 rel_off = img_data->data[offset];
				uint8 count   = img_data->data[offset + 1];
				uint8 *pixels = &img_data->data[offset + 2];
				offset += 2 + count;

				xpos += rel_off & 127;
				if (xpos >= xend) break; // Past the end.
				if (xpos < xoff) {
					if (xpos + count <= xoff) {
						/* Entirely before. */
						if ((rel_off & 128) != 0) break;
						xpos += count;
						continue;
					}
					/* Partially before. */
					count  -= xoff - xpos;
					pixels += xoff - xpos;
					xpos   += xoff - xpos;
				}

				while (count > 0) {
					/* Blit pixel. */
					assert(xpos + img_base.x < s->w);
					((uint8 *)(s->pixels))[xpos + img_base.x + s->pitch * (img_base.y + yoff)] = *pixels;

					pixels++;
					xpos++;
					if (xpos >= xend) goto end_line;
					count--;
				}
				if ((rel_off & 128) != 0) break;
			}
		}
end_line:
		yoff++;
		ylen--;
	}
}


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
#include "math_func.h"

ClippedRectangle::ClippedRectangle()
{
	this->absx = 0;
	this->absy = 0;
	this->width = 0;
	this->height = 0;
	this->address = NULL; this->pitch = 0;
}

/**
 * Construct a clipped rectangle from coordinates.
 * @param x Topleft x position.
 * @param y Topleft y position.
 * @param w Width.
 * @param h Height.
 */
ClippedRectangle::ClippedRectangle(uint16 x, uint16 y, uint16 w, uint16 h)
{
	this->absx = x;
	this->absy = y;
	this->width = w;
	this->height = h;
	this->address = NULL; this->pitch = 0;
}

/**
 * Construct a clipped rectangle inside an existing one.
 * @param cr Existing rectangle.
 * @param x Topleft x position.
 * @param y Topleft y position.
 * @param w Width.
 * @param h Height.
 * @note Rectangle is clipped to the old one.
 */
ClippedRectangle::ClippedRectangle(const ClippedRectangle &cr, uint16 x, uint16 y, uint16 w, uint16 h)
{
	if (x >= cr.width || y >= cr.height) {
		this->absx = 0;
		this->absy = 0;
		this->width = 0;
		this->height = 0;
		this->address = NULL; this->pitch = 0;
		return;
	}
	if (x + w > cr.width)  w = cr.width - x;
	if (y + h > cr.height) h = cr.height - y;

	this->absx = cr.absx + x;
	this->absy = cr.absy + y;
	this->width = w;
	this->height = h;
	this->address = NULL; this->pitch = 0;
}

/**
 * Construct an area based on a rectangle.
 * @param rect %Rectangle to use as source.
 */
ClippedRectangle::ClippedRectangle(const Rectangle &rect)
{
	this->absx   = (rect.base.x >= 0) ? (uint16)rect.base.x : 0;
	this->absy   = (rect.base.y >= 0) ? (uint16)rect.base.y : 0;
	this->width  = (rect.width  >= 0) ? (uint16)rect.width  : 0;
	this->height = (rect.height >= 0) ? (uint16)rect.height : 0;
	this->address = NULL; this->pitch = 0;
}

/**
 * Copy constructor.
 * @param cr Existing clipped rectangle.
 */
ClippedRectangle::ClippedRectangle(const ClippedRectangle &cr)
{
	this->absx = cr.absx;
	this->absy = cr.absy;
	this->width = cr.width;
	this->height = cr.height;
	this->address = cr.address; this->pitch = cr.pitch;
}

/**
 * Assignment operator override.
 * @param cr Existing clipped rectangle.
 */
ClippedRectangle &ClippedRectangle::operator=(const ClippedRectangle &cr)
{
	if (this != &cr) {
		this->absx = cr.absx;
		this->absy = cr.absy;
		this->width = cr.width;
		this->height = cr.height;
		this->address = cr.address; this->pitch = cr.pitch;
	}
	return *this;
}

/** Initialize the #address if not done already. */
void ClippedRectangle::ValidateAddress()
{
	if (this->address == NULL) {
		SDL_Surface *s = SDL_GetVideoSurface();
		this->pitch = s->pitch;
		this->address = ((uint8 *)(s->pixels)) + this->absx + this->absy * s->pitch;
	}
}

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
 * @param font_name Name of the font file to load.
 * @param font_size Size of the font.
 * @return Whether initialization was a success.
 */
bool VideoSystem::Initialize(const char *font_name, int font_size)
{
	if (this->initialized) return true;
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return false;

	this->video = SDL_SetVideoMode(800, 600, 8, SDL_HWSURFACE);
	if (this->video == NULL) {
		SDL_Quit();
		return false;
	}

	SDL_WM_SetCaption("FreeRCT", "FreeRCT");

	if (TTF_Init() != 0) {
		SDL_Quit();
		return false;
	}

	this->font = TTF_OpenFont(font_name, font_size);
	if (this->font == NULL) {
		TTF_Quit();
		SDL_Quit();
		return false;
	}

	this->initialized = true;
	this->dirty = true; // Ensure it gets painted.

	this->blit_rect = ClippedRectangle(0, 0, this->GetXSize(), this->GetYSize());
	return true;
}

/**
 * Set the clipped area.
 * @param cr New clipped blitting area.
 */
void VideoSystem::SetClippedRectangle(const ClippedRectangle &cr)
{
	this->blit_rect = cr;
}

/**
 * Get the current clipped blitting area.
 * @return Current clipped area.
 */
ClippedRectangle VideoSystem::GetClippedRectangle()
{
	this->blit_rect.ValidateAddress();
	return this->blit_rect;
}

/** Set up the palette for the video surface. */
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
		TTF_CloseFont(this->font);
		TTF_Quit();
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
 * @warning The VideoSystem::dirty flag is not updated here!!
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
}

/** Finish repainting, perform the final steps. */
void VideoSystem::FinishRepaint()
{
	UpdateScreen();
	MarkDisplayClean();
}


/**
 * Fill the rectangle with a single colour.
 * @param colour Colour to fill with.
 * @param rect %Rectangle to fill.
 * @pre Surface must be locked.
 * @todo Make it use #ClippedRectangle too.
 */
void VideoSystem::FillSurface(uint8 colour, const Rectangle &rect)
{
	SDL_Surface *s = SDL_GetVideoSurface();

	int x = Clamp((int)rect.base.x, 0, s->w);
	int w = Clamp((int)(rect.base.x + rect.width), 0, s->w);
	int y = Clamp((int)rect.base.y, 0, s->h);
	int h = Clamp((int)(rect.base.y + rect.height), 0, s->h);

	w -= x;
	h -= y;
	if (w == 0 || h == 0) return;

	uint8 *pixels = ((uint8 *)s->pixels) + x + y * s->pitch;
	while (h > 0) {
		memset(pixels, colour, w);
		pixels += s->pitch;
		h--;
	}
}

/**
 * Blit a pixel to an area of \a numx times \a numy sprites.
 * @param cr Clipped rectangle.
 * @param val Pixel value to blit.
 * @param xmin Minimal x position.
 * @param ymin Minimal y position.
 * @param numx Number of horizontal count.
 * @param numy Number of vertical count.
 * @param width Width of an image.
 * @param height Height of an image.
 */
static void BlitPixel(const ClippedRectangle &cr, uint8 *scr_base,
		int32 xmin, int32 ymin, uint16 numx, uint16 numy, uint16 width, uint16 height, uint8 val)
{
	const int32 xend = xmin + numx * width;
	const int32 yend = ymin + numy * height;
	while (ymin < yend) {
		if (ymin >= cr.height) return;

		if (ymin >= 0) {
			uint8 *scr = scr_base;
			int32 x = xmin;
			while (x < xend) {
				if (x >= cr.width) break;
				if (x >= 0) *scr = val;

				x += width;
				scr += width;
			}
		}
		ymin += height;
		scr_base += height * cr.pitch;
	}
}

/**
 * Blit pixels from the \a spr relative to \a img_base into the area.
 * @param x_base Base X coordinate of the sprite data.
 * @param y_base Base Y coordinate of the sprite data.
 * @param spr The sprite to blit.
 * @param numx Number of sprites to draw in horizontal direction.
 * @param numy Number of sprites to draw in vertical direction.
 * @pre Surface must be locked.
 */
void VideoSystem::BlitImages(int32 x_base, int32 y_base, const Sprite *spr, uint16 numx, uint16 numy)
{
	this->blit_rect.ValidateAddress();

	x_base += spr->xoffset;
	y_base += spr->yoffset;

	ImageData *img_data = spr->img_data;

	/* Don't draw wildly outside the screen. */
	while (numx > 0 && x_base + img_data->width < 0) {
		x_base += img_data->width; numx--;
	}
	while (numx > 0 && x_base + (numx - 1) * img_data->width >= this->blit_rect.width) numx--;
	if (numx == 0) return;

	while (numy > 0 && y_base + img_data->height < 0) {
		y_base += img_data->height; numy--;
	}
	while (numy > 0 && y_base + (numy - 1) * img_data->height >= this->blit_rect.height) numy--;
	if (numy == 0) return;

	uint8 *line_base = this->blit_rect.address + x_base + this->blit_rect.pitch * y_base;
	int32 ypos = y_base;
	for (int yoff = 0; yoff < img_data->height; yoff++) {
		uint32 offset = img_data->table[yoff];
		if (offset != ImageData::INVALID_JUMP) {
			int32 xpos = x_base;
			uint8 *src_base = line_base;
			for (;;) {
				uint8 rel_off = img_data->data[offset];
				uint8 count   = img_data->data[offset + 1];
				uint8 *pixels = &img_data->data[offset + 2];
				offset += 2 + count;

				xpos += rel_off & 127;
				src_base += rel_off & 127;
				while (count > 0) {
					BlitPixel(this->blit_rect, src_base, xpos, ypos, numx, numy, img_data->width, img_data->height, *pixels);
					pixels++;
					xpos++;
					src_base++;
					count--;
				}
				if ((rel_off & 128) != 0) break;
			}
		}
		line_base += this->blit_rect.pitch;
		ypos++;
	}
}


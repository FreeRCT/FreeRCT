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
#include "sprite_data.h"
#include "palette.h"
#include "math_func.h"
#include "bitmath.h"
#include "rev.h"

VideoSystem *_video = nullptr; ///< Global address of the video sub-system.

/** Default constructor of a clipped rectangle. */
ClippedRectangle::ClippedRectangle()
{
	this->absx = 0;
	this->absy = 0;
	this->width = 0;
	this->height = 0;
	this->address = nullptr; this->pitch = 0;
}

/**
 * Construct a clipped rectangle from coordinates.
 * @param x Top-left x position.
 * @param y Top-left y position.
 * @param w Width.
 * @param h Height.
 */
ClippedRectangle::ClippedRectangle(uint16 x, uint16 y, uint16 w, uint16 h)
{
	this->absx = x;
	this->absy = y;
	this->width = w;
	this->height = h;
	this->address = nullptr; this->pitch = 0;
}

/**
 * Construct a clipped rectangle inside an existing one.
 * @param cr Existing rectangle.
 * @param x Top-left x position.
 * @param y Top-left y position.
 * @param w Width.
 * @param h Height.
 * @note %Rectangle is clipped to the old one.
 */
ClippedRectangle::ClippedRectangle(const ClippedRectangle &cr, uint16 x, uint16 y, uint16 w, uint16 h)
{
	if (x >= cr.width || y >= cr.height) {
		this->absx = 0;
		this->absy = 0;
		this->width = 0;
		this->height = 0;
		this->address = nullptr; this->pitch = 0;
		return;
	}
	if (x + w > cr.width)  w = cr.width - x;
	if (y + h > cr.height) h = cr.height - y;

	this->absx = cr.absx + x;
	this->absy = cr.absy + y;
	this->width = w;
	this->height = h;
	this->address = nullptr; this->pitch = 0;
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
 * @return The assigned value.
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
	if (this->address == nullptr) {
		this->pitch = _video->GetXSize();
		this->address = _video->mem + this->absx + this->absy * this->pitch;
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
	if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;

	this->vid_width = 800;
	this->vid_height = 600;

	char caption[50];
	snprintf(caption, sizeof(caption), "FreeRCT %s", _freerct_revision);
	this->window = SDL_CreateWindow(caption, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, this->vid_width, this->vid_height, 0);
	if (this->window == nullptr) {
		SDL_Quit();
		fprintf(stderr, "Could not create window at %dx%d (%s)\n", this->vid_width, this->vid_height, SDL_GetError());
		return false;
	}

	this->renderer = SDL_CreateRenderer(this->window, -1, 0);
	if (this->renderer == nullptr) {
		SDL_Quit();
		fprintf(stderr, "Could not create renderer (%s)\n", SDL_GetError());
		return false;
	}

	this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, this->vid_width, this->vid_height);
	if (this->texture == nullptr) {
		SDL_Quit();
		fprintf(stderr, "Could not create texture (%s)\n", SDL_GetError());
		return false;
	}

	this->mem = new uint32[this->vid_width * this->vid_height];
	if (this->mem == nullptr) {
		SDL_Quit();
		fprintf(stderr, "Failed to obtain window display storage.\n");
		return false;
	}

	SDL_Surface *icon = SDL_LoadBMP("../graphics/sprites/logo/logo.32.bmp"); /// \todo Non hardcoded filepath.
	if (icon != nullptr) {
// XXX		SDL_SetColorKey(icon, SDL_SRCCOLORKEY, SDL_MapRGB(icon->format, 0, 0, 255)); // Replace blue
		SDL_SetWindowIcon(this->window, icon);
		SDL_FreeSurface(icon);
	}

	if (TTF_Init() != 0) {
		SDL_Quit();
		delete[] this->mem;
		return false;
	}

	this->font = TTF_OpenFont(font_name, font_size);
	if (this->font == nullptr) {
		TTF_Quit();
		SDL_Quit();
		delete[] this->mem;
		return false;
	}

	this->font_height = TTF_FontLineSkip(this->font);
	this->initialized = true;
	this->dirty = true; // Ensure it gets painted.
	this->missing_sprites = false;

	this->blit_rect = ClippedRectangle(0, 0, this->GetXSize(), this->GetYSize());
	this->digit_size.x = 0;
	this->digit_size.y = 0;

	return true;
}

/** Mark the entire display as being out of date (it needs the be repainted). */
void VideoSystem::MarkDisplayDirty()
{
	this->dirty = true;
}

/**
 * Mark the stated area of the screen as being out of date.
 * @param rect %Rectangle which is out of date.
 * @todo Keep an administration of the rectangle(s??) to update, and update just that part.
 */
void VideoSystem::MarkDisplayDirty(const Rectangle32 &rect)
{
	this->dirty = true;
}

/** Mark the display as being up-to-date. */
void VideoSystem::MarkDisplayClean()
{
	this->dirty = false;
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

/** Close down the video system. */
void VideoSystem::Shutdown()
{
	if (this->initialized) {
		TTF_CloseFont(this->font);
		TTF_Quit();
		SDL_Quit();
		delete[] this->mem;
		this->initialized = false;
		this->dirty = false;
	}
}

/**
 * Finish repainting, perform the final steps.
 * @todo Implement partial window repainting.
 */
void VideoSystem::FinishRepaint()
{
	SDL_UpdateTexture(this->texture, nullptr, this->mem, this->GetXSize() * sizeof(uint32)); // Upload memory to the GPU.
	SDL_RenderClear(this->renderer);
	SDL_RenderCopy(this->renderer, this->texture, nullptr, nullptr);
	SDL_RenderPresent(this->renderer);

	MarkDisplayClean();
}

/**
 * Fill the rectangle with a single colour.
 * @param colour Colour to fill with.
 * @param rect %Rectangle to fill.
 */
void VideoSystem::FillSurface(uint32 colour, const Rectangle32 &rect)
{
	ClippedRectangle cr = this->GetClippedRectangle();

	int x = Clamp((int)rect.base.x, 0, (int)cr.width);
	int w = Clamp((int)(rect.base.x + rect.width), 0, (int)cr.width);
	int y = Clamp((int)rect.base.y, 0, (int)cr.height);
	int h = Clamp((int)(rect.base.y + rect.height), 0, (int)cr.height);

	w -= x;
	h -= y;
	if (w == 0 || h == 0) return;

	uint32 *pixels_base = cr.address + x + y * cr.pitch;
	while (h > 0) {
		uint32 *pixels = pixels_base;
		for (int i = 0; i < w; i++) *pixels++ = colour;
		pixels_base += cr.pitch;
		h--;
	}
}

/**
 * Blit pixels from the \a spr relative to #blit_rect into the area.
 * @param img_base Coordinate of the sprite data.
 * @param spr The sprite to blit.
 * @param recolour Sprite recolouring definition.
 * @param shift Gradient shift.
 */
void VideoSystem::BlitImage(const Point32 &img_base, const ImageData *spr, const Recolouring &recolour, GradientShift shift)
{
	this->BlitImage(img_base.x, img_base.y, spr, recolour, shift);
}

/**
 * Blit an image at the specified position (top-left position) relative to #blit_rect.
 * @param x Horizontal position.
 * @param y Vertical position.
 * @param img The sprite image data to blit.
 * @param recolour Sprite recolouring definition.
 * @param shift Gradient shift.
 */
void VideoSystem::BlitImage(int x, int y, const ImageData *img, const Recolouring &recolour, GradientShift shift)
{
	if (img == nullptr) {
		this->missing_sprites = true;
		return;
	}

	int im_left = 0;
	int im_top = 0;
	int im_right = img->width;
	int im_bottom = img->height;

	x += img->xoffset;
	y += img->yoffset;

	if (x < 0) {
		im_left = -x;
		if (im_left >= im_right) return;
		x = 0;
	}
	if (y < 0) {
		im_top = -y;
		if (im_top >= im_bottom) return;
		y = 0;
	}

	this->blit_rect.ValidateAddress();
	if (x >= this->blit_rect.width || y >= this->blit_rect.height) return;

	if (x + im_right > this->blit_rect.width) {
		im_right = this->blit_rect.width - x;
		if (im_left >= im_right) return;
	}
	if (y + im_bottom > this->blit_rect.height) {
		im_bottom = this->blit_rect.height - y;
		if (im_top >= im_bottom) return;
	}

	uint32 *base = this->blit_rect.address + x + y * this->blit_rect.pitch;
	if (GB(img->flags, IFG_IS_8BPP, 1) != 0) {
		const uint8 *recoloured = recolour.GetPalette(shift); // Get the palette after recolouring and gradient shifting.
		/* Draw an 8bpp image. */
		for (; im_top < im_bottom; im_top++) {
			uint32 *dest = base;
			uint32 offset = img->table[im_top];
			if (offset == INVALID_JUMP) {
				base += this->blit_rect.pitch;
				continue;
			}

			int xpos = 0;
			for (;;) {
				uint8 rel_off = img->data[offset];
				uint8 count   = img->data[offset + 1];
				uint8 *pixels = &img->data[offset + 2];
				offset += 2 + count;

				if (xpos + (rel_off & 127) >= im_left) {
					if (xpos >= im_left) {
						dest += (rel_off & 127);
					} else {
						dest += (rel_off & 127) + xpos - im_left;
					}
				}
				xpos += rel_off & 127;

				for (; count > 0; count--) {
					if (xpos >= im_right) break;
					if (xpos >= im_left) {
						*dest = _palette[recoloured[*pixels]];
						dest++;
					}
					xpos++;
					pixels++;
				}
				if (xpos >= im_right || (rel_off & 128) != 0) break;
			}
			base += this->blit_rect.pitch;
		}
	} else {
		/* Draw a 32bpp image. */
		const uint8 *start = img->data;
		for (int ypos = 0; ypos < im_top; ypos++) start += start[0] | (start[1] << 8);
		for (; im_top < im_bottom; im_top++) {
			const uint8 *src = start + 2; // Skip length.
			start += start[0] + (start[1] << 8); // Jump to next line.
			uint32 *dest = base;
			int xpos = 0;
			for (;;) {
				uint8 mode = *src++;
				if (mode == 0) break;
				switch (mode >> 6) {
					case 0: // Fully opaque pixels.
						mode &= 0x3F;
						while (mode > 0) {
							if (xpos >= im_left) {
								*dest++ = MakeRGBA(src[0], src[1], src[2], OPAQUE);
							}
							xpos++;
							src += 3;
							mode--;
							if (xpos >= im_right) goto next_line;
						}
						break;

					case 1: { // Partial opaque pixels.
						uint8 opacity = *src++;
						mode &= 0x3F;
						while (mode > 0) {
							if (xpos >= im_left) {
								*dest++ = MakeRGBA(src[0], src[1], src[2], opacity);
							}
							xpos++;
							src += 3;
							mode--;
							if (xpos >= im_right) goto next_line;
						}
						break;
					}
					case 2: // Fully transparent pixels.
						mode &= 0x3F;
						if (xpos + mode >= im_left) {
							if (xpos >= im_left) {
								dest += mode;
							} else {
								dest += mode + xpos - im_left;
							}
						}
						xpos += mode;
						if (xpos >= im_right) goto next_line;
						break;

					case 3: { // Recoloured pixels.
						uint8 layer = *src++;
						uint8 opacity = *src++;
						mode &= 0x3F;
						while (mode > 0) {
							if (xpos >= im_left) {
								*dest++ = recolour.Recolour32bpp(*src++, shift, layer, opacity);
							}
							xpos++;
							mode--;
							if (xpos >= im_right) goto next_line;
						}
						break;
					}
				}
			}
next_line:
			base += this->blit_rect.pitch;
		}
	}
}

/**
 * Blit a pixel to an area of \a numx times \a numy sprites.
 * @param cr Clipped rectangle.
 * @param scr_base Base address of the screen array.
 * @param xmin Minimal x position.
 * @param ymin Minimal y position.
 * @param numx Number of horizontal count.
 * @param numy Number of vertical count.
 * @param width Width of an image.
 * @param height Height of an image.
 * @param colour Pixel value to blit.
 */
static void BlitPixel(const ClippedRectangle &cr, uint32 *scr_base,
		int32 xmin, int32 ymin, uint16 numx, uint16 numy, uint16 width, uint16 height, uint32 colour)
{
	const int32 xend = xmin + numx * width;
	const int32 yend = ymin + numy * height;
	while (ymin < yend) {
		if (ymin >= cr.height) return;

		if (ymin >= 0) {
			uint32 *scr = scr_base;
			int32 x = xmin;
			while (x < xend) {
				if (x >= cr.width) break;
				if (x >= 0) *scr = colour;

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
 * @param recolour Sprite recolouring definition.
 */
void VideoSystem::BlitImages(int32 x_base, int32 y_base, const ImageData *spr, uint16 numx, uint16 numy, const Recolouring &recolour)
{
	this->blit_rect.ValidateAddress();

	x_base += spr->xoffset;
	y_base += spr->yoffset;

	/* Don't draw wildly outside the screen. */
	while (numx > 0 && x_base + spr->width < 0) {
		x_base += spr->width; numx--;
	}
	while (numx > 0 && x_base + (numx - 1) * spr->width >= this->blit_rect.width) numx--;
	if (numx == 0) return;

	while (numy > 0 && y_base + spr->height < 0) {
		y_base += spr->height; numy--;
	}
	while (numy > 0 && y_base + (numy - 1) * spr->height >= this->blit_rect.height) numy--;
	if (numy == 0) return;

	uint32 *line_base = this->blit_rect.address + x_base + this->blit_rect.pitch * y_base;
	if (GB(spr->flags, IFG_IS_8BPP, 1) != 0) {
		const uint8 *recoloured = recolour.GetPalette(GS_NORMAL); // Get the palette after recolouring and gradient shifting.
		/* Drawing 8bpp image. */
		int32 ypos = y_base;
		for (int yoff = 0; yoff < spr->height; yoff++) {
			uint32 offset = spr->table[yoff];
			if (offset != INVALID_JUMP) {
				int32 xpos = x_base;
				uint32 *src_base = line_base;
				for (;;) {
					uint8 rel_off = spr->data[offset];
					uint8 count   = spr->data[offset + 1];
					uint8 *pixels = &spr->data[offset + 2];
					offset += 2 + count;

					xpos += rel_off & 127;
					src_base += rel_off & 127;
					while (count > 0) {
						uint32 colour = _palette[recoloured[*pixels]];
						BlitPixel(this->blit_rect, src_base, xpos, ypos, numx, numy, spr->width, spr->height, colour);
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
	} else {
		/* Drawing 32bpp image. */
		int32 ypos = y_base;
		const uint8 *src = spr->data + 2; // Skip the length word.
		for (int yoff = 0; yoff < spr->height; yoff++) {
			int32 xpos = x_base;
			uint32 *src_base = line_base;
			for (;;) {
				uint8 mode = *src++;
				if (mode == 0) break;
				switch (mode >> 6) {
					case 0: // Fully opaque pixels.
						mode &= 0x3F;
						for (; mode > 0; mode--) {
							uint32 colour = MakeRGBA(src[0], src[1], src[2], OPAQUE);
							BlitPixel(this->blit_rect, src_base, xpos, ypos, numx, numy, spr->width, spr->height, colour);
							xpos++;
							src_base++;
						}
						break;

					case 1: { // Partial opaque pixels.
						uint opacity = *src++;
						mode &= 0x3F;
						for (; mode > 0; mode--) {
							uint32 colour = MakeRGBA(src[0], src[1], src[2], opacity);
							BlitPixel(this->blit_rect, src_base, xpos, ypos, numx, numy, spr->width, spr->height, colour);
							xpos++;
							src_base++;
						}
						break;
					}
					case 2: // Fully transparent pixels.
						xpos += mode & 0x3F;
						src_base += mode & 0x3F;
						break;

					case 3: { // Recoloured pixels.
						uint8 layer = *src++;
						uint8 opacity = *src++;
						mode &= 0x3F;
						for (; mode > 0; mode--) {
							uint32 colour = recolour.Recolour32bpp(*src++, 0, layer, opacity);
							BlitPixel(this->blit_rect, src_base, xpos, ypos, numx, numy, spr->width, spr->height, colour);
							xpos++;
							src_base++;
						}
						break;
					}
				}
			}
			line_base += this->blit_rect.pitch;
			ypos++;
			src += 2; // Skip the length word.
		}
	}
}

/**
 * Get the text-size of a string.
 * @param text Text to calculate.
 * @param width [out] Resulting width.
 * @param height [out] Resulting height.
 */
void VideoSystem::GetTextSize(const uint8 *text, int *width, int *height)
{
	if (TTF_SizeUTF8(this->font, (const char *)text, width, height) != 0) {
		*width = 0;
		*height = 0;
	}
}

/**
 * Get the text-size of a number between \a smallest and \a biggest.
 * @param smallest Smallest possible number to display.
 * @param biggest Biggest possible number to display.
 * @param width [out] Resulting width.
 * @param height [out] Resulting height.
 */
void VideoSystem::GetNumberRangeSize(int64 smallest, int64 biggest, int *width, int *height)
{
	if (this->digit_size.x == 0) { // First call, initialize the variable.
		this->digit_size.x = 0;
		this->digit_size.y = 0;
		for (int i = '0'; i < '9'; i++) {
			uint8 buffer[2];
			buffer[0] = i;
			buffer[1] = '\0';
			int w, h;
			this->GetTextSize(buffer, &w, &h);
			if (w > this->digit_size.x) this->digit_size.x = w;
			if (h > this->digit_size.y) this->digit_size.y = h;
		}
	}

	int digit_count = 1;
	if (smallest < 0) {
		smallest = -smallest;
		digit_count++; // Add a 'digit' for the '-' sign.
	}
	if (biggest < 0) biggest = -biggest;
	if (smallest > biggest) biggest = smallest;
	while (biggest > 9) {
		digit_count++;
		biggest /= 10;
	}

	*width = digit_count * this->digit_size.x;
	*height = this->digit_size.y;
}

/**
 * Blit text to the screen.
 * @param text Text to display.
 * @param colour Colour of the text.
 * @param xpos Absolute horizontal position at the display.
 * @param ypos Absolute vertical position at the display.
 * @param width Available width of the text (in pixels).
 * @param align Horizontal alignment of the string.
 */
void VideoSystem::BlitText(const uint8 *text, uint32 colour, int xpos, int ypos, int width, Alignment align)
{
	SDL_Color col = {0, 0, 0}; // Font colour does not matter as only the bitmap is used.
	SDL_Surface *surf = TTF_RenderUTF8_Solid(this->font, (const char *)text, col);
	if (surf == nullptr) {
		fprintf(stderr, "Rendering text failed (%s)\n", TTF_GetError());
		return;
	}

	if (surf->format->BitsPerPixel != 8 || surf->format->BytesPerPixel != 1) {
		fprintf(stderr, "Rendering text failed (Wrong surface format)\n");
		return;
	}

	int real_w = std::min(surf->w, width);
	switch (align) {
		case ALG_LEFT:
			break;

		case ALG_CENTER:
			xpos += (width - real_w) / 2;
			break;

		case ALG_RIGHT:
			xpos += width - real_w;
			break;

		default: NOT_REACHED();
	}

	this->blit_rect.ValidateAddress();

	uint8 *src = ((uint8 *)surf->pixels);
	uint32 *dest = this->blit_rect.address + xpos + ypos * this->blit_rect.pitch;
	int h = surf->h;
	if (ypos < 0) {
		h += ypos;
		src  -= ypos * surf->pitch;
		dest -= ypos * this->blit_rect.pitch;
		ypos = 0;
	}
	while (h > 0) {
		if (ypos >= this->blit_rect.height) break;
		uint8 *src2 = src;
		uint32 *dest2 = dest;
		int w = real_w;
		int x = xpos;
		if (x < 0) {
			w += x;
			if (w <= 0) break;
			dest2 -= x;
			src2 -= x;
			x = 0;
		}
		while (w > 0) {
			if (x >= this->blit_rect.width) break;
			if (*src2 != 0) *dest2 = colour;
			src2++;
			dest2++;
			x++;
			w--;
		}
		ypos++;
		src  += surf->pitch;
		dest += this->blit_rect.pitch;
		h--;
	}

	SDL_FreeSurface(surf);
}

/**
 * Draw a line from \a start to \a end using the specified \a colour.
 * @param start Starting point at the screen.
 * @param end End point at the screen.
 * @param colour Colour to use.
 */
void VideoSystem::DrawLine(const Point16 &start, const Point16 &end, uint32 colour)
{
	int16 dx, inc_x, dy, inc_y;
	if (start.x > end.x) {
		dx = start.x - end.x;
		inc_x = -1;
	} else {
		dx = end.x - start.x;
		inc_x = 1;
	}
	if (start.y > end.y) {
		dy = start.y - end.y;
		inc_y = -1;
	} else {
		dy = end.y - start.y;
		inc_y = 1;
	}

	int16 step = std::min(dx, dy);
	int16 pos_x = start.x;
	int16 pos_y = start.y;
	int16 sum_x = 0;
	int16 sum_y = 0;

	this->blit_rect.ValidateAddress();
	uint32 *dest = this->blit_rect.address + pos_x + pos_y * this->blit_rect.pitch;

	for (;;) {
		/* Blit pixel. */
		if (pos_x >= 0 && pos_x < this->blit_rect.width && pos_y >= 0 && pos_y < this->blit_rect.height) {
			*dest = colour;
		}
		if (pos_x == end.x && pos_y == end.y) break;

		sum_x += step;
		sum_y += step;
		if (sum_x >= dy) {
			pos_x += inc_x;
			dest += inc_x;
			sum_x -= dy;
		}
		if (sum_y >= dx) {
			pos_y += inc_y;
			dest += inc_y * this->blit_rect.pitch;
			sum_y -= dx;
		}
	}
}

/**
 * Draw a rectangle at the screen.
 * @param rect %Rectangle to draw.
 * @param colour Colour to use.
 */
void VideoSystem::DrawRectangle(const Rectangle32 &rect, uint32 colour)
{
	Point16 top_left     = {static_cast<int16>(rect.base.x),                  static_cast<int16>(rect.base.y)};
	Point16 top_right    = {static_cast<int16>(rect.base.x + rect.width - 1), static_cast<int16>(rect.base.y)};
	Point16 bottom_left  = {static_cast<int16>(rect.base.x),                  static_cast<int16>(rect.base.y + rect.height - 1)};
	Point16 bottom_right = {static_cast<int16>(rect.base.x + rect.width - 1), static_cast<int16>(rect.base.y + rect.height - 1)};
	this->DrawLine(top_left, top_right, colour);
	this->DrawLine(top_left, bottom_left, colour);
	this->DrawLine(top_right, bottom_right, colour);
	this->DrawLine(bottom_left, bottom_right, colour);
}

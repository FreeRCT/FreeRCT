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
#include "SDL_ttf.h"
#include "geometry.h"

class Sprite;
class PaletteData;

/** Clipped rectangle. */
class ClippedRectangle {
public:
	ClippedRectangle();
	ClippedRectangle(uint16 x, uint16 y, uint16 w, uint16 h);
	ClippedRectangle(const ClippedRectangle &cr, uint16 x, uint16 y, uint16 w, uint16 h);
	ClippedRectangle(const Rectangle &rect);

	ClippedRectangle(const ClippedRectangle &cr);
	ClippedRectangle &operator=(const ClippedRectangle &cr);

	void ValidateAddress();

	uint16 absx;    ///< Absolute X position in the screen of the top-left.
	uint16 absy;    ///< Absolute Y position in the screen of the top-left.
	uint16 width;   ///< Number of columns.
	uint16 height;  ///< Number of rows.

	uint8 *address; ///< Base address. @note Call #ValidateAddress prior to use.
	uint16 pitch;   ///< Pitch of a row in bytes. @note Call #ValidateAddress prior to use.
};


/**
 * Class representing the video system.
 */
class VideoSystem {
public:
	VideoSystem();
	~VideoSystem();

	bool Initialize(const char *font_name, int font_size);
	void SetPalette();
	void Shutdown();

	uint16 GetXSize() const;
	uint16 GetYSize() const;

	/**
	 * Query whether the display needs to be repainted.
	 * @return Display needs an update.
	 */
	FORCEINLINE bool DisplayNeedsRepaint()
	{
		return this->dirty;
	}

	/** Mark the display as being out of date (it needs the be repainted). */
	FORCEINLINE void MarkDisplayDirty()
	{
		this->dirty = true;
	}

	/** Mark the display as being up-to-date. */
	FORCEINLINE void MarkDisplayClean()
	{
		this->dirty = false;
	}

	void SetClippedRectangle(const ClippedRectangle &cr);
	ClippedRectangle GetClippedRectangle();

	void LockSurface();
	void UnlockSurface();
	void FillSurface(uint8 colour, const Rectangle &rect);

	/**
	 * Blit pixels from the \a spr relative to \a img_base into the area.
	 * @param img_base Base coordinate of the sprite data.
	 * @param spr The sprite to blit.
	 * @pre Surface must be locked.
	 */
	FORCEINLINE void BlitImage(const Point32 &img_base, const Sprite *spr)
	{
		this->BlitImages(img_base.x, img_base.y, spr, 1, 1);
	}

	/**
	 * Blit pixels from the \a spr relative to \a img_base into the area.
	 * @param x Horizontal base coordinate.
	 * @param y Vertical base coordinate.
	 * @param spr The sprite to blit.
	 * @pre Surface must be locked.
	 */
	FORCEINLINE void BlitImage(int x, int y, const Sprite *spr)
	{
		this->BlitImages(x, y, spr, 1, 1);
	}

	/**
	 * Blit a row of sprites.
	 * @param xmin Start X position.
	 * @param numx Number of sprites to draw.
	 * @param y Y position.
	 * @param spr Sprite to draw.
	 */
	FORCEINLINE void BlitHorizontal(int32 xmin, uint16 numx, int32 y, const Sprite *spr)
	{
		this->BlitImages(xmin, y, spr, numx, 1);
	}

	/**
	 * Blit a column of sprites.
	 * @param ymin Start Y position.
	 * @param numy Number of sprites to draw.
	 * @param x X position.
	 * @param spr Sprite to draw.
	 */
	FORCEINLINE void BlitVertical(int32 ymin, uint16 numy, int32 x, const Sprite *spr)
	{
		this->BlitImages(x, ymin, spr, 1, numy);
	}

	void BlitImages(int32 x_base, int32 y_base, const Sprite *spr, uint16 numx, uint16 numy);

	void FinishRepaint();

	void GetTextSize(const char *text, int *width, int *height);
	void BlitText(const char *text, int xpos, int ypos, uint8 colour);

private:
	bool initialized;   ///< Video system is initialized.
	bool dirty;         ///< Video display needs being repainted.

	TTF_Font *font;             ///< Opened text font.
	SDL_Surface *video;         ///< Video surface.
	ClippedRectangle blit_rect; ///< Rectangle to blit in.
};

extern VideoSystem *_video;

#endif
